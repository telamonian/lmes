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
#include <string>

#include "lm/Print.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/trajectory/TrajectoryList.h"
#include "lm/Types.h"

using lm::io::DiffusionModel;
using lm::io::ReactionModel;
using std::map;
using std::string;

namespace lm {
namespace trajectory {

TrajectoryList::TrajectoryList(): simulationPhase(0)
{
}

TrajectoryList::TrajectoryList(uint64_t simulationPhase): simulationPhase(simulationPhase)
{
}

TrajectoryList::~TrajectoryList()
{
    deleteAllTrajectories();
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
    }
    trajectories.clear();
    waitingTrajectories.clear();
    runningTrajectories.clear();
    finishedTrajectories.clear();
}

void TrajectoryList::deleteTrajectory(uint64_t id)
{
    if (trajectories.count(id))
    {
        delete trajectories[id];
        trajectories[id] = NULL;
        trajectories.erase(id);
        waitingTrajectories.erase(id);
        runningTrajectories.erase(id);
        finishedTrajectories.erase(id);
    }
}

// accessors
bool TrajectoryList::areAllFinished() const
{
    return waitingTrajectories.size() == 0 && runningTrajectories.size() == 0;
}

//mutators
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
            t->setStatus(Trajectory::RUNNING);
            waitingTrajectories.erase(id);
            runningTrajectories[id] = t;

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

Trajectory* TrajectoryList::getRunningTrajectory(uint64_t id)
{
    if (runningTrajectories.count(id) == 0)
        throw Exception("Consistency error in trajectory list, expected trajectory not in the running list",id);

    Trajectory* t = runningTrajectories[id];
    if (t->getStatus() != Trajectory::RUNNING)
        throw Exception("Consistency error in trajectory list, expected trajectory did not have a running status",id);
    return t;
}

void TrajectoryList::incrementSimulationPhase()
{
    simulationPhase++;
}

void TrajectoryList::setAllFinished()
{
    for (TrajectoryMap::iterator it=waitingTrajectories.begin(); it!=waitingTrajectories.end(); it++)
    {
        it->second->setStatus(Trajectory::FINISHED);
        finishedTrajectories[it->first] = it->second;
    }
    waitingTrajectories.clear();
    for (TrajectoryMap::iterator it=runningTrajectories.begin(); it!=runningTrajectories.end(); it++)
    {
        it->second->setStatus(Trajectory::FINISHED);
        finishedTrajectories[it->first] = it->second;
    }
    runningTrajectories.clear();
    workUnitsRunning.clear();
}

void TrajectoryList::setTrajectoryFinished(lm::trajectory::Trajectory* traj)
{
    traj->setStatus(Trajectory::FINISHED);

    uint64_t id = traj->getID();
    if (runningTrajectories.count(id)) runningTrajectories.erase(id);
    if (waitingTrajectories.count(id)) waitingTrajectories.erase(id);
    finishedTrajectories[id] = traj;
}
void TrajectoryList::setTrajectoryRunning(lm::trajectory::Trajectory* traj)
{
    traj->setStatus(Trajectory::RUNNING);

    uint64_t id = traj->getID();
    if (finishedTrajectories.count(id)) finishedTrajectories.erase(id);
    if (waitingTrajectories.count(id)) waitingTrajectories.erase(id);
    runningTrajectories[id] = traj;
}
void TrajectoryList::setTrajectoryWaiting(lm::trajectory::Trajectory* traj)
{
    traj->setStatus(Trajectory::WAITING);

    uint64_t id = traj->getID();
    if (finishedTrajectories.count(id)) finishedTrajectories.erase(id);
    if (runningTrajectories.count(id)) runningTrajectories.erase(id);
    waitingTrajectories[id] = traj;
}

void TrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit& fwuBuf)
{
    // Get the work unit id.
    uint64_t workUnitId = fwuBuf.work_unit_id();

    //Get the list of trajectories associated with this work unit.
    list<uint64_t> involvedTrajectories = workUnitsRunning[workUnitId];
    workUnitsRunning.erase(workUnitId);

    //Make sure the sizes between the list and the message are consistent.
    if (involvedTrajectories.size() != fwuBuf.part_status_size())
        throw Exception("Consistency error in trajectory list, number of involved trajectories differed from work units finished message",workUnitId);

    // Loop over the trajectories.
    for (list<uint64_t>::iterator it=involvedTrajectories.begin(); it != involvedTrajectories.end(); it++)
    {
        uint64_t id = *it;

        // normally the trajectory associated with the id has to still exist at this point or an exception is thrown. The two lines below are a hook that allows subclasses to override this behavior.
        Trajectory* t = getRunningTrajectory(id);
        if (t==NULL) continue;

        // Find the trajectory in the message.
        int partIndex=-1;
        for (int i=0; i<fwuBuf.part_status_size(); i++)
        {
            if (fwuBuf.part_status(i).final_state().trajectory_id() == id)
            {
                partIndex = i;
                break;
            }
        }
        if (partIndex == -1)
            throw Exception("Consistency error in trajectory list, could not find trajectory id in work units finished",id);

        workUnitPartFinished(fwuBuf.part_status(partIndex), t);

//        // Update the state of the trajectory.
//        t->setState(fwuBuf.part_status(partIndex).final_state());
//        t->incrementWorkUnitsPerformed();
//
//        // Update the status of the trajectory and move to the appropriate list.
//        if (fwuBuf.part_status(partIndex).status() == lm::message::WorkUnitStatus::STEPS_FINISHED)
//        {
//            t->setStatus(Trajectory::WAITING);
//            runningTrajectories.erase(id);
//            waitingTrajectories[id] = t;
//        }
//        else if (fwuBuf.part_status(partIndex).status() == lm::message::WorkUnitStatus::LIMIT_REACHED)
//        {
//            t->setStatus(Trajectory::FINISHED);
//            runningTrajectories.erase(id);
//            finishedTrajectories[id] = t;
//        }
//        else if (fwuBuf.part_status(partIndex).status() == lm::message::WorkUnitStatus::ERROR)
//        {
//            if (fwuBuf.part_status(partIndex).has_error_message())
//                throw Exception("Error received in work unit status",fwuBuf.part_status(partIndex).error_message().c_str());
//            else
//                throw Exception("Error received in work unit status","<no error message specified");
//        }
//        else
//        {
//            throw Exception("Unknown work unit status", fwuBuf.part_status(partIndex).status());
//        }
    }
}

