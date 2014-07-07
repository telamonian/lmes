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
#include "lm/resource/TrajectoryList.h"

using std::map;
using std::string;

namespace lm {
namespace resource {

TrajectoryList::TrajectoryList(map<string,string>& simulationParameters, const lm::io::ReactionModel& reactionModel): simulationParameters(simulationParameters), reactionModel(reactionModel), trajectoryCount(0),workUnitCount(0)
{
}

TrajectoryList::~TrajectoryList()
{
    for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        delete it->second;
    }
}

void TrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit & msg)
{
	if (msg.status() == lm::message::FinishedWorkUnit::LIMIT_REACHED)
	{
		updateTrajectoryStatus(msg.final_state().trajectory_id(), TrajectoryList::FINISHED);
		updateTrajectoryState(msg.final_state().trajectory_id(), msg.final_state());
	}
	else
	{
		updateTrajectoryStatus(msg.final_state().trajectory_id(), TrajectoryList::WAITING);
		updateTrajectoryState(msg.final_state().trajectory_id(), msg.final_state());
	}
}

lm::message::Message * TrajectoryList::getNextWorkUnitMsg()
{
	lm::message::Message * nextWorkUnitMsg;
	for (TrajectoryMap::iterator it : trajectories)
	{
		if (it->second->status==NOT_STARTED || it->second->status==WAITING)
		{
			it->second->setWorkUnitId(workUnitCount++);
			return it->second->msg;
		}
	}
	return NULL;
}

Trajectory::status_t TrajectoryList::getTrajectoryStatus(int trajectory)
{
    return trajectories[trajectory]->status;
}

void TrajectoryList::updateTrajectoryStatus(int trajectory, Trajectory::status_t status)
{
    trajectories[trajectory]->status = status;
}

const lm::io::TrajectoryState& TrajectoryList::getTrajectoryState(int trajectory)
{
    return trajectories[trajectory]->state;
}

void TrajectoryList::updateTrajectoryState(int trajectory, const lm::io::TrajectoryState& state)
{
    trajectories[trajectory]->state = state;
}

}
}
