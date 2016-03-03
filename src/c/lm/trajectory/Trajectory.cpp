/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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
#include "lm/Types.h"
#include "lm/input/Input.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/tiling/Tilings.h"
#include "lm/trajectory/Trajectory.h"

using lm::io::DiffusionModel;
using lm::io::ReactionModel;
using lm::io::TrajectoryState;
using lm::tiling::Tilings;
using std::list;
using std::map;
using std::string;

namespace lm {
namespace trajectory {

char *trajectoryStatusStrings[] =
{
    "NOT_STARTED",
    "RUNNING",
    "WAITING",
    "FINISHED"
};

Trajectory::Trajectory(uint64_t id,const lm::io::TrajectoryState& initialState)
:id(id),status(NOT_STARTED),state(initialState),numberWorkUnitsPerformed(0)
{
}

Trajectory::Trajectory(uint64_t id, const lm::input::Input& input, bool reversed)
:id(id),status(NOT_STARTED),state(),numberWorkUnitsPerformed(0)
{
    initializeState(input, reversed);
}

Trajectory::~Trajectory()
{
}

void Trajectory::initializeState(const lm::input::Input& input, bool reversed)
{
    state.Clear();

    state.set_trajectory_id(id);

    // Set cme state from the reaction model.
    if (input.hasReactionModel())
    {
        const lm::io::ReactionModel& reactionModel = input.getReactionModelMsg();
        state.mutable_cme_state()->mutable_species_counts()->set_trajectory_id(id);
        state.mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
        state.mutable_cme_state()->mutable_species_counts()->set_number_species(reactionModel.number_species());
        if (!reversed)
        {
            for (uint j=0; j<reactionModel.number_species(); j++)
            {
                state.mutable_cme_state()->mutable_species_counts()->add_species_count(reactionModel.initial_species_count(j));
            }
        }
        else
        {
            for (uint j=0; j<reactionModel.number_species(); j++)
            {
                state.mutable_cme_state()->mutable_species_counts()->add_species_count(reactionModel.initial_species_count_backward(j));  // reversed_initial_species_count is set in the input file
            }
        }
        state.mutable_cme_state()->mutable_species_counts()->add_time(0.0);

        // Initialize the first passage times in the cme state.
        if (input.getOutputOptionsMsg().fpt_species_to_track_size())
        {
            for (int i=0; i< input.getOutputOptionsMsg().fpt_species_to_track_size(); i++)
            {
                uint speciesIndex = input.getOutputOptionsMsg().fpt_species_to_track(i);
                lm::io::FirstPassageTimes* fpt = state.mutable_cme_state()->add_first_passage_times();
                fpt->set_trajectory_id(id);
                fpt->set_species(speciesIndex);
                fpt->set_number_entries(1);
                fpt->add_species_count(reactionModel.initial_species_count(speciesIndex));
                fpt->add_first_passage_time(0.0);
            }
        }
    }

    // Initialize the rdme state from the diffusion model.
    if (input.hasDiffusionModel())
    {
        const lm::io::DiffusionModel& diffusionModel = input.getDiffusionModelMsg();
        lm::io::RDMEState* rdmeState = state.mutable_rdme_state();
        lm::io::Lattice* initialLattice = rdmeState->mutable_species_positions();
        initialLattice->set_lattice_x_size(diffusionModel.initial_lattice().lattice_x_size());
        initialLattice->set_lattice_y_size(diffusionModel.initial_lattice().lattice_y_size());
        initialLattice->set_lattice_z_size(diffusionModel.initial_lattice().lattice_z_size());
        initialLattice->set_particles_per_site(diffusionModel.initial_lattice().particles_per_site());
        initialLattice->set_particles_ordering(diffusionModel.initial_lattice().particles_ordering());
        initialLattice->set_particles(diffusionModel.initial_lattice().particles());
    }

    if (input.hasTilings()) inititializeHists(input);
}

void Trajectory::inititializeHists(const lm::input::Input& input)
{
    lm::io::TilingHist* tHist = state.mutable_cme_state()->add_tiling_hists();
    tHist->set_tiling_id(input.getTilings().getCurrentTilingID());
    for (lm::tiling::EdgeIterator e_it=input.getTilings().getCurrentTiling()->begin();e_it!=input.getTilings().getCurrentTiling()->end();e_it++)
    {
        tHist->add_tile_vals(0);
    }
//    for (lm::tiling::TilingMap::iterator t_it=input.getTilings().begin();t_it!=input.getTilings().end();t_it++)
//    {
//        lm::io::TilingHist* tHist = getState()->mutable_cme_state()->add_tiling_hists();
//        tHist->set_tiling_id(t_it->second->getID());
//        for (lm::tiling::EdgeIterator e_it=t_it->second->begin();e_it!=t_it->second->end();e_it++)
//        {
//            tHist->add_tile_vals(0);
//        }
//    }
}

// accessor definitions
int64_t Trajectory::getLimitIndexReached()
{
    return state.limit_index_reached();
}

uint64_t Trajectory::getId()
{
    return id;
}

const lm::io::OrderParametersValues& Trajectory::getOrderParameterValues()
{
    state.cme_state().order_parameter_values().order_parameter_values();
    return state.cme_state().order_parameter_values();
//	uint* lastSpeciesCount = new uint[getSpeciesCounts().number_species()];
//	uint offset = (getSpeciesCounts().number_entries() - 1)*(getSpeciesCounts().number_species());
//	double time = getSpeciesCounts().time(getSpeciesCounts().number_entries() - 1);  //double time = getSpeciesCounts()->time(getSpeciesCounts()->time_size()-1);
//	for (int i=0; i<getSpeciesCounts().number_species(); i++)
//	{
//		lastSpeciesCount[i] = getSpeciesCounts().species_count(i + offset);
//	}
//	return input.oparams[opID]->calc(lastSpeciesCount, time);
//	delete [] lastSpeciesCount;
}

const lm::io::SpeciesCounts& Trajectory::getSpeciesCounts()
{
	return state.cme_state().species_counts();
}

uint Trajectory::getSimSteps()
{
    return getSpeciesCounts().number_entries();
}

double Trajectory::getSimTime()
{
    return getSpeciesCounts().time(getSpeciesCounts().time_size() - 1);
}

Trajectory::status_t Trajectory::getStatus()
{
    return status;
}

const lm::io::TrajectoryState& Trajectory::getState()
{
    return state;
}

// debug helper function for printing trajectory status to stdout
void Trajectory::printStatus()
{
    printf("trajectory ID: %d has status: %s\n", id, trajectoryStatusStrings[getStatus()]);
}

// mutator definitions
void Trajectory::resetSimTime()
{
    state.mutable_cme_state()->mutable_species_counts()->set_time(getSpeciesCounts().time_size() - 1, 0.0);
}

void Trajectory::setLimitIndexReached(int64_t limitIndex)
{
    state.set_limit_index_reached(limitIndex);
}

void Trajectory::setID(uint64_t newID)
{
    id = newID;
    state.set_trajectory_id(newID);
    state.mutable_cme_state()->mutable_species_counts()->set_trajectory_id(newID);
}

void Trajectory::setState(const lm::io::TrajectoryState& newState)
{
    state.CopyFrom(newState);
}

void Trajectory::setStatus(status_t newStatus)
{
    status = newStatus;
}

void Trajectory::incrementWorkUnitsPerformed()
{
    numberWorkUnitsPerformed++;
}

int64_t Trajectory::getWorkUnitsPerformed()
{
    return numberWorkUnitsPerformed;
}

}
}
