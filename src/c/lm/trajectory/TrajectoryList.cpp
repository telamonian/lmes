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
#include <list>
#include <map>
#include <stdexcept>
#include <string>

#include "lm/EnumHelper.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/Print.h"
#include "lm/protowrap/Repeated.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/trajectory/TrajectoryList.h"
#include "lm/Types.h"

using lm::input::DiffusionModel;
using lm::input::ReactionModel;
using lm::protowrap::Repeated;
using std::map;
using std::string;

namespace lm {
namespace trajectory {

TrajectoryList::TrajectoryList(): count(0), simulationPhaseIndex(0)
{
}

TrajectoryList::TrajectoryList(const lm::input::SimulationPhase& phase): count(0), simulationPhaseIndex(phase.id())
{
    init(phase.trajectory_states());
}

TrajectoryList::TrajectoryList(const lm::input::SimulationPhase& phase, const TrajectoryList& previousList): count(previousList.count), simulationPhaseIndex(phase.id())
{
    init(previousList);
}

TrajectoryList::~TrajectoryList()
{
    deleteAllTrajectories();
}

// initializer
void TrajectoryList::init(const Repeated<lm::io::TrajectoryState>::GoogleT& initialStates)
{
    for (Repeated<lm::io::TrajectoryState>::const_iterator it=initialStates.begin(); it!=initialStates.end(); it++)
    {
        uint64_t id = count++;
        trajectories[id] = initTrajectory(id, getSimulationPhaseIndex(), *it);
        waitingTrajectories[id] = trajectories[id];
    }
}

void TrajectoryList::init(const TrajectoryList& previousList)
{
    for (TrajectoryMap::const_iterator it=previousList.finishedTrajectories.begin(); it!=previousList.finishedTrajectories.end(); it++)
    {
        uint64_t id = count++;
        trajectories[id] = initTrajectory(id, getSimulationPhaseIndex(), it->second->getState());
        waitingTrajectories[id] = trajectories[id];
    }
}

Trajectory* TrajectoryList::initTrajectory(uint64_t id, uint64_t phase, const lm::io::TrajectoryState& initialState)
{
    return new Trajectory(initialState, id, phase);
}

// destroyer
void TrajectoryList::deleteAllNotStarted()
{
    for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        if (it->second->getStatus()==Trajectory::NOT_STARTED)
        {
            deleteTrajectory(it->second->getID());
        }
    }
}

void TrajectoryList::deleteAllTrajectories()
{
    for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        delete it->second;
        it->second = NULL;
    }
    trajectories.clear();
    abortedTrajectories.clear();
    finishedTrajectories.clear();
    runningTrajectories.clear();
    waitingTrajectories.clear();
}

void TrajectoryList::deleteTrajectory(uint64_t id)
{
    if (trajectories.count(id))
    {
        delete trajectories[id];
        trajectories[id] = NULL;
        trajectories.erase(id);
        abortedTrajectories.erase(id);
        finishedTrajectories.erase(id);
        runningTrajectories.erase(id);
        waitingTrajectories.erase(id);
    }
}

// accessors
bool TrajectoryList::areAllFinished() const
{
    return abortedTrajectories.size() == 0 && runningTrajectories.size() == 0 && waitingTrajectories.size() == 0;
}

const TrajectoryMap& TrajectoryList::getTrajectoryMap(Trajectory::Status status) const
{
    switch (status)
    {
    case Trajectory::ABORTED: return abortedTrajectories;
    case Trajectory::FINISHED: return finishedTrajectories;
    case Trajectory::NOT_STARTED: throw Exception("unimplemented");
    case Trajectory::RUNNING: return runningTrajectories;
    case Trajectory::WAITING: return waitingTrajectories;
    }
    throw Exception("Unknown Trajectory Status", status);
}

