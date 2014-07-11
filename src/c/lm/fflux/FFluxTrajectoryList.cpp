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

FFluxTrajectoryList::FFluxTrajectoryList(uint64_t simulataneousTrajectoryCount, map<string,string>& simulationParameters, const lm::io::ReactionModel& reactionModel):
	TrajectoryList(simulationParameters, reactionModel), ffluxPhase(0), simulatenousTrajectoryCount(simulatenousTrajectoryCount)
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
	lm::io::TrajectoryState* trajectoryState = initFirstTrajectoryState();
    for (long long i=0; i<=simulatenousTrajectoryCount; i++)
    {
    	initTrajectory(trajectoryCount++, trajectoryState);
    }
    delete trajectoryState;
}

void FFluxTrajectoryList::initTrajectory(uint64_t id, lm::io::TrajectoryState* state)
{
	// Construct new trajectory
	trajectories[id] = new lm::resource::Trajectory(id);

	// Initialize the trajectory's runWorkUnit message
	trajectories[id]->setMsg(trajectoryTemplateMsg);

	// Copy the TrajectoryState referenced in the function args to the TrajectoryState of the newly constructed trajectory
	trajectories[id]->setState(*state);

	// Set the trajectory id in the trajectory state.
	trajectories[id]->getState().set_trajectory_id(id);

	// Set the trajectory id in the CME state of the trajectory state (if applicable).
	if (trajectories[id]->getState().has_cme_state())
		trajectories[id]->getState().mutable_cme_state()->mutable_species_counts()->set_trajectory_id(id);

	// Set the trajectory id in the RDME state of the trajectory state (if applicable).
//	if (trajectories[id]->getState().has_rdme_state())
//		trajectories[id]->getState().mutable_rdme_state()->mutable_species_counts()->set_trajectory_id(id);
}

lm::io::TrajectoryState* FFluxTrajectoryList::initFirstTrajectoryState()
{
	lm::io::TrajectoryState* trajectoryState = new lm::io::TrajectoryState();
	trajectoryState->mutable_cme_state()->mutable_species_counts()->set_number_species(reactionModel.number_species());
	trajectoryState->mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
	for (int j=0; j<(int)reactionModel.number_species(); j++)
		trajectoryState->mutable_cme_state()->mutable_species_counts()->add_species_count(reactionModel.initial_species_count(j));
	trajectoryState->mutable_cme_state()->mutable_species_counts()->add_time(0.0);
	return trajectoryState;
}

void FFluxTrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit & finishedWorkUnitMsg)
{
	// Call the base class method
	TrajectoryList::workUnitFinished(finishedWorkUnitMsg);

	// If the work unit stopped because it detected a crossing event...
	if (finishedWorkUnitMsg.status()==lm::message::FinishedWorkUnit::LIMIT_REACHED)
	{
		// ...add the work unit's final state to the appropriate list of crossings and delete the finished trajectory
		crossings[ffluxPhase].push_back(finishedWorkUnitMsg.final_state());
		deleteTrajectory(finishedWorkUnitMsg.final_state().trajectory_id());
		// Next, if enough crossing events have been detected for this phase of forward flux sampling...
		if (crossings[ffluxPhase].size()>=100)
		{
			// ...delete the currently running set of trajectories, increment the fflux phase counter, and start up a new set of trajectories
		}
		// ...otherwise we still have to collect more crossing events related to this phase's interface...
		else
		{
			// ...and if the forward flux sampling is still in its 0th (ie initial) phase...
			if (ffluxPhase==0)
			{

			}
			// ...otherwise if ffluxPhase > 0...
			else
			{

			}
		}

	}
		//        *run.mutable_initial_state() = trajectories->getTrajectoryState(nextTrajectory);
		//        Print::printf(Print::INFO, "Sending message to start work unit %d with trajectory %d on slot %d:%d.", run.work_unit_id(), nextTrajectory, workSlot->getSlotKey()[0], workSlot->getSlotKey()[1]);
		//        communicator.sendMessage(workSlot->getSlotKey()[0], workSlot->getSlotKey()[1], &msg);
		//        trajectories->updateTrajectoryStatus(nextTrajectory, FFluxTrajectoryList::RUNNING);

}

}
}
