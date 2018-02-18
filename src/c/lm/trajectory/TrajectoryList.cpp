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
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using lm::input::DiffusionModel;
using lm::input::ReactionModel;
using lm::protowrap::Repeated;
using std::list;
using std::map;
using std::string;

namespace lm {
namespace trajectory {

const uint64_t TrajectoryList::DEFAULT_TRAJECTORY_ID = std::numeric_limits<uint64_t>::max();

TrajectoryList::TrajectoryList(): _count(0), _simulationPhaseID(0)
{
}

TrajectoryList::TrajectoryList(uint64_t count, uint64_t simulationPhaseID): _count(count), _simulationPhaseID(simulationPhaseID)
{
}

TrajectoryList::~TrajectoryList()
{
    deleteAllTrajectories();
}

// initializer
void TrajectoryList::init(const TrajectoryStates& initialStates)
{
    // find the largest id of the passed-in TrajectoryStates and initialize count to that plus one
    uint64_t previousMaxCount = 0;
    for (TrajectoryStates::const_iterator it=initialStates.begin();it!=initialStates.end();it++)
    {
        if (previousMaxCount < it->trajectory_id()) previousMaxCount = it->trajectory_id();
    }
    _count = previousMaxCount + 1;

    for (Repeated<lm::io::TrajectoryState>::const_iterator it=initialStates.begin(); it!=initialStates.end(); it++)
    {
        initTrajectory(*it, simulationPhaseID());
    }
}

void TrajectoryList::init(const TrajectoryList& previousList)
{
    // set the count of this list to one past the count of the previousList
    _count = previousList._count + 1;

    for (TrajectoryMap::const_iterator it=previousList.finishedTrajectories.begin(); it!=previousList.finishedTrajectories.end(); it++)
    {
        initTrajectory(it->second->getState(), simulationPhaseID());
    }
}

Trajectory* TrajectoryList::initTrajectory(Trajectory* allocatedTrajectory)
{
//    uint64_t oldID = allocatedTrajectory->getID();
//    uint64_t newID = resolveTrajectoryID(oldID);
//    if (oldID!=newID) {allocatedTrajectory->setID(newID);}
    
    uint64_t id = allocatedTrajectory->getID();
    
    trajectories[id] = allocatedTrajectory;
    waitingTrajectories[id] = trajectories[id];

    return trajectories[id];
}

Trajectory* TrajectoryList::initTrajectory(const lm::input::Input& input, uint64_t phase, uint64_t id)
{
    return initTrajectory(new Trajectory(input, phase, resolveTrajectoryID(id)));
}

Trajectory* TrajectoryList::initTrajectory(const lm::io::TrajectoryState& initialState, uint64_t phase, uint64_t id)
{
    return initTrajectory(new Trajectory(initialState, phase, resolveTrajectoryID(id)));
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
        Trajectory* traj = eraseTrajectoryID(id);
        delete traj;

//        delete trajectories[id];
//        trajectories[id] = NULL;
//        trajectories.erase(id);
//        abortedTrajectories.erase(id);
//        finishedTrajectories.erase(id);
//        runningTrajectories.erase(id);
//        waitingTrajectories.erase(id);
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
    case Trajectory::NOT_STARTED: return waitingTrajectories;
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
                throw ConsistencyException("Consistency error in trajectory list, next trajectory to run was not in the waiting list",id);
            Trajectory* t = waitingTrajectories[id];

            // Validate that it really needs to be run.
            if (t->getStatus() != Trajectory::NOT_STARTED && t->getStatus() != Trajectory::WAITING)
                throw ConsistencyException("Consistency error in trajectory list, invalid trajectory in the waiting list",id,t->getStatus());

            // Move it to the running list.
            trajectoriesAdded.push_back(id);
            setTrajectoryStatus(t, Trajectory::RUNNING);

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

void TrajectoryList::copyTrajectoriesWeakly(const TrajectoryList& srcTrajList, Trajectory::Status status)
{
    TrajectoryMap* dstMap = getTrajectoryMap(status);
    const TrajectoryMap& srcMap = srcTrajList.getTrajectoryMap(status);

    for (TrajectoryMap::const_iterator it=srcMap.begin();it!=srcMap.end();it++)
    {
        (*dstMap)[it->first] = it->second;
    }
}

void TrajectoryList::copyWorkUnitsRunning(const TrajectoryList& srcTrajList)
{
    for (map<uint64_t,list<uint64_t> >::const_iterator it=srcTrajList.workUnitsRunning.begin();it!=srcTrajList.workUnitsRunning.end();it++)
    {
        workUnitsRunning[it->first] = it->second;
    }
}

Trajectory* TrajectoryList::eraseTrajectoryID(uint64_t id)
{
    Trajectory* traj = eraseTrajectoryIDFromSublists(id);
    trajectories.erase(id);

    return traj;
}

Trajectory* TrajectoryList::eraseTrajectoryIDFromSublists(uint64_t id)
{
    Trajectory* traj = trajectories[id];

    switch (traj->getStatus())
    {
    case Trajectory::ABORTED: abortedTrajectories.erase(id); break;
    case Trajectory::FINISHED: finishedTrajectories.erase(id); break;
    case Trajectory::NOT_STARTED: waitingTrajectories.erase(id); break;
    case Trajectory::RUNNING: runningTrajectories.erase(id); break;
    case Trajectory::WAITING: waitingTrajectories.erase(id); break;
    }

    return traj;
}

Trajectory* TrajectoryList::getTrajectoryForFinishedWorkUnit(uint64_t id)
{
    Trajectory* t;
    if (runningTrajectories.count(id) == 1)
    {
        t = runningTrajectories[id];
        if (t->getStatus() != Trajectory::RUNNING)
        {
            throw ConsistencyException("Consistency error in trajectory list, trajectory %d found in runningTrajectories map but did not have a running status", id);
        }
    }
    else if (abortedTrajectories.count(id) == 1)
    {
        t = abortedTrajectories[id];
        if (t != NULL and t->getStatus() != Trajectory::ABORTED)
        {
            throw ConsistencyException("Consistency error in trajectory list, trajectory %d found in abortedTrajectories map but did not have an aborted status", id);
        }
    }
    else
    {
        throw ConsistencyException("Consistency error in trajectory list, trajectory %d not in the aborted or running maps", id);
    }
    return t;
}

TrajectoryMap* TrajectoryList::getTrajectoryMap(Trajectory::Status status)
{
    return const_cast<TrajectoryMap*>(&const_cast<const TrajectoryList*>(this)->getTrajectoryMap(status));
}

uint64_t TrajectoryList::resolveTrajectoryID(uint64_t newID)
{
    // if the new Trajectory id has been left as the default, resolve the id to the TrajectoryList's _count attribute and postcrement _count
    if (newID==DEFAULT_TRAJECTORY_ID) {return _count++;}
    else                              {return newID;}
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

void TrajectoryList::takeTrajectories(TrajectoryList* srcTrajList, Trajectory::Status srcStatus, Trajectory::Status newStatus)
{
    TrajectoryMap* dstMap = getTrajectoryMap(newStatus);
    TrajectoryMap* srcMap = srcTrajList->getTrajectoryMap(srcStatus);

    // By copying the trajectory pointers into this instance's primary trajectories map (and by erasing it from the src's trajectories) we have taken ownership of the pointed-to-trajectories' memory
    TrajectoryMap::iterator it=srcMap->begin();
    while (it!=srcMap->end())
    {
        trajectories[it->first] = it->second;
        (*dstMap)[it->first] = it->second;
        it->second->setStatus(newStatus);

        srcTrajList->trajectories.erase(it->first);
        // erasing the map entry invalidates the iterator, so increment before erasing (via postcrement, which is confusing)
        srcMap->erase(it++);
    }
}

void TrajectoryList::takeWorkUnitsRunning(TrajectoryList* srcTrajList){
    copyWorkUnitsRunning(*srcTrajList);
    srcTrajList->workUnitsRunning.clear();
}

void TrajectoryList::workUnitPartFinished(const lm::message::WorkUnitStatus& wusBuf, lm::trajectory::Trajectory* traj)
{
    // Update the state of the trajectory.
    traj->setState(wusBuf.final_state());
    traj->incrementWorkUnitsPerformed();

    // Update the status of the trajectory and move it to the appropriate internal TrajectoryMap
    if (wusBuf.status() == lm::message::WorkUnitStatus::STEPS_FINISHED)
    {
        setTrajectoryStatus(traj, Trajectory::WAITING);
    }
    else if (wusBuf.status() == lm::message::WorkUnitStatus::LIMIT_REACHED)
    {
        setTrajectoryStatus(traj, Trajectory::FINISHED);
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
        throw ConsistencyException("ID of finished work unit not found in the list of running work units: id", workUnitId);
    }
    workUnitsRunning.erase(workUnitId);

    //Make sure the sizes between the list and the message are consistent.
    if (involvedTrajectories.size() != fwuMsg.part_status_size())
        throw ConsistencyException("Consistency error in trajectory list, number of involved trajectories differed from work units finished message: id, involved trajectories, work unit trajectories", workUnitId, involvedTrajectories.size(), fwuMsg.part_status_size());

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
//            printf("id in fwu: %d\n", fwuMsg.part_status(i).final_state().trajectory_id());
            if (fwuMsg.part_status(i).final_state().trajectory_id() == id)
            {
                partIndex = i;
                break;
            }
        }
        if (partIndex == -1)
            throw ConsistencyException("Consistency error in trajectory list, trajectory id: %d was part of work unit id: %d, but was not found in corresponding FinishedWorkUnit message", id, workUnitId);

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

bool TrajectoryList::isTrajectoryInMap(uint64_t trajID, const TrajectoryMap& trajMap, Trajectory::Status expectedStatus) const
{
    bool exists(trajMap.count(trajID)==1);
    if (exists && trajMap.at(trajID)->getStatus()!=expectedStatus) throw ConsistencyException("trajectory found in list that does not match its status: id, status, list_status", trajID, Trajectory::status_strings[trajMap.at(trajID)->getStatus()].c_str(), Trajectory::status_strings[expectedStatus].c_str());
    return exists;
}

bool TrajectoryList::isTrajectoryInMap(lm::trajectory::Trajectory* traj, const TrajectoryMap& trajMap, Trajectory::Status expectedStatus) const
{
    bool exists(trajMap.count(traj->getID())==1);
    if (exists && traj->getStatus()!=expectedStatus) throw ConsistencyException("trajectory found in list that does not match its status: id, status, list_status", traj->getID(), Trajectory::status_strings[traj->getStatus()].c_str(), Trajectory::status_strings[expectedStatus].c_str());
    return exists;
}

// protected mutators
void TrajectoryList::setTrajectoryID(lm::trajectory::Trajectory* traj, uint64_t newID, Trajectory::Status newStatus)
{
    eraseTrajectoryID(traj->getID());

    traj->setID(newID);
    trajectories[newID] = traj;

    traj->setStatus(newStatus);
    (*getTrajectoryMap(newStatus))[newID] = traj;
}

void TrajectoryList::setTrajectoryStatus(lm::trajectory::Trajectory* traj, Trajectory::Status newStatus)
{
    uint64_t id = traj->getID();
    eraseTrajectoryIDFromSublists(id);
    traj->setStatus(newStatus);
    (*getTrajectoryMap(newStatus))[id] = traj;
}


}
}