// mutators
int TrajectoryList::addWorkUnitParts(uint64_t workUnitId, lm::message::RunWorkUnit* msg, uint numberParts)
{
    list<uint64_t> trajectoriesAdded;
    for (int i=0; i<numberParts; i++)
    {
        // See if any trajectories are waiting.
        if (waitingTrajectories.size() > 0)
        {
            // Get the first trajectory.
            uint64_t id = findNextTrajectoryToRun();
            if (!waitingTrajectories.count(id))
                throw Exception("Consistency error in trajectory list, next trajectory to run was not in the waiting list",id);
            Trajectory* t = waitingTrajectories[id];

            // Validate that it really needs to be run.
            if (t->getStatus() != Trajectory::NOT_STARTED && t->getStatus() != Trajectory::WAITING)
                throw Exception("Consistency error in trajectory list, invalid trajectory in the waiting list",id,t->getStatus());

            // Move it to the running list.
            trajectoriesAdded.push_back(id);
            setTrajectoryRunning(t);

            // Fill in the message.
            lm::message::WorkUnit* wu = msg->add_part();
            wu->mutable_initial_state()->CopyFrom(t->getState());
        }
    }

    // Add these trajectories to the work units running map.
    workUnitsRunning[workUnitId] = trajectoriesAdded;

    // Print some performance statistics, if it has been a while.
    printTrajectoryStatistics();

    return trajectoriesAdded.size();
}

void TrajectoryList::copyTrajectories(const TrajectoryList& srcTrajList, Trajectory::Status status)
{
    TrajectoryMap* dstMap = getTrajectoryMap(status);
    const TrajectoryMap& srcMap = srcTrajList.getTrajectoryMap(status);

    for (TrajectoryMap::const_iterator it=srcMap.begin();it!=srcMap.end();it++)
    {
        dstMap[it->first] = it->second;
    }
}

Trajectory* TrajectoryList::getTrajectoryForFinishedWorkUnit(uint64_t id)
{
    Trajectory* t;
    if (runningTrajectories.count(id) == 1)
    {
        t = runningTrajectories[id];
        if (t->getStatus() != Trajectory::RUNNING)
        {
            throw Exception("Consistency error in trajectory list, expected trajectory did not have a running status", id);
        }
    }
    else if (abortedTrajectories.count(id) == 1)
    {
        t = abortedTrajectories[id];
        if (t->getStatus() != Trajectory::ABORTED)
        {
            throw Exception("Consistency error in trajectory list, expected trajectory did not have a running status", id);
        }
    }
    else
    {
        throw Exception("Consistency error in trajectory list, expected trajectory not in the aborted or running list", id);
    }
    return t;
}

void TrajectoryList::incrementSimulationPhase()
{
    simulationPhaseIndex++;
}

TrajectoryMap* TrajectoryList::getTrajectoryMap(Trajectory::Status status)
{
    return const_cast<TrajectoryMap*>(const_cast<const TrajectoryList*>(this)->getTrajectoryMap(status));
}

void TrajectoryList::setAll(Trajectory::Status oldStatus, Trajectory::Status newStatus)
{
    TrajectoryMap& oldMap = *getTrajectoryMap(oldStatus);
    TrajectoryMap& newMap = *getTrajectoryMap(newStatus);
    for (TrajectoryMap::iterator it=oldMap.begin(); it!=oldMap.end(); it++)
    {
        it->second->setStatus(newStatus);
        newMap[it->first] = it->second;
    }
    oldMap.clear();
}

void TrajectoryList::takeTrajectories(TrajectoryList* srcTrajList, Trajectory::Status status)
{
    copyTrajectories(*srcTrajList, status);
    TrajectoryMap* srcMap = srcTrajList->getTrajectoryMap(status);

    for (TrajectoryMap::iterator it=srcMap->begin();it!=srcMap->end();it++)
    {
        srcMap->erase(it);
    }
}

