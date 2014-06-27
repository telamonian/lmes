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
#include "lm/io/Lattice.pb.h"
#include "lm/io/RDMEState.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/replicates/TrajectoryList.h"

using std::string;

namespace lm {
namespace replicates {

TrajectoryList::TrajectoryList(int firstTrajectory, int lastTrajectory, map<string,string>& simulationParameters, const lm::io::ReactionModel& reactionModel, const lm::io::DiffusionModel& diffusionModel)
{
    for (int i=firstTrajectory; i<=lastTrajectory; i++)
    {
        trajectories[i] = new TrajectoryStatus(i);

        // Initialize the trajectory id.
        trajectories[i]->state.set_trajectory_id(i);

        // Initialize the species counts in the cme state.
        trajectories[i]->state.mutable_cme_state()->mutable_species_counts()->set_trajectory_id(i);
        trajectories[i]->state.mutable_cme_state()->mutable_species_counts()->set_number_species(reactionModel.number_species());
        trajectories[i]->state.mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
        for (int j=0; j<(int)reactionModel.number_species(); j++)
            trajectories[i]->state.mutable_cme_state()->mutable_species_counts()->add_species_count(reactionModel.initial_species_count(j));
        trajectories[i]->state.mutable_cme_state()->mutable_species_counts()->add_time(0.0);

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
            lm::io::FirstPassageTimes* fpt = trajectories[i]->state.mutable_cme_state()->add_first_passage_times();
            fpt->set_trajectory_id(i);
            fpt->set_species(*it);
            fpt->set_number_entries(1);
            fpt->add_species_count(reactionModel.initial_species_count(*it));
            fpt->add_first_passage_time(0.0);
            Print::printf(Print::DEBUG, "Added fpt tracking for species %d", *it);
        }

        // Initialize the rdme state from the diffusion model.
        lm::io::RDMEState* rdmeState = trajectories[i]->state.mutable_rdme_state();
        lm::io::Lattice* initialLattice = rdmeState->mutable_species_positions();
        initialLattice->set_data_order(diffusionModel.initial_lattice().data_order());
        initialLattice->set_lattice_x_size(diffusionModel.initial_lattice().lattice_x_size());
        initialLattice->set_lattice_y_size(diffusionModel.initial_lattice().lattice_y_size());
        initialLattice->set_lattice_z_size(diffusionModel.initial_lattice().lattice_z_size());
        initialLattice->set_particles_per_site(diffusionModel.initial_lattice().particles_per_site());
        initialLattice->set_particles(diffusionModel.initial_lattice().particles());
//        int aadded=0;
//        int badded=0;
//        string* particles=new string();
//        particles->resize(initialLattice->lattice_x_size()*initialLattice->lattice_y_size()*initialLattice->lattice_z_size()*initialLattice->particles_per_site());
//        char* buffer=&((*particles)[0]);
//        for (int x=0,index=0; x<initialLattice->lattice_x_size(); x++)
//            for (int y=0; y<initialLattice->lattice_y_size(); y++)
//                for (int z=0; z<initialLattice->lattice_z_size(); z++)
//                    for (int p=0; p<initialLattice->particles_per_site(); p++,index++)
//                    {
//                        if (p == 0 && z == 0 && aadded++ < 10)
//                            buffer[index]=1;
//                        else if (p == 0 && z == 4 && badded++ < 10)
//                            buffer[index]=2;
//                        else
//                            buffer[index]=0;
//                    }
//        initialLattice->set_allocated_particles(particles);
    }
}

TrajectoryList::~TrajectoryList()
{
    for (map<int,TrajectoryStatus*>::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        delete it->second;
    }
}

int TrajectoryList::nextTrajectoryToRun()
{
    for (map<int,TrajectoryStatus*>::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        TrajectoryStatus* t = it->second;
        if (t->status == NOT_STARTED || t->status == WAITING)
        {
            return t->trajectoryNumber;
        }
    }
    return -1;
}

TrajectoryList::status_t TrajectoryList::getTrajectoryStatus(int trajectory)
{
    return trajectories[trajectory]->status;
}

void TrajectoryList::updateTrajectoryStatus(int trajectory, status_t status)
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
