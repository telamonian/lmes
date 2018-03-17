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
#ifndef LM_TRAJECTORY_TRAJECTORYLIST_H_
#define LM_TRAJECTORY_TRAJECTORYLIST_H_

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

namespace lm {
namespace trajectory {

class TrajectoryList
{
public:
    typedef std::map<uint64_t,Trajectory*> idmap;
    typedef lm::unordered_set<uint64_t>::type idset;

public:
    TrajectoryList();
    TrajectoryList(uint64_t count, uint64_t simulationPhaseID);
    virtual ~TrajectoryList();

// initializer
    virtual void init(const TrajectoryStates& initialStates);
    virtual void init(const TrajectoryList& previousList);
    // add pre-constructed Trajectory to the internal list
    virtual Trajectory* initTrajectory(Trajectory* allocatedTrajectory);
    // construct Trajectory from Input and add it to the internal list
    virtual Trajectory* initTrajectory(const lm::input::Input& input, uint64_t phase, uint64_t id=DEFAULT_TRAJECTORY_ID);
    // construct Trajectory from a pre-existing TrajectoryState and add it to the internal list
    virtual Trajectory* initTrajectory(const lm::io::TrajectoryState& initialState, uint64_t phase, uint64_t id=DEFAULT_TRAJECTORY_ID);
    // construct Trajectory from a range of species count values (and optionally a starting time) and add it to the internal list
    template <typename InputIterator> Trajectory* initTrajectory(const lm::input::Input& input, InputIterator speciesStart, InputIterator speciesEnd, double startTime, uint64_t phase, uint64_t id=DEFAULT_TRAJECTORY_ID)
    {
        return initTrajectory(new Trajectory(input, speciesStart, speciesEnd, startTime, phase, resolveTrajectoryID(id)));
    }

// destroyer
    virtual void deleteAllNotStarted();
    virtual void deleteTrajectory(uint64_t trajectoryID);
    virtual void deleteAllTrajectories();

// accessors
    virtual bool allFinished() const {return not (anyAborted() or anyRunning() or anyWaiting());}
    virtual bool anyAborted() const {return not abortedTrajectories.empty();}
    virtual bool anyRunning() const {return not runningTrajectories.empty();}
    virtual bool anyWaiting() const {return not waitingTrajectories.empty();}
    virtual uint64_t count() const {return _count;}
    virtual bool exists(uint64_t id) const {return trajectories.count(id) != 0;}
    virtual const idset& getIDSet(Trajectory::Status status) const;
    virtual bool isTrajectoryAborted(uint64_t trajID) const {return abortedTrajectories.count(trajID) != 0;}
    virtual bool isTrajectoryFinished(uint64_t trajID) const {return finishedTrajectories.count(trajID) != 0;}
    virtual bool isTrajectoryRunning(uint64_t trajID) const {return runningTrajectories.count(trajID) != 0;}
    virtual bool isTrajectoryWaiting(uint64_t trajID) const {return waitingTrajectories.count(trajID) != 0;}
    virtual bool isTrajectoryAborted(Trajectory* traj) const {return abortedTrajectories.count(traj->getID()) != 0;}
    virtual bool isTrajectoryFinished(Trajectory* traj) const {return finishedTrajectories.count(traj->getID()) != 0;}
    virtual bool isTrajectoryRunning(Trajectory* traj) const {return runningTrajectories.count(traj->getID()) != 0;}
    virtual bool isTrajectoryWaiting(Trajectory* traj) const {return waitingTrajectories.count(traj->getID()) != 0;}
    virtual uint64_t simulationPhaseID() const {return _simulationPhaseID;}
    virtual size_t size() const {return trajectories.size();}

// mutators
    virtual int addWorkUnitParts(uint64_t workUnitId, lm::message::RunWorkUnit* msg, uint64_t numberParts);
    virtual void copyTrajectoriesWeakly(const TrajectoryList& srcTrajList, Trajectory::Status status);
    virtual void copyWorkUnitsRunning(const TrajectoryList& srcTrajList);
    virtual idset* getIDSet(Trajectory::Status status);
    virtual Trajectory* getTrajectoryForFinishedWorkUnit(uint64_t id);
    template <typename InputIterator> Trajectory* recycleTrajectory(InputIterator speciesStart, InputIterator speciesEnd, double startTime, uint64_t oldID, uint64_t newID)
    {
        Trajectory* traj = trajectories[oldID];
        newID = resolveTrajectoryID(newID);

        setTrajectoryID(traj, newID, Trajectory::NOT_STARTED);
        traj->recycle(speciesStart, speciesEnd, startTime, newID);

        return traj;
    }
    virtual uint64_t resolveTrajectoryID(uint64_t newID);
    virtual void restartFinishedTrajectories();
    virtual void setAll(Trajectory::Status oldStatus, Trajectory::Status newStatus);
    virtual void takeTrajectories(TrajectoryList* srcTrajList, Trajectory::Status status, Trajectory::Status newStatus);
    virtual void takeWorkUnitsRunning(TrajectoryList* srcTrajList);
    virtual void workUnitFinished(const lm::message::FinishedWorkUnit& fwuMsg);
    virtual void workUnitPartFinished(const lm::message::WorkUnitStatus& wusBuf, lm::trajectory::Trajectory* traj);

protected:
// accessors
    virtual uint64_t findNextTrajectoryToRun() const;
    virtual void printTrajectoryStatistics() const {};

// mutators
    virtual Trajectory* eraseTrajectoryID(uint64_t id);
    virtual Trajectory* eraseTrajectoryIDFromSublists(uint64_t id);
    virtual void setTrajectoryID(Trajectory* traj, uint64_t newID, Trajectory::Status newStatus);
    virtual void setTrajectoryStatus(Trajectory* traj, Trajectory::Status newStatus);

public:
    // constants
    static const uint64_t DEFAULT_TRAJECTORY_ID;
    
protected:
    uint64_t _count;
    uint64_t _simulationPhaseID;
    
    idmap trajectories;
    idset abortedTrajectories;
    idset finishedTrajectories;
    idset runningTrajectories;
    idset waitingTrajectories;
    std::map<uint64_t,std::list<uint64_t> > workUnitsRunning;
};

}
}

#endif
