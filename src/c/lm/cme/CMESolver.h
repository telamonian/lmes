/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2015 Roberts Group,
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
#include <deque>
#include <list>
#include <map>
#include <pthread.h>
#include <string>
#include <utility>
#include <vector>

#include "lm/Math.h"
#include "lm/Types.h"
#include "lm/cme/ReactionModel.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/ParameterValues.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/main/Main.h"
#include "lm/me/MESolver.h"
#include "lm/me/PropensityFunction.h"
#include "lm/oparam/OParams.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/thread/Thread.h"
#include "lm/tiling/Tilings.h"

using std::list;
using std::map;
using std::pair;
using std::string;
using std::vector;
using lm::me::MESolver;
using lm::rng::RandomGenerator;

namespace lm {

namespace ioFirstOrderPropensity {
class ReactionModel;
}

namespace cme {

class CMESolver : public MESolver
{
protected:

    class SpeciesLimit
    {
    public:
        enum limit_type_t {MIN, MAX, DECREASING_ASCENDING, INCREASING_ASCENDING, DECREASING_DESCENDING, INCREASING_DESCENDING};
        limit_type_t type;
        int species;
        double limit;
    };
    class FPTTracking
    {
    public:
        int species;
        int minValueAchieved;
        int maxValueAchieved;
        std::deque<std::pair<int,double> > fptValues;
        void serializeTo(uint64_t trajectoryId, lm::io::FirstPassageTimes* fpt)
        {
            fpt->set_trajectory_id(trajectoryId);
            fpt->set_species(species);
            fpt->set_number_entries(fptValues.size());
            for (std::deque<std::pair<int,double> >::iterator it=fptValues.begin(); it != fptValues.end(); it++)
            {
                fpt->add_species_count(it->first);
                fpt->add_first_passage_time(it->second);
            }
        }
    };
    struct TrackedParameter
    {
        TrackedParameter(string name, double * valuePointer):name(name),valuePointer(valuePointer) {dataSet.set_parameter(name);}
        string name;
        double * valuePointer;
        lm::io::ParameterValues dataSet;
    };
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
    virtual bool needsOrderParameters() {return ffluxFlag;}
    virtual void setOrderParameters(const lm::io::OrderParameters& opsBuf);
    virtual bool needsTilings() {return ffluxFlag;}
    virtual void setTilings(const lm::io::Tilings& tilingsBuf);
    virtual void reset();
    virtual void getState(lm::io::TrajectoryState* state);
    virtual void setState(const lm::io::TrajectoryState& state);
    virtual void setLimits(const lm::io::TrajectoryLimits& limits);
    virtual lm::io::TrajectoryLimits::LimitType getFinalLimitType();

protected:
    virtual void setSpeciesUpperLimit(int species, int limit);
    virtual void setSpeciesLowerLimit(int species, int limit);
    virtual void setSpeciesDecreasingLimit(lm::io::TrajectoryLimits::Arrangement, int opID, double limit);
    virtual void setSpeciesIncreasingLimit(lm::io::TrajectoryLimits::Arrangement, int opID, double limit);
    virtual void addToParameterTrackingList(pair<string,double*>parameter);

    //virtual double recordParameters(double nextRecordTime, double recordInterval, double simulationTime);
    //virtual void queueRecordedParameters(bool flush=false);

    inline void performReactionEvent(uint r)
    {
    	// Update the counts according to the dependency tables.
        for (int i=0; i<(int)reactionModel->numberDependentSpecies[r]; i++)
        {
            speciesCounts[reactionModel->dependentSpecies[r][i]] += reactionModel->dependentSpeciesChange[r][i];
            updatedSpeciesCounts();
        }
        if (ffluxFlag==true)
        {
            // Update the order parameters, if required
            for (uint i=0; i<oparams->size(); i++)
            {
                (*oparams)[i]->calc((uint*)speciesCounts);
            }
            // Update the tilingHists, if required
//            for (int i=0;i<numberTilingHists;i++)
//            {
//                tilingHists[i].tileVals[(*tilings)[tilingHists[i].tilingID]->getTileIndex((*oparams)[(*tilings)[tilingHists[i].tilingID]->getOrderParameterID()]->get())] += timeStep;
//            }
        }
    }

    inline void updatedSpeciesCounts()
    {
        // Update the first passage time tables.
        for (int i=0; i<numberFptTrackedSpecies; i++)
        {
            int speciesCount = speciesCounts[fptTrackedSpecies[i].species];
            while (speciesCount < fptTrackedSpecies[i].minValueAchieved)
            {
                fptTrackedSpecies[i].fptValues.push_front(std::pair<int,double>(--fptTrackedSpecies[i].minValueAchieved,time));
            }
            while (speciesCount > fptTrackedSpecies[i].maxValueAchieved)
            {
                fptTrackedSpecies[i].fptValues.push_back(std::pair<int,double>(++fptTrackedSpecies[i].maxValueAchieved,time));
            }
        }
    }

    virtual bool isTrajectoryOutsideLimits();


protected:
    RandomGenerator::Distributions neededDists;
    RandomGenerator * rng;
    ReactionModel* reactionModel;
    lm::oparam::OParams* oparams;
    lm::tiling::Tilings* tilings;

    // The current limits.
    double maxTime;
    uint numberSpeciesLimits;
    SpeciesLimit* speciesLimits;

    list<TrackedParameter> trackedParameters;

    // The current state.
    uint64_t trajectoryId;
    bool trajectoryStarted;
    int* speciesCounts;
    int* previousSpeciesCounts;
    double time;
    double timeStep;    // stores last time step calculated, used for building histogram
    int numberFptTrackedSpecies;
    FPTTracking* fptTrackedSpecies;
    uint numberTilingHists;
    TilingHist* tilingHists;

    // the type limit that stopped the trajectory. only has meaning after the trajectory's last step
    lm::io::TrajectoryLimits::LimitType finalLimitType;
};

}
}

#endif
