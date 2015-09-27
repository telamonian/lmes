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
#include <list>
#include <map>
#include <string>

#include "lm/Print.h"
#include "lm/input/Input.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/trajectory/TrajectoryList.h"
#include "lm/Types.h"

using lm::io::DiffusionModel;
using lm::io::ReactionModel;
using std::map;
using std::string;

namespace lm {
namespace trajectory {

TrajectoryList::TrajectoryList(lm::input::Input& input)
:communicator(NULL),input(input),trajectoryCount(0),workUnitCount(0)
{
}

TrajectoryList::~TrajectoryList()
{
    deleteAllTrajectories();
}

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
}

void TrajectoryList::deleteTrajectory(uint64_t trajectoryID)
{
//	Print::printf(Print::INFO,"Deleting trajectory ID: %d", trajectoryID);
    TrajectoryMap::iterator it(trajectories.find(trajectoryID));
    delete it->second;
    trajectories.erase(it);
}

// initializer(s)
void TrajectoryList::setCommunicator(lm::message::Communicator& newCom)
{
    communicator = &newCom;
}

lm::trajectory::Trajectory* TrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit& msg)
{
    Trajectory* t = trajectories[msg.final_state().trajectory_id()];
    // store some info for later use by printTrajectoryStatistics
    t->incrementWorkUnitsPerformed();

    // Print some performance statistics, if it has been a while.
    printTrajectoryStatistics();

    if (msg.status() == lm::message::FinishedWorkUnit::LIMIT_REACHED)
    {
        setTrajectoryStatus(msg.final_state().trajectory_id(), Trajectory::FINISHED);
        setTrajectoryState(msg.final_state().trajectory_id(), msg.final_state());
    }
    else
    {
        setTrajectoryStatus(msg.final_state().trajectory_id(), Trajectory::WAITING);
        setTrajectoryState(msg.final_state().trajectory_id(), msg.final_state());
    }
    setTrajectoryStarted(msg.final_state().trajectory_id(), true);

    return getTrajectory(msg.final_state().trajectory_id());
}

lm::message::Message* TrajectoryList::getNextWorkUnitMsg()
{
//	Print::printf(Print::INFO, "As I get the next work unit, the trajectories size is: %d\n", trajectories.size());
    for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        lm::message::Message* retMsg = it->second->getNextWorkUnitMsg(workUnitCount++);
        if (retMsg!=NULL)
        {
            return retMsg;
        }
    }
    return NULL;
}

// check if all of the trajectories are truly finished or if some of them are still running
bool TrajectoryList::isFinished()
{
    for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        if (it->second->getStatus()==Trajectory::RUNNING)
        {
            return false;
        }
    }
    return true;
}

lm::trajectory::Trajectory* TrajectoryList::getTrajectory(uint64_t trajectoryID)
{
    if(trajectories.find(trajectoryID)!=trajectories.end())
    {
        return trajectories[trajectoryID];
    }
    else
    {
        return NULL;
    }
}

Trajectory::status_t TrajectoryList::getTrajectoryStatus(uint64_t trajectoryID)
{
    return trajectories[trajectoryID]->getStatus();
}

lm::io::TrajectoryState* TrajectoryList::getTrajectoryState(uint64_t trajectoryID)
{
    return trajectories[trajectoryID]->getState();
}

void TrajectoryList::setAllFinished()
{
	for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
	{
//	    it->second->printStatus();
		it->second->setStatus(Trajectory::FINISHED);
	}
}

void TrajectoryList::setTrajectoryStarted(uint64_t trajectoryID, bool trajectoryStarted)
{
    trajectories[trajectoryID]->setStarted(trajectoryStarted);
}

void TrajectoryList::setTrajectoryStatus(uint64_t trajectoryID, Trajectory::status_t status)
{
    trajectories[trajectoryID]->setStatus(status);
}

void TrajectoryList::setTrajectoryState(uint64_t trajectoryID, const lm::io::TrajectoryState& state)
{
    trajectories[trajectoryID]->setState(&state);
}

void TrajectoryList::printTrajectoryStatistics()
{
}

}
}
