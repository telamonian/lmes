/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the Software), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimers.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimers in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS WITH THE SOFTWARE.
 *
 * Author(s): Elijah Roberts, Max Klein
 */
#ifndef LM_CME_CMESOLVER_H_
#define LM_CME_CMESOLVER_H_

#include <algorithm>
#include <cstdio>
#include <list>
#include <map>
#include <pthread.h>
#include <string>
#include <utility>
#include <vector>

#include "lm/EnumHelper.h"
#include "lm/Math.h"
#include "lm/Types.h"
#include "lm/cme/ReactionModel.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/ParameterValues.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/me/FPTDeque.h"
#include "lm/me/MESolver.h"
#include "lm/me/PropensityFunction.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/oparam/OrderParameterFunction.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/thread/Thread.h"
#include "lm/tiling/Tilings.h"
#include "lm/trajectory/TrajectoryLimits.h"

using std::list;
using std::map;
using std::pair;
using std::string;
using std::vector;
using lm::me::MESolver;
using lm::rng::RandomGenerator;
using lm::trajectory::TrajectoryLimit;

namespace lm {

namespace ioFirstOrderPropensity {
class ReactionModel;
}

namespace cme {

class CMESolver : public MESolver
{
protected:

    class TilingHist
    {
    public:
        // consructor reads in a TilingHistBuf object
        TilingHist(): numberTileVals(0), tilingID(), tileVals(NULL)
        {
        }
        ~TilingHist()
        {
            delete tileVals;
        }
        // consructor reads in a TilingHistBuf object
        void init(const lm::io::TilingHist& tHistBuf)
        {
            numberTileVals = tHistBuf.tile_vals_size();
            tilingID = tHistBuf.tiling_id();
            tileVals = new double[numberTileVals];
            for (uint i=0;i<numberTileVals;i++)
            {
                tileVals[i] = tHistBuf.tile_vals(i);
            }
        }
        // this function writes out to a TilingHistBuf object
        void serializeTo(lm::io::TilingHist* tHistBuf)
        {
            tHistBuf->set_tiling_id(tilingID);
            tHistBuf->clear_tile_vals();
            for (uint i=0;i<numberTileVals;i++)
            {
                tHistBuf->add_tile_vals(tileVals[i]);
            }
        }
        uint numberTileVals;
        uint tilingID;
        double* tileVals;
    };

public:
    CMESolver(RandomGenerator::Distributions neededDists);
    virtual ~CMESolver();
    virtual void setComputeResources(vector<int> cpus, vector<int> gpus);
    virtual bool needsReactionModel() {return true;}
    virtual void setReactionModel(const lm::io::ReactionModel& rm);
    virtual bool needsDiffusionModel() {return false;}
    virtual void setDiffusionModel(const lm::io::DiffusionModel& dm) {}
    virtual void setOrderParameters(const lm::io::OrderParameters& opsBuf);
    virtual void setTilings(const lm::io::Tilings& tilingsBuf);
    virtual void setLimits(const lm::io::TrajectoryLimits& limits);
    virtual void setOutputOptions(const lm::io::OutputOptions& outputOptions);
    virtual void reset();
    virtual void getState(lm::io::TrajectoryState* state, uint trajectoryNumber=0);
    virtual void setState(const lm::io::TrajectoryState& state, uint trajectoryNumber=0);
    virtual lm::message::WorkUnitOutput* getOutput(uint trajectoryNumber=0);
    virtual lm::message::WorkUnitStatus::Status getStatus(uint trajectoryNumber=0);

protected:
    inline void performReactionEvent(uint r)
    {
        // Update the counts according to the dependency tables.
        for (int i=0; i<(int)reactionModel->numberDependentSpecies[r]; i++)
        {
            speciesCounts[reactionModel->dependentSpecies[r][i]] += reactionModel->dependentSpeciesChange[r][i];
        }
        if (hasUpdateSpeciesCountsListeners) callUpdateSpeciesCountsListeners(r);
    }

    inline void callUpdateSpeciesCountsListeners(uint r)
    {
        // Update the degree advancement
        if (trackingDegreeAdvancements)
        {
            degreeAdvancements[r]++;
        }

        // Update the first passage time tables.
        for (int i=0; i<numberFptSpecies; i++)
        {
            int value = speciesCounts[fptValues[i].species];
            if (value < fptValues[i].minValue || value > fptValues[i].maxValue)
                fptValues[i].insert(value, time);
        }

        // Update any order parameters.
        for (int i=0; i<numberOrderParameters; i++)
        {
            orderParameterPreviousValues[i] = orderParameterValues[i];
            orderParameterValues[i] = orderParameterFunctions[i]->calculate(time, speciesCounts, reactionModel->numberSpecies);
        }

//        // Update any tilingHists.
//        if (tilings != NULL)
//        {
//            for (int i=0;i<numberTilingHists;i++)
//            {
//                tilingHists[i].tileVals[(*tilings)[tilingHists[i].tilingID]->getTileIndex((*oparams)[(*tilings)[tilingHists[i].tilingID]->getOrderParameterID()]->get())] += timeStep;
//            }
//        }
    }
    bool isTrajectoryOutsideLimits();

protected:
    RandomGenerator::Distributions neededDists;
    RandomGenerator * rng;
    ReactionModel* reactionModel;
    bool hasUpdateSpeciesCountsListeners;
    lm::tiling::Tilings* tilings;

    // Degree advancement tracking
    bool trackingDegreeAdvancements;

    // Order parameter function.
    int32_t numberOrderParameters;
    lm::oparam::OrderParameterFunction** orderParameterFunctions;

    // Trajectory output.
    lm::message::WorkUnitOutput* output;

    // Trajectory status.
    lm::message::WorkUnitStatus::Status status;
    uint64_t trajectoryId;
    bool previouslyStarted;

    // Limits for the trajectory.
    lm::trajectory::TrajectoryLimits trajectoryLimits;
    double timeLimit;
    size_t numberLimits;
    TrajectoryLimit* limits;
    int32_t limitIDReached;
    lm::io::TrajectoryLimits::LimitType limitTypeReached;

    // Output options.
    bool writeDegreeAdvancementTimeSeries, writeOrderParameterTimeSeries, writeSpeciesTimeSeries;
    double degreeAdvancementWriteInterval, orderParameterWriteInterval, speciesWriteInterval;

    //First passage time variables.
    int numberFptSpecies;
    lm::me::FPTDeque* fptValues;

    // The current state.
    uint64_t* degreeAdvancements;
    double* orderParameterValues;
    double* orderParameterPreviousValues;
    int32_t* speciesCounts;
    double time;
    double timeStep;    // stores last time step calculated, used for building histogram

    uint numberTilingHists;
    TilingHist* tilingHists;
};

}
}

#endif
