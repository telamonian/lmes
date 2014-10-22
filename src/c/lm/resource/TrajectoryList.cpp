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

#include <list>
#include <map>
#include <string>

#include "lm/Print.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/resource/Trajectory.h"
#include "lm/resource/TrajectoryList.h"
#include "lm/Types.h"

using std::map;
using std::string;

namespace lm {
namespace resource {

TrajectoryList::TrajectoryList()
:trajectoryCount(0),workUnitCount(0)
{
}

TrajectoryList::~TrajectoryList()
{
    deleteAllTrajectories();
}

void TrajectoryList::initMsg(int supervisorProcess, int supervisorThread, int outputProcess, int outputThread)
{
    getRunMsg()->set_supervisor_process(supervisorProcess);
    getRunMsg()->set_supervisor_thread(supervisorThread);
    // Set the default writer process/thread
    getRunMsg()->set_output_process(outputProcess);
    getRunMsg()->set_output_thread(outputThread);
    // Set the default work unit-specific limits
    getRunMsg()->set_max_steps(100);
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
	TrajectoryMap::iterator it(trajectories.find(trajectoryID));
	delete it->second;
	trajectories.erase(it);
}

lm::resource::Trajectory* TrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit& msg)
{
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

lm::message::Message * TrajectoryList::getNextWorkUnitMsg()
{
	for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
	{
		if (it->second->getStatus()==Trajectory::NOT_STARTED || it->second->getStatus()==Trajectory::WAITING)
		{
			it->second->setStatus(Trajectory::RUNNING);
			it->second->setWorkUnitId(workUnitCount++);
			it->second->updateInitialRunState();
			return it->second->getMsg();
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

lm::resource::Trajectory* TrajectoryList::getTrajectory(uint64_t trajectoryID)
{
    return trajectories[trajectoryID];
}

Trajectory::status_t TrajectoryList::getTrajectoryStatus(uint64_t trajectoryID)
{
    return trajectories[trajectoryID]->getStatus();
}

const lm::io::TrajectoryState& TrajectoryList::getTrajectoryState(uint64_t trajectoryID)
{
    return trajectories[trajectoryID]->getState();
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
    trajectories[trajectoryID]->setState(state);
}

}
}
