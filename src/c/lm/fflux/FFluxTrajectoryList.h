/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
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
 * - Neither the names of the Roberts Group, Johns Hopkins University,
 * nor the names of its contributors may be used to endorse or
 * promote products derived from this Software without specific prior written
 * permission.
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

#ifndef FFLUXTRAJECTORYLIST_H_
#define FFLUXTRAJECTORYLIST_H_

#include <map>
#include <string>
#include <vector>

#include "lm/io/FFluxParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/resource/TrajectoryList.h"
#include "lm/rng/XORShift.h"
#include "lm/Types.h"

using std::map;
using std::vector;

typedef vector<lm::io::TrajectoryState *> CrossingVector;
typedef map<long long, CrossingVector> CrossingsMap;

namespace lm {
namespace fflux {

class FFluxTrajectoryList : public lm::resource::TrajectoryList
{
public:
    // enumerated type used for describing the direction of the current fflux simulation relative to the arrangements (low-to-high or high-to-low) of the individual interfaces
    enum direction {FORWARD, BACKWARD};
    FFluxTrajectoryList(uint64_t simultaneousTrajectoryCount, double zerothInterface, map<std::string,std::string>& simulationParameters, const lm::io::ReactionModel& reactionModel, const lm::io::FFluxParameters& ffluxParameters);
    virtual ~FFluxTrajectoryList();
    virtual void init();
    virtual void initTrajectory(uint64_t trajectoryID, lm::io::TrajectoryState* trajectoryState);
    virtual void initPhaseZeroTrajectory(lm::io::TrajectoryState* oldCrossing);
    virtual void initPhaseNTrajectories(uint64_t trajectoriesToStart, long long lastFFluxPhase);
    virtual lm::io::TrajectoryState* initFirstTrajectoryState();
    virtual void reset();
    virtual void reverse();
    virtual void workUnitFinished(const lm::message::FinishedWorkUnit & finishedWorkUnitMsg);

    // Returns a randomly chosen crossing event (in the form of a TrajectoryState) collected durring forward flux phase ffluxPhase
    virtual lm::io::TrajectoryState* getRandomCrossing(long long ffluxPhase);
    virtual void initLimits(uint ifaceIndex);
    virtual void incrLimits(uint ifaceIndex);
    virtual void setLowLimit(uint ifaceIndex, double lowLimit);
    virtual void setHighLimit(uint ifaceIndex, double highLimit);
    virtual void setLimits(uint ifaceIndex, double lowLimit, double highLimit);
protected:
    //// TEMP
    virtual double calcTestCaseOParam(const lm::io::TrajectoryState& finalState);
    virtual void incrTestCaseLimits();
    //// TEMP

    map<std::string,std::string>& simulationParameters;
    const lm::io::ReactionModel& reactionModel;
    const lm::io::FFluxParameters& ffluxParams;
    lm::rng::XORShift xorShift;	//RNG used for randomly choosing a crossing in a crossing vector
    uint64_t simultaneousTrajectoryCount;
    direction direction;
    long long ffluxPhase;
    unsigned crossingsPerPhase;	//the count of crossing events that should be collected for every fflux sampling phase

    // variables related to how the forward flux interfaces are set up
    double zerothInterface;
    double finalInterface;
    long long interfaceCount;
	double maxPhaseZeroTime;
	vector<long long> finishedTrajectoriesCounts;
	CrossingsMap crossings;
	long long maxFFluxPhase;
	double oParamStep;
};

}
}

#endif
