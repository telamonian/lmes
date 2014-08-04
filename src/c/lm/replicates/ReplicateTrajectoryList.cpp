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
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/replicates/ReplicateTrajectoryList.h"
#include "lm/resource/Trajectory.h"
#include "lm/resource/TrajectoryList.h"
#include "lm/Types.h"

using std::map;
using std::string;

namespace lm {
namespace replicates {

ReplicateTrajectoryList::ReplicateTrajectoryList(uint64_t firstTrajectory, uint64_t lastTrajectory, map<string,string>& simulationParameters, const lm::io::ReactionModel& reactionModel, const lm::io::DiffusionModel& diffusionModel)
:TrajectoryList(),firstTrajectory(firstTrajectory),lastTrajectory(lastTrajectory),simulationParameters(simulationParameters),reactionModel(reactionModel),diffusionModel(diffusionModel)
{
}

ReplicateTrajectoryList::~ReplicateTrajectoryList()
{
}

void ReplicateTrajectoryList::init()
{
	for (uint64_t i=firstTrajectory; i<=lastTrajectory; i++)
	{
		trajectories[i] = new lm::resource::Trajectory(i);

		// Initialize the trajectory's runWorkUnit message
		trajectories[i]->setMsg(trajectoryTemplateMsg);

		// Initialize the trajectory id.
		trajectories[i]->getState().set_trajectory_id(i);

		// Initialize the species counts in the cme state.
		trajectories[i]->getState().mutable_cme_state()->mutable_species_counts()->set_trajectory_id(i);
		trajectories[i]->getState().mutable_cme_state()->mutable_species_counts()->set_number_species(reactionModel.number_species());
		trajectories[i]->getState().mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
		for (int j=0; j<(int)reactionModel.number_species(); j++)
			trajectories[i]->getState().mutable_cme_state()->mutable_species_counts()->add_species_count(reactionModel.initial_species_count(j));
		trajectories[i]->getState().mutable_cme_state()->mutable_species_counts()->add_time(0.0);

		// Initialize the first passage times in the cme state.
		const string listString = simulationParameters["fptTrackingList"];
		std::list<int> fptList;
		size_t start=0, end=0;
		while (end != string::npos)
		{
			end = listString.find(',', start);
			string trackedSpecies = listString.substr(start, (end == string::npos) ? string::npos : end - start);
			if (trackedSpecies.length() > 0)
			{
				fptList.push_back(atoi(trackedSpecies.c_str()));
			}
			start = end+1;
		}
		for (std::list<int>::iterator it=fptList.begin(); it != fptList.end(); it++)
		{
			lm::io::FirstPassageTimes* fpt= trajectories[i]->getState().mutable_cme_state()->add_first_passage_times();
			fpt->set_trajectory_id(i);
			fpt->set_species(*it);
			fpt->set_number_entries(1);
			fpt->add_species_count(reactionModel.initial_species_count(*it));
			fpt->add_first_passage_time(0.0);
			Print::printf(Print::DEBUG, "Added fpt tracking for species %d", *it);
		}

        // Initialize the rdme state from the diffusion model.
        lm::io::RDMEState* rdmeState = trajectories[i]->getState().mutable_rdme_state();
        lm::io::Lattice* initialLattice = rdmeState->mutable_species_positions();
        initialLattice->set_lattice_x_size(diffusionModel.initial_lattice().lattice_x_size());
        initialLattice->set_lattice_y_size(diffusionModel.initial_lattice().lattice_y_size());
        initialLattice->set_lattice_z_size(diffusionModel.initial_lattice().lattice_z_size());
        initialLattice->set_particles_per_site(diffusionModel.initial_lattice().particles_per_site());
        initialLattice->set_particles_ordering(diffusionModel.initial_lattice().particles_ordering());
        initialLattice->set_particles(diffusionModel.initial_lattice().particles());
    }
}

}
}