void TrajectoryList::workUnitPartFinished(const lm::message::WorkUnitStatus& wusBuf, lm::trajectory::Trajectory* traj)
{
    uint64_t id = wusBuf.final_state().trajectory_id();

    // Update the state of the trajectory.
    traj->setState(wusBuf.final_state());
    traj->incrementWorkUnitsPerformed();

    // Update the status of the trajectory and move it to the appropriate internal TrajectoryMap
    if (wusBuf.status() == lm::message::WorkUnitStatus::STEPS_FINISHED)
    {
        setTrajectoryWaiting(traj);
    }
    else if (wusBuf.status() == lm::message::WorkUnitStatus::LIMIT_REACHED)
    {
        setTrajectoryFinished(traj);
    }
    else if (wusBuf.status() == lm::message::WorkUnitStatus::ERROR)
    {
        if (wusBuf.has_error_message())
            throw Exception("Error received in work unit status",wusBuf.error_message().c_str());
        else
            throw Exception("Error received in work unit status","<no error message specified");
    }
    else
    {
        throw Exception("Unknown work unit status", wusBuf.status());
    }
}

void TrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit& fwuMsg)
{
    // Get the work unit id.
    uint64_t workUnitId = fwuMsg.work_unit_id();

    // Get the list of trajectories associated with this work unit.
    list<uint64_t> involvedTrajectories;
    try
    {
        involvedTrajectories = workUnitsRunning.at(workUnitId);
    }
    catch (std::out_of_range e)
    {
        throw Exception("ID of finished work unit not found in the list of running work units: id", workUnitId);
    }
    workUnitsRunning.erase(workUnitId);

    //Make sure the sizes between the list and the message are consistent.
    if (involvedTrajectories.size() != fwuMsg.part_status_size())
        throw Exception("Consistency error in trajectory list, number of involved trajectories differed from work units finished message: id, involved trajectories, work unit trajectories", workUnitId, involvedTrajectories.size(), fwuMsg.part_status_size());

    // Loop over the trajectories.
    for (list<uint64_t>::iterator it=involvedTrajectories.begin(); it != involvedTrajectories.end(); it++)
    {
        uint64_t id = *it;

        // normally the trajectory associated with the id has to still exist at this point or an exception is thrown. The two lines below are a hook that allows subclasses to override this behavior.
        Trajectory* t = getTrajectoryForFinishedWorkUnit(id);
        if (t->getStatus()==Trajectory::ABORTED) continue;

        // Find the trajectory in the message.
        int partIndex=-1;
        for (int i=0; i<fwuMsg.part_status_size(); i++)
        {
            if (fwuMsg.part_status(i).final_state().trajectory_id() == id)
            {
                partIndex = i;
                break;
            }
        }
        if (partIndex == -1)
            throw Exception("Consistency error in trajectory list, could not find trajectory id in work units finished",id);

        workUnitPartFinished(fwuMsg.part_status(partIndex), t);
    }

    // check to see if any of the trajectories involved in this work unit have been aborted in the previous loop
    for (list<uint64_t>::iterator it=involvedTrajectories.begin(); it != involvedTrajectories.end(); it++)
    {
        if (abortedTrajectories.count(*it) == 1)
        {
            deleteTrajectory(*it);
        }
    }
}

// protected accessors
uint64_t TrajectoryList::findNextTrajectoryToRun() const
{
    TrajectoryMap::const_iterator it = waitingTrajectories.begin();
    return it->first;
}

bool TrajectoryList::isTrajectoryInMap(lm::trajectory::Trajectory* traj, const TrajectoryMap& trajMap, Trajectory::Status expectedStatus) const
{
    bool exists(trajMap.count(traj->getID())==1);
    if (exists && traj->getStatus()!=expectedStatus) throw ConsistencyException("trajectory found in list that does not match its status: id, status, list_status", traj->getID(), Trajectory::status_strings[traj->getStatus()].c_str(), Trajectory::status_strings[expectedStatus].c_str());
    return exists;
}

// protected mutators
void TrajectoryList::setTrajectoryStatus(lm::trajectory::Trajectory* traj, TrajectoryMap& trajMap, Trajectory::Status newStatus)
{
    traj->setStatus(newStatus);

    uint64_t id = traj->getID();
    if (abortedTrajectories.count(id)) abortedTrajectories.erase(id);
    if (finishedTrajectories.count(id)) finishedTrajectories.erase(id);
    if (runningTrajectories.count(id)) runningTrajectories.erase(id);
    if (waitingTrajectories.count(id)) waitingTrajectories.erase(id);
    trajMap[id] = traj;
}


}
}
