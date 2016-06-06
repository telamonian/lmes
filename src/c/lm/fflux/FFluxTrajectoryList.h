/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
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
#ifndef LM_FFLUX_FFLUXTRAJECTORYLIST_H
#define LM_FFLUX_FFLUXTRAJECTORYLIST_H

#include <google/protobuf/repeated_field.h>
#include <map>
#include <string>
#include <vector>

#include "lm/fflux/FFluxTrajectory.h"
#include "lm/input/Input.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Communicator.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/rng/XORShift.h"
#include "lm/tiling/Tilings.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/trajectory/TrajectoryList.h"
#include "lm/Types.h"

namespace lm {
namespace fflux {

typedef std::vector<lm::io::TrajectoryState*> CrossingVector;
typedef std::map<long long, CrossingVector> CrossingsMap;
typedef std::map<long long, double> DwellTimeMap;
typedef std::map<long long, long long> FinishedTrajectoriesCountMap;
typedef std::vector<lm::io::TilingHist*> TilingVector;

class FFluxTrajectoryList : public lm::trajectory::TrajectoryList
{
    friend class FFluxSupervisor;
public:
    // enumerated type used for describing the direction of the current fflux simulation relative to the arrangements (low-to-high or high-to-low) of the individual interfaces
    enum Direction {FORWARD, BACKWARD};
    // enumerated type used for picking which phase termination check to use
    enum PhaseCheck {CROSSINGS, TIME};

//    FFluxTrajectoryList(uint64_t simultaneousTrajectoryCount,const lm::io::ReactionModel& reactionModel,const lm::io::DiffusionModel& diffusionModel,std::map<std::string,std::string>& simulationParameters, lm::tiling::Tilings& tilings);
    FFluxTrajectoryList(uint64_t simulationPhase, lm::input::Input& input, lm::message::Communicator& communicator, uint64_t simultaneousTrajectoryCount);
    virtual ~FFluxTrajectoryList();
    virtual void init();
    virtual void initChecks(lm::input::Input& input);
    virtual void initFFluxOutput();
    virtual void initReversed();
    virtual void initTrajectories(uint64_t toStartCount,bool reversed=false);
    virtual void initTrajectories(uint64_t toStartCount, lm::io::TrajectoryState* zerothTraj);
    virtual void initPhaseNTrajectories(uint64_t trajectoriesToStart);

    virtual void setLimits();

    virtual void workUnitPartFinished(const lm::message::WorkUnitStatus& wusMsg, lm::trajectory::Trajectory* traj);
    virtual void workUnitPartFinishedPhaseZero(const message::WorkUnitStatus& wusMsg, lm::fflux::FFluxTrajectory* traj, int prevFinalLimitID, double prevTime);
    virtual void workUnitPartFinishedPhaseN(const message::WorkUnitStatus& wusMsg, lm::fflux::FFluxTrajectory* traj, int prevFinalLimitID, double prevTime);

    // getters
    virtual CrossingVector getCrossings(uint64_t ffluxPhase);
    virtual uint getCrossingsPerPhase();
    virtual lm::io::FFluxOutput* getFFluxOutput();
    virtual lm::io::FFluxOutput* getFFluxOutputStreaming();
    virtual lm::io::TrajectoryState* getRandomCrossing(uint64_t ffluxPhase);        // Returns a randomly chosen crossing event (in the form of a TrajectoryState) collected durring forward flux phase ffluxPhase
//    virtual lm::trajectory::Trajectory* getTrajectoryForFinishedWorkUnit(uint64_t id);          // same as the parent class method but does an exists check to handle the case that we're trying to get a trajectory from a finished phase
    virtual CrossingsMap getSavedCrossings(lm::fflux::FFluxTrajectoryList::Direction dir);

protected:
    typedef std::map<lm::fflux::FFluxTrajectoryList::Direction, CrossingsMap> SavedCrossings;
    typedef std::map<lm::fflux::FFluxTrajectoryList::Direction, DwellTimeMap> SavedDwellTimes;
    typedef std::map<lm::fflux::FFluxTrajectoryList::Direction, FinishedTrajectoriesCountMap> SavedFinishedTrajectoriesCounts;
    typedef std::map<lm::fflux::FFluxTrajectoryList::Direction, lm::io::TilingHist*> SavedHists;

