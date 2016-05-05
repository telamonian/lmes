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
#include <cmath>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/array/Tuple.h"
#include "lm/input/Input.h"
#include "lm/protowrap/NDArray.h"
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
using std::vector;

namespace lm {
namespace trajectory {

const std::string Trajectory::status_t_strings[] = {"ABORTED",
                                                    "FINISHED",
                                                    "NOT_STARTED",
                                                    "RUNNING",
                                                    "WAITING"};

Trajectory::Trajectory(uint64_t id, uint64_t phase, const lm::io::TrajectoryState& initialState)
:id(static_cast<uint>(-1)),simulationPhase(phase),status(NOT_STARTED),state(initialState),numberWorkUnitsPerformed(0)
{
    setID(id);
}

Trajectory::Trajectory(uint64_t id, uint64_t phase, const lm::input::Input& input, bool reversed)
:id(id),simulationPhase(phase),status(NOT_STARTED),state(),numberWorkUnitsPerformed(0)
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
        // Initialize the degree advancements
        if (input.hasDegreeAdvancement())
        {
            initializeDegreeAdvancements(input);
        }

        // Initialize the species counts
        const lm::io::ReactionModel& reactionModel = input.getReactionModelMsg();
        lm::io::SpeciesCounts* sc = state.mutable_cme_state()->mutable_species_counts();
        sc->set_trajectory_id(id);
        sc->set_number_entries(1);
        sc->set_number_species(reactionModel.number_species());
        if (!reversed)
        {
            for (uint j=0; j<reactionModel.number_species(); j++)
            {
                sc->add_species_count(reactionModel.initial_species_count(j));
            }
        }
        else
        {
            for (uint j=0; j<reactionModel.number_species(); j++)
            {
                sc->add_species_count(reactionModel.initial_species_count_backward(j));  // reversed_initial_species_count is set in the input file
            }
        }
        sc->add_time(0.0);
        
        // Initialize the order parameters values
        if (input.hasOrderParameters())
        {
            initializeOrderParameters(input);
        }
        
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

