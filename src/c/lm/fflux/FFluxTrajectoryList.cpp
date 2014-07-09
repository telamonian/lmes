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

#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/io/CMEState.pb.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/Print.h"
#include "lm/resource/Trajectory.h"

using std::map;
using std::string;

namespace lm {
namespace fflux {

FFluxTrajectoryList::FFluxTrajectoryList(long long simulataneousTrajectoryCount, map<string,string>& simulationParameters, const lm::io::ReactionModel& reactionModel):
	TrajectoryList(simulationParameters, reactionModel), simulatenousTrajectoryCount(simulatenousTrajectoryCount)
{
}

FFluxTrajectoryList::~FFluxTrajectoryList()
{
    for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        delete it->second;
    }
}

void FFluxTrajectoryList::init()
{
	lm::io::CMEState* trajectoryCMEState = initTrajectoryCMEState();
    for (long long i=0; i<=simulatenousTrajectoryCount; i++)
    {
    	initTrajectory(trajectoryCount++, trajectoryCMEState);
    }
    delete trajectoryCMEState;
}

void FFluxTrajectoryList::initTrajectory(long long id, lm::io::CMEState* cmeState)
{
	// Construct new trajectory
	trajectories[id] = new lm::resource::Trajectory(id);

	// Initialize the trajectory's runWorkUnit message
	trajectories[id]->setMsg(trajectoryTemplateMsg);

	// Copy the referenced CMEState to a new CMEState
	lm::io::CMEState * newTrajectoryCMEState = new lm::io::CMEState(*cmeState);

	// Assign ownership of the CMEState copy to the newly constructed trajectory
	trajectories[id]->getState().set_allocated_cme_state(newTrajectoryCMEState);

	// Set the trajectory id in the trajectory state.
	trajectories[id]->getState().set_trajectory_id(id);

	// Set the trajectory id in the CME state of the trajectory state.
	trajectories[id]->getState().mutable_cme_state()->mutable_species_counts()->set_trajectory_id(id);
}

lm::io::CMEState* FFluxTrajectoryList::initTrajectoryCMEState()
{
	lm::io::CMEState* trajectoryCMEState = new lm::io::CMEState();
	trajectoryCMEState->mutable_species_counts()->set_number_species(reactionModel.number_species());
	trajectoryCMEState->mutable_species_counts()->set_number_entries(1);
	for (int j=0; j<(int)reactionModel.number_species(); j++)
		trajectoryCMEState->mutable_species_counts()->add_species_count(reactionModel.initial_species_count(j));
	trajectoryCMEState->mutable_species_counts()->add_time(0.0);
	return trajectoryCMEState;
}

void FFluxTrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit & finishedWorkUnitMsg)
{
	if (finishedWorkUnitMsg.status() == lm::message::FinishedWorkUnit::LIMIT_REACHED)
	{
		// TODO: add last state of trajectory to interceptList, remove old trajectory, start new trajectory
	}
	// Call the base class method
	TrajectoryList::workUnitFinished(finishedWorkUnitMsg);
		//        *run.mutable_initial_state() = trajectories->getTrajectoryState(nextTrajectory);
		//        Print::printf(Print::INFO, "Sending message to start work unit %d with trajectory %d on slot %d:%d.", run.work_unit_id(), nextTrajectory, workSlot->getSlotKey()[0], workSlot->getSlotKey()[1]);
		//        communicator.sendMessage(workSlot->getSlotKey()[0], workSlot->getSlotKey()[1], &msg);
		//        trajectories->updateTrajectoryStatus(nextTrajectory, FFluxTrajectoryList::RUNNING);

}

}
}