    // methods that encapsulate workUnitFinished inner loop tasks
    virtual void addCrossing(const message::WorkUnitStatus& wusMsg);
    virtual void incrementFFluxPhase();
    virtual bool isFFluxDone();
    virtual bool isPhaseDoneN(double simTime);
    virtual bool isPhaseDoneZero(double simTime);
    virtual bool isPhaseZero();
    virtual void reduceTilingHist(const lm::io::TilingHist& tHist);
    virtual void restart();
    virtual void reverse();
    virtual void saveCrossings();
    virtual void saveDwellTimes();
    virtual void saveFinishedTrajectoriesCounts();

    // methods related to fflux data output
    virtual void ffluxOutputAddBasin(CrossingsMap& crossings, DwellTimeMap& dwellTimes, FinishedTrajectoriesCountMap& finishedTrajectoriesCounts);
    virtual void ffluxOutputAddTrajectory(FFluxTrajectory* traj, lm::io::FFluxOutput::Lifecycle lifecycle);
    virtual void ffluxOutputAddTrajectory(const lm::io::SpeciesCounts& specCountsMsg, lm::io::FFluxOutput::Lifecycle lifecycle);
    virtual void ffluxOutputAddTrajectory(const lm::io::SpeciesTimeSeries& specTimeSeriesMsg, lm::io::FFluxOutput::Lifecycle lifecycle);
    virtual void ffluxOutputFinishTrajectory();
    virtual void ffluxOutputPrintBasin(CrossingsMap& crossings, FinishedTrajectoriesCountMap& finishedTrajectoriesCounts);
    virtual void ffluxOutputPrintFinal(SavedCrossings& savedCrossings, SavedDwellTimes& savedDwellTimes, SavedFinishedTrajectoriesCounts& savedFinishedTrajectoriesCounts, SavedHists& savedHists);
    virtual void ffluxOutputSetFinal(SavedCrossings& savedCrossings, SavedDwellTimes& savedDwellTimes, SavedFinishedTrajectoriesCounts& savedFinishedTrajectoriesCounts, SavedHists& savedHists);

protected:
    const lm::message::Communicator& communicator;
    Direction direction;
    // for printing the name of the current simulation direction
    static const std::vector<std::string> directionStrings;
    uint64_t ffluxPhase;
    lm::input::Input& input;
    uint64_t maxFFluxPhase;
    uint64_t simultaneousTrajectoryCount;
    uint64_t trajectoryCount;
    lm::rng::XORShift xorShift; //RNG used for randomly choosing a crossing in a crossing vector

//    // members that hold trajectory data used for calculation of the forward flux during phase 0
//    std::map<uint,uint> phaseZeroCrossings;
//    std::map<uint,double> phaseZeroTimes;

    // members that hold the trajectory data used for the calculations at the end of fflux
    lm::io::TilingHist averageTilingHist;
    CrossingsMap crossings;
    DwellTimeMap dwellTimes;
    FinishedTrajectoriesCountMap finishedTrajectoriesCounts;

    SavedCrossings savedCrossings;
    SavedDwellTimes savedDwellTimes;
    SavedFinishedTrajectoriesCounts savedFinishedTrajectoriesCounts;
    SavedHists savedHists;

    // user defined parameters that determine how the forward flux sampling is carried out
    unsigned maxCrossingsZero;
    double maxTimeZero;
    unsigned maxCrossingsN; //the count of crossing events that should be collected for every fflux sampling phase
    double maxTimeN;

    PhaseCheck checkZero;
    PhaseCheck checkN;

    // Messages used to send the large-ish FFluxOutput at the end of the simulation and to stream fflux TrajectoryOutput messages as the simulation runs
    lm::message::Message msg;
    lm::message::Message msgStreaming;

    // tuning parameter for setting how much fflux data has to accumulate before it is sent off to the master output. normally this is dynamically adjusted
    int ffluxOutputQueueSize;
};

}
}
#endif