        // Initialize the order parameter first passage times in the cme state.
        if (input.getOutputOptionsMsg().fpt_order_parameter_to_track_size())
        {
            initializeOrderParameterFirstPassageTimes(input);
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

    // Initialize the tiling hists
    if (input.hasTilings())
    {
        inititializeHists(input);
    }
}

void Trajectory::initializeDegreeAdvancements(const lm::input::Input& input)
{
    const lm::io::ReactionModel& reactionModel = input.getReactionModelMsg();
    lm::io::DegreeAdvancements* da = state.mutable_cme_state()->mutable_degree_advancements();
    da->set_trajectory_id(id);
    da->set_number_entries(1);
    da->set_number_reactions(reactionModel.number_reactions());
    da->mutable_degree_advancements()->Resize(da->number_reactions(), 0);
    da->add_time(0.0);
}

void Trajectory::inititializeHists(const lm::input::Input& input)
{
    lm::io::TilingHist* tHist = state.mutable_cme_state()->add_tiling_hists();
    tHist->set_tiling_id(input.getTilings().getCurrentTilingID());
    for (lm::tiling::EdgeIterator e_it=input.getCurrentTiling().begin();e_it!=input.getCurrentTiling().end();e_it++)
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

void Trajectory::initializeOrderParameters(const lm::input::Input& input)
{
    const lm::oparam::OParams& oparams = input.getOrderParameters();
    lm::io::OrderParametersValues* opv = state.mutable_cme_state()->mutable_order_parameter_values();
    opv->set_trajectory_id(id);
    opv->set_number_entries(1);
    opv->set_number_order_parameters(oparams.size());
    for (uint i=0; i<opv->number_order_parameters(); i++)
    {
        opv->add_order_parameter_values(oparams.at(i)->calc(state));
    }
    opv->add_time(0.0);
}

void Trajectory::initializeOrderParameterFirstPassageTimes(const lm::input::Input& input)
{
    for (int i=0; i< input.getOutputOptionsMsg().fpt_order_parameter_to_track_size(); i++)
    {
        uint oparamID = input.getOutputOptionsMsg().fpt_order_parameter_to_track(i);
        const lm::oparam::OParam* oparam = input.getOrderParameters().at(oparamID);
        lm::io::OrderParameterFirstPassageTimes* opFPT = state.mutable_cme_state()->add_order_parameter_first_passage_times();
        opFPT->set_trajectory_id(id);
        opFPT->set_order_parameter_id(oparamID);

        // TODO: improve the syntax of .set_array()
        lm::protowrap::NDArray<int> fptValueWrap(opFPT->mutable_order_parameter_value());
        fptValueWrap.set_array(UTuple(1), std::vector<int>(1, (int)round(oparam->calc(state))), false);

        // TODO: improve the syntax of .set_array()
        lm::protowrap::NDArray<double> timeWrap(opFPT->mutable_first_passage_time());
        timeWrap.set_array(UTuple(1), std::vector<double>(1, 0.0), false);
    }
}

// accessors
vector<double> Trajectory::getLastOrderParameterValues() const
{
    const lm::io::OrderParametersValues& orderParameterValues(getOrderParameterValues());

    // offset the order_parameter_values iterator to ensure that we only get the last "row" of values
    int offset = (orderParameterValues.number_entries() - 1)*(orderParameterValues.number_order_parameters());
    return vector<double>(orderParameterValues.order_parameter_values().begin()+offset, orderParameterValues.order_parameter_values().end());
}

vector<int32_t> Trajectory::getLastSpeciesCounts() const
{
    const lm::io::SpeciesCounts& speciesCounts(getSpeciesCounts());

    // offset the species_count iterator to ensure that we only get the last "row" of values
    int offset = (speciesCounts.number_entries() - 1)*(speciesCounts.number_species());
    return vector<int32_t>(speciesCounts.species_count().begin()+offset, speciesCounts.species_count().end());
}

const lm::io::TrajectoryLimits::TrajectoryLimit& Trajectory::getLimitReached() const
{
    return state.limit_reached();
}

uint64_t Trajectory::getID() const
{
    return id;
}

const lm::io::OrderParametersValues& Trajectory::getOrderParameterValues() const
{
    return state.cme_state().order_parameter_values();
}

uint64_t Trajectory::getSimulationPhase() const
{
    return simulationPhase;
}

int32_t Trajectory::getSimSteps() const
{
    return getSpeciesCounts().number_entries();
}

double Trajectory::getSimTime() const
{
    return getSpeciesCounts().time(getSpeciesCounts().time_size() - 1);
}

const lm::io::SpeciesCounts& Trajectory::getSpeciesCounts() const
{
    return state.cme_state().species_counts();
}

Trajectory::status_t Trajectory::getStatus() const
{
    return status;
}

const lm::io::TrajectoryState& Trajectory::getState() const
{
    return state;
}

int64_t Trajectory::getWorkUnitsPerformed() const
{
    return numberWorkUnitsPerformed;
}

// debug helper function for printing trajectory status to stdout
void Trajectory::printStatus() const
{
    printf("trajectory ID: %d has status: %s\n", id, status_t_strings[getStatus()].c_str());
}

// mutators
void Trajectory::clearLimitReached()
{
    state.clear_limit_reached();
}

double* Trajectory::getLastOrderParameterValuesMutable()
{
    lm::io::OrderParametersValues* opv(state.mutable_cme_state()->mutable_order_parameter_values());

    // offset the order_parameter_values pointer to ensure that we only get the last "row" of values
    int offset = (opv->number_entries() - 1)*(opv->number_order_parameters());
    return opv->mutable_order_parameter_values()->mutable_data() + offset;
}

int32_t* Trajectory::getLastSpeciesCountsMutable()
{
    lm::io::SpeciesCounts* sc(state.mutable_cme_state()->mutable_species_counts());

    // offset the species_count pointer to ensure that we only get the last "row" of values
    int offset = (sc->number_entries() - 1)*(sc->number_species());
    return sc->mutable_species_count()->mutable_data() + offset;
}

void Trajectory::incrementWorkUnitsPerformed()
{
    numberWorkUnitsPerformed++;
}

void Trajectory::resetSimTime()
{
    state.mutable_cme_state()->mutable_species_counts()->set_time(getSpeciesCounts().time_size() - 1, 0.0);
}

void Trajectory::setID(uint64_t newID)
{
    id = newID;
    state.set_trajectory_id(newID);
    state.mutable_cme_state()->mutable_species_counts()->set_trajectory_id(newID);

    if (state.mutable_cme_state()->has_degree_advancements()) state.mutable_cme_state()->mutable_degree_advancements()->set_trajectory_id(newID);
    if (state.mutable_cme_state()->has_order_parameter_values()) state.mutable_cme_state()->mutable_order_parameter_values()->set_trajectory_id(newID);
}

void Trajectory::setLimitReached(const lm::io::TrajectoryLimits::TrajectoryLimit& limitBuf)
{
    state.mutable_limit_reached()->CopyFrom(limitBuf);
}

void Trajectory::setState(const lm::io::TrajectoryState& newState)
{
    state.CopyFrom(newState);
}

void Trajectory::setStatus(status_t newStatus)
{
    status = newStatus;
}

}
}
