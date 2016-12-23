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

#include <map>
#include <string>

#include "lm/input/Input.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/Types.h"

using std::map;
using std::string;

typedef std::map<uint64_t,lm::trajectory::Trajectory*> TrajectoryMap;

namespace lm {
namespace trajectory {

class TrajectoryList
{
public:
    TrajectoryList();
    TrajectoryList(uint64_t simulationPhase);
    virtual ~TrajectoryList();

// destroyer
    virtual void deleteAllNotStarted();
    virtual void deleteTrajectory(uint64_t trajectoryID);
    virtual void deleteAllTrajectories();

// accessors
    virtual bool areAllFinished() const;
    virtual bool areAnyWaiting() const;
    virtual bool exists(uint64_t id) const {return trajectories.count(id)==1;}
    virtual uint64_t getSimulationPhase() const {return simulationPhase;}
    virtual bool isTrajectoryAborted(lm::trajectory::Trajectory* traj);
    virtual bool isTrajectoryFinished(lm::trajectory::Trajectory* traj);
    virtual bool isTrajectoryRunning(lm::trajectory::Trajectory* traj);
    virtual bool isTrajectoryWaiting(lm::trajectory::Trajectory* traj);
    virtual size_t size() const {return trajectories.size();}

// mutators
    virtual int addWorkUnitParts(uint64_t workUnitId, lm::message::RunWorkUnit* msg, uint numberParts);
    virtual Trajectory* getTrajectoryForFinishedWorkUnit(uint64_t id);
    virtual void incrementSimulationPhase();
    virtual TrajectoryMap* mutableTrajectoryMapFromStatus(Trajectory::status_t status);
    virtual void setSimulationPhase(uint64_t newPhase) {simulationPhase = newPhase;}
    virtual void setAll(Trajectory::status_t oldStatus, Trajectory::status_t newStatus);
    virtual void restartFinishedTrajectories();
    virtual void setTrajectoryAborted(lm::trajectory::Trajectory* traj);
    virtual void setTrajectoryFinished(lm::trajectory::Trajectory* traj);
    virtual void setTrajectoryRunning(lm::trajectory::Trajectory* traj);
    virtual void setTrajectoryWaiting(lm::trajectory::Trajectory* traj);
    virtual void workUnitFinished(const lm::message::FinishedWorkUnit& fwuMsg);
    virtual void workUnitPartFinished(const lm::message::WorkUnitStatus& wusBuf, lm::trajectory::Trajectory* traj);

protected:
    virtual uint64_t findNextTrajectoryToRun() const;
    virtual void printTrajectoryStatistics() const {};

protected:
    uint64_t simulationPhase;
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