void TrajectoryList::workUnitPartFinished(const lm::message::WorkUnitStatus& wusBuf)
{
    workUnitPartFinished(wusBuf, runningTrajectories[wusBuf.final_state().trajectory_id()]);
}

void TrajectoryList::workUnitPartFinished(const lm::message::WorkUnitStatus& wusBuf, lm::trajectory::Trajectory* traj)
{
    uint64_t id = wusBuf.final_state().trajectory_id();
    
    // Update the state of the trajectory.
    traj->setState(wusBuf.final_state());
    traj->incrementWorkUnitsPerformed();

    // Update the status of the trajectory and move to the appropriate list.
    if (wusBuf.status() == lm::message::WorkUnitStatus::STEPS_FINISHED)
    {
        traj->setStatus(Trajectory::WAITING);
        runningTrajectories.erase(id);
        waitingTrajectories[id] = traj;
    }
    else if (wusBuf.status() == lm::message::WorkUnitStatus::LIMIT_REACHED)
    {
        traj->setStatus(Trajectory::FINISHED);
        runningTrajectories.erase(id);
        finishedTrajectories[id] = traj;
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

// protected
uint64_t TrajectoryList::findNextTrajectoryToRun() const
{
    TrajectoryMap::const_iterator it = waitingTrajectories.begin();
    return it->first;
}

}
}
