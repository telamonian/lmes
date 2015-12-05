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

#include <limits>
#include <list>
#include <map>
#include <string>
#include "hrtime.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/input/Input.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/replicates/ReplicateTrajectory.h"
#include "lm/replicates/ReplicateTrajectoryList.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/trajectory/TrajectoryList.h"

#ifndef UINT64_MAX
#define UINT64_MAX        18446744073709551615ULL
#endif

using std::map;
using std::string;

namespace lm {
namespace replicates {

ReplicateTrajectoryList::ReplicateTrajectoryList(lm::input::Input& input, uint64_t firstTrajectory, uint64_t lastTrajectory)
:TrajectoryList(input),firstTrajectory(firstTrajectory),lastTrajectory(lastTrajectory),stats_lastPrintTime(getHrTime())
{
    init();
}

ReplicateTrajectoryList::~ReplicateTrajectoryList()
{

}

void ReplicateTrajectoryList::init()
{
	for (uint64_t i=firstTrajectory; i<=lastTrajectory; i++)
	{
		trajectories[i] = new lm::replicates::ReplicateTrajectory(i,input);

		// Initialize the first passage times in the cme state.
		const string listString = input.simulationParameters["fptTrackingList"];
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
			lm::io::FirstPassageTimes* fpt= trajectories[i]->getState()->mutable_cme_state()->add_first_passage_times();
			fpt->set_trajectory_id(i);
			fpt->set_species(*it);
			fpt->set_number_entries(1);
			fpt->add_species_count(input.reactionModelBuf.initial_species_count(*it));
			fpt->add_first_passage_time(0.0);
			Print::printf(Print::DEBUG, "Added fpt tracking for species %d", *it);
		}

        // Initialize the rdme state from the diffusion model.
        lm::io::RDMEState* rdmeState = trajectories[i]->getState()->mutable_rdme_state();
        lm::io::Lattice* initialLattice = rdmeState->mutable_species_positions();
        initialLattice->set_lattice_x_size(input.diffusionModelBuf.initial_lattice().lattice_x_size());
        initialLattice->set_lattice_y_size(input.diffusionModelBuf.initial_lattice().lattice_y_size());
        initialLattice->set_lattice_z_size(input.diffusionModelBuf.initial_lattice().lattice_z_size());
        initialLattice->set_particles_per_site(input.diffusionModelBuf.initial_lattice().particles_per_site());
        initialLattice->set_particles_ordering(input.diffusionModelBuf.initial_lattice().particles_ordering());
        initialLattice->set_particles(input.diffusionModelBuf.initial_lattice().particles());
    }
}

lm::trajectory::Trajectory* ReplicateTrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit& msg)
{
    lm::trajectory::Trajectory* t = TrajectoryList::workUnitFinished(msg);
    if (t->getStatus() == lm::trajectory::Trajectory::FINISHED)
    {
        Print::printf(Print::INFO, "Replicate %lld completed with %8.2e of simulation time using %d work units.", t->getID(), t->getState()->cme_state().species_counts().time(0), t->getWorkUnitsPerformed());
    }

    return t;
}

lm::message::Message* ReplicateTrajectoryList::getNextWorkUnitMsg()
{
    uint64_t minId=UINT64_MAX;
    double minTime=std::numeric_limits<double>::infinity();
    for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        lm::trajectory::Trajectory* t = it->second;
        if (t->getStatus()==lm::trajectory::Trajectory::NOT_STARTED || t->getStatus()==lm::trajectory::Trajectory::WAITING)
        {
            double time = t->getState()->cme_state().species_counts().time(0);
            if (time < minTime)
            {
                minTime = time;
                minId = it->first;
            }
        }
    }

    // If we found an available trajectory, return the next work unit message.
    if (minId != UINT64_MAX)
    {
        return trajectories[minId]->getNextWorkUnitMsg(workUnitCount++);
    }
    return NULL;
}

void ReplicateTrajectoryList::printTrajectoryStatistics()
{
    // Print some performance statistics, if it has been a while.
    hrtime currentTime = getHrTime();
    if (convertHrToSeconds(currentTime-stats_lastPrintTime) > 700.0)
    {
        const std::string statusStrings[] = {"NOT_STARTED", "RUNNING", "WAITING", "FINISHED"};
        Print::printf(Print::INFO, "Trajectory status");
        Print::printf(Print::INFO, "        ID State       Time     Work Units");
        Print::printf(Print::INFO, "------------------------------------------");
        for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
        {
            uint64_t id = it->first;
            lm::trajectory::Trajectory* t = it->second;
            Print::printf(Print::INFO, "%10lld %-11s %8.2e %10d", id, statusStrings[(int)t->getStatus()].c_str(), t->getState()->cme_state().species_counts().time(0), t->getWorkUnitsPerformed());
        }
        stats_lastPrintTime = getHrTime();
    }
}

}
}
