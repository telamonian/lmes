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

Trajectory::Trajectory(uint64_t id,const lm::io::TrajectoryState& initialState)
:id(id),status(NOT_STARTED),state(initialState),numberWorkUnitsPerformed(0)
{
}

Trajectory::Trajectory(uint64_t id, const lm::input::Input& input, bool reversed)
:id(id),status(NOT_STARTED),state(),numberWorkUnitsPerformed(0)
{
    initializeState(input, reversed);
}

void Trajectory::initializeState(const lm::input::Input& input, bool reversed)
{
    state.Clear();

    // Set cme state from the reaction model.
    if (input.hasReactionModel())
    {
        const lm::io::ReactionModel& reactionModel = input.getReactionModel();
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
        if (input.hasFirstPassageTimes())
        {
            list<uint> fptSpeciesList = input.getFirstPassageTimesSpecies();
            for (std::list<uint>::iterator it=fptSpeciresList.begin(); it != fptSpeciesList.end(); it++)
            {
                lm::io::FirstPassageTimes* fpt = state.mutable_cme_state()->add_first_passage_times();
                fpt->set_trajectory_id(i);
                fpt->set_species(*it);
                fpt->set_number_entries(1);
                fpt->add_species_count(reactionModel.initial_species_count(*it));
                fpt->add_first_passage_time(0.0);
                Print::printf(Print::DEBUG, "Added fpt tracking for species %d", *it);
            }
        }

    }

    // Initialize the rdme state from the diffusion model.
    if (input.hasDiffusionModel)
    {
        lm::io::RDMEState* rdmeState = trajectories[i]->getState()->mutable_rdme_state();
        lm::io::Lattice* initialLattice = rdmeState->mutable_species_positions();
        initialLattice->set_lattice_x_size(input.diffusionModelBuf.initial_lattice().lattice_x_size());
        initialLattice->set_lattice_y_size(input.diffusionModelBuf.initial_lattice().lattice_y_size());
        initialLattice->set_lattice_z_size(input.diffusionModelBuf.initial_lattice().lattice_z_size());
        initialLattice->set_particles_per_site(input.diffusionModelBuf.initial_lattice().particles_per_site());
        initialLattice->set_particles_ordering(input.diffusionModelBuf.initial_lattice().particles_ordering());
        initialLattice->set_particles(input.diffusionModelBuf.initial_lattice().particles());
    }



    //if (input.hasTilings) initHists();
}


Trajectory::~Trajectory()
{
}

/*void Trajectory::initHists()
{
    lm::io::TilingHist* tHist = getState()->mutable_cme_state()->add_tiling_hists();
    tHist->set_tiling_id(input.tilings.getCurrentTilingID());
    for (lm::tiling::EdgeIterator e_it=input.tilings.getCurrentTiling()->begin();e_it!=input.tilings.getCurrentTiling()->end();e_it++)
    {
        tHist->add_tile_vals(0);
    }
//    for (lm::tiling::TilingMap::iterator t_it=input.tilings.begin();t_it!=input.tilings.end();t_it++)
//    {
//        lm::io::TilingHist* tHist = getState()->mutable_cme_state()->add_tiling_hists();
//        tHist->set_tiling_id(t_it->second->getID());
//        for (lm::tiling::EdgeIterator e_it=t_it->second->begin();e_it!=t_it->second->end();e_it++)
//        {
//            tHist->add_tile_vals(0);
//        }
//    }
}
*/

uint64_t Trajectory::getId()
{
    return id;
}

Trajectory::status_t Trajectory::getStatus()
{
    return status;
}

const lm::io::TrajectoryState& Trajectory::getState()
{
    return state;
}

int64_t Trajectory::getWorkUnitsPerformed()
{
    return numberWorkUnitsPerformed;
}

void Trajectory::setState(const lm::io::TrajectoryState* newState)
{
    *getState() = *newState;
}

void Trajectory::setStatus(status_t newStatus)
{
    status = newStatus;
}

void Trajectory::incrementWorkUnitsPerformed()
{
    numberWorkUnitsPerformed++;
}

}
}
