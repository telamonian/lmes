/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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
#ifndef LM_TRAJECTORY_TRAJECTORYLIST_H
#define LM_TRAJECTORY_TRAJECTORYLIST_H

#include <limits>
#include <map>
#include <string>

#include "lm/input/Input.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/protowrap/Repeated.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/Types.h"

using std::map;
using std::string;

namespace lm {
namespace trajectory {

typedef std::map<uint64_t,lm::trajectory::Trajectory*> TrajectoryMap;

class TrajectoryList
{
public:
// constants
    static const uint64_t DEFAULT_TRAJECTORY_ID;

    TrajectoryList();
    TrajectoryList(uint64_t count, uint64_t simulationPhaseIndex);
    virtual ~TrajectoryList();

// initializer
    virtual void init(const TrajectoryStates& initialStates);
    virtual void init(const TrajectoryList& previousList);
    virtual Trajectory* initTrajectory(Trajectory* allocatedTrajectory);
    virtual Trajectory* initTrajectory(const lm::input::Input&, uint64_t phase, uint64_t id=DEFAULT_TRAJECTORY_ID);
    virtual Trajectory* initTrajectory(const lm::io::TrajectoryState& initialState, uint64_t phase, uint64_t id=DEFAULT_TRAJECTORY_ID);
    // construct Trajectory from a range of species count values (and optionally a starting time)
    template <typename InputIterator> Trajectory* initTrajectory(const lm::input::Input& input, InputIterator speciesStart, InputIterator speciesEnd, double startTime, uint64_t phase, uint64_t id=DEFAULT_TRAJECTORY_ID)
    {
        initTrajectory(new Trajectory(input, speciesEnd, speciesStart, startTime, phase, id));
    }

// destroyer
    virtual void deleteAllNotStarted();
    virtual void deleteTrajectory(uint64_t trajectoryID);
    virtual void deleteAllTrajectories();

// accessors
    virtual bool areAllFinished() const;
    virtual uint64_t count() const {return _count;}
    virtual bool exists(uint64_t id) const {return trajectories.count(id)==1;}
    virtual const TrajectoryMap& getTrajectoryMap(Trajectory::Status status) const;
    virtual bool isTrajectoryAborted(lm::trajectory::Trajectory* traj) const {return isTrajectoryInMap(traj, abortedTrajectories, Trajectory::ABORTED);}
    virtual bool isTrajectoryFinished(lm::trajectory::Trajectory* traj) const {return isTrajectoryInMap(traj, finishedTrajectories, Trajectory::FINISHED);}
    virtual bool isTrajectoryRunning(lm::trajectory::Trajectory* traj) const {return isTrajectoryInMap(traj, runningTrajectories, Trajectory::RUNNING);}
    virtual bool isTrajectoryWaiting(lm::trajectory::Trajectory* traj) const {return isTrajectoryInMap(traj, waitingTrajectories, Trajectory::WAITING);}
    virtual uint64_t simulationPhaseIndex() const {return _simulationPhaseIndex;}
    virtual size_t size() const {return trajectories.size();}

// mutators
    virtual int addWorkUnitParts(uint64_t workUnitId, lm::message::RunWorkUnit* msg, uint numberParts);
    virtual void copyTrajectories(const TrajectoryList& srcTrajList, Trajectory::Status status);
    virtual Trajectory* getTrajectoryForFinishedWorkUnit(uint64_t id);
    virtual TrajectoryMap* getTrajectoryMap(Trajectory::Status status);
    virtual void setSimulationPhaseIndex(uint64_t newPhaseIx) {_simulationPhaseIndex = newPhaseIx;}
    virtual void setAll(Trajectory::Status oldStatus, Trajectory::Status newStatus);
    virtual void setTrajectoryAborted(lm::trajectory::Trajectory* traj) {setTrajectoryStatus(traj, abortedTrajectories, Trajectory::ABORTED);}
    virtual void setTrajectoryFinished(lm::trajectory::Trajectory* traj) {setTrajectoryStatus(traj, finishedTrajectories, Trajectory::FINISHED);}
    virtual void setTrajectoryRunning(lm::trajectory::Trajectory* traj) {setTrajectoryStatus(traj, runningTrajectories, Trajectory::RUNNING);}
    virtual void setTrajectoryWaiting(lm::trajectory::Trajectory* traj) {setTrajectoryStatus(traj, waitingTrajectories, Trajectory::WAITING);}
    virtual void takeTrajectories(TrajectoryList* srcTrajList, Trajectory::Status status);
    virtual void workUnitFinished(const lm::message::FinishedWorkUnit& fwuMsg);
    virtual void workUnitPartFinished(const lm::message::WorkUnitStatus& wusBuf, lm::trajectory::Trajectory* traj);

protected:
// accessors
    virtual bool isTrajectoryInMap(lm::trajectory::Trajectory* traj, const TrajectoryMap& trajMap, Trajectory::Status expectedStatus) const;
    virtual uint64_t findNextTrajectoryToRun() const;
    virtual void printTrajectoryStatistics() const {};

// mutators
    virtual void setTrajectoryStatus(lm::trajectory::Trajectory* traj, TrajectoryMap& trajMap, Trajectory::Status newStatus);

protected:
    uint64_t _count;
    uint64_t _simulationPhaseIndex;
    TrajectoryMap trajectories;
    TrajectoryMap abortedTrajectories;
    TrajectoryMap finishedTrajectories;
    TrajectoryMap runningTrajectories;
    TrajectoryMap waitingTrajectories;
    map<uint64_t,list<uint64_t> > workUnitsRunning;
};

}
}

#endif
