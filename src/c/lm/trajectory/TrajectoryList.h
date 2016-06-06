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

#include <map>
#include <string>

#include "lm/input/Input.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationPhase.pb.h"
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

//template <typename T> class _TrajectoryStateIteratorBase : public std::iterator<std::forward_iterator_tag, lm::io::TrajectoryState>
//{
//public:
//    _TrajectoryStateIteratorBase(T tmit): tmit(tmit) {}
//    _TrajectoryStateIteratorBase(const _TrajectoryStateIteratorBase& tsit): tmit(tsit.tmit) {}
//    _TrajectoryStateIteratorBase& operator++() {++tmit;return *this;}
//    _TrajectoryStateIteratorBase operator++(int) {_TrajectoryStateIteratorBase tmp(*this); operator++(); return tmp;}
//    bool operator==(const _TrajectoryStateIteratorBase& rhs) {return tmit==rhs.tmit;}
//    bool operator!=(const _TrajectoryStateIteratorBase& rhs) {return tmit!=rhs.tmit;}
//protected:
//    T tmit;
//};
//template <typename T> class _TrajectoryStateIterator : public _TrajectoryStateIteratorBase<T>
//{
//public:
//    _TrajectoryStateIterator(T tmit): _TrajectoryStateIteratorBase(tmit) {}
//    _TrajectoryStateIterator(const _TrajectoryStateIterator& tsit): _TrajectoryStateIteratorBase(tsit.tmit) {}
//    lm::io::TrajectoryState& operator*() {return *tmit->second->getStateMutable();}
//};
//template <typename T> class _TrajectoryStateConstIterator : public _TrajectoryStateIteratorBase<T>
//{
//public:
//    _TrajectoryStateConstIterator(T tmit): _TrajectoryStateIteratorBase(tmit) {}
//    _TrajectoryStateConstIterator(const _TrajectoryStateConstIterator& tsit): _TrajectoryStateIteratorBase(tsit.tmit) {}
//    lm::io::TrajectoryState& operator*() {return tmit->second->getState();}
//};
//typedef _TrajectoryStateIterator<TrajectoryMap::iterator> TrajectoryStateIterator;
//typedef _TrajectoryStateConstIterator<TrajectoryMap::const_iterator> TrajectoryStateConstIterator;

class TrajectoryList
{
public:
    
    TrajectoryList();
    TrajectoryList(const lm::io::SimulationPhase& phase);
    TrajectoryList(const lm::io::SimulationPhase& phase, const TrajectoryList& previousList);
    virtual ~TrajectoryList();

// initializer
    virtual void init(const lm::protowrap::Repeated<lm::io::TrajectoryState>::type& initialStates);
    virtual void init(const TrajectoryList& previousList);
    virtual Trajectory* initTrajectory(uint64_t id, uint64_t phase, const lm::io::TrajectoryState& initialState);

// destroyer
    virtual void deleteAllNotStarted();
    virtual void deleteTrajectory(uint64_t trajectoryID);
    virtual void deleteAllTrajectories();

// accessors
    virtual bool areAllFinished() const;
    virtual bool exists(uint64_t id) const {return trajectories.count(id)==1;}
    virtual uint64_t getSimulationPhase() const {return simulationPhase;}
    virtual bool isTrajectoryAborted(lm::trajectory::Trajectory* traj) const {return isTrajectoryInMap(traj, abortedTrajectories, Trajectory::ABORTED);}
    virtual bool isTrajectoryFinished(lm::trajectory::Trajectory* traj) const {return isTrajectoryInMap(traj, finishedTrajectories, Trajectory::FINISHED);}
    virtual bool isTrajectoryRunning(lm::trajectory::Trajectory* traj) const {return isTrajectoryInMap(traj, runningTrajectories, Trajectory::RUNNING);}
    virtual bool isTrajectoryWaiting(lm::trajectory::Trajectory* traj) const {return isTrajectoryInMap(traj, waitingTrajectories, Trajectory::WAITING);}
    virtual size_t size() const {return trajectories.size();}

// mutators
    virtual int addWorkUnitParts(uint64_t workUnitId, lm::message::RunWorkUnit* msg, uint numberParts);
    virtual Trajectory* getTrajectoryForFinishedWorkUnit(uint64_t id);
    virtual void incrementSimulationPhase();
    virtual TrajectoryMap* mutableTrajectoryMapFromStatus(Trajectory::status_t status);
    virtual void setSimulationPhase(uint64_t newPhase) {simulationPhase = newPhase;}
    virtual void setAll(Trajectory::status_t oldStatus, Trajectory::status_t newStatus);
    virtual void setTrajectoryAborted(lm::trajectory::Trajectory* traj) {setTrajectoryStatus(traj, abortedTrajectories, Trajectory::ABORTED);}
    virtual void setTrajectoryFinished(lm::trajectory::Trajectory* traj) {setTrajectoryStatus(traj, finishedTrajectories, Trajectory::FINISHED);}
    virtual void setTrajectoryRunning(lm::trajectory::Trajectory* traj) {setTrajectoryStatus(traj, runningTrajectories, Trajectory::RUNNING);}
    virtual void setTrajectoryWaiting(lm::trajectory::Trajectory* traj) {setTrajectoryStatus(traj, waitingTrajectories, Trajectory::WAITING);}
    virtual void workUnitFinished(const lm::message::FinishedWorkUnit& fwuMsg);
    virtual void workUnitPartFinished(const lm::message::WorkUnitStatus& wusBuf, lm::trajectory::Trajectory* traj);

protected:
// accessors
    virtual bool isTrajectoryInMap(lm::trajectory::Trajectory* traj, const TrajectoryMap& trajMap, Trajectory::status_t expectedStatus) const;
    virtual uint64_t findNextTrajectoryToRun() const;
    virtual void printTrajectoryStatistics() const {};

// mutators
    virtual void setTrajectoryStatus(lm::trajectory::Trajectory* traj, TrajectoryMap& trajMap, Trajectory::status_t newStatus);

protected:
    uint64_t count;
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
