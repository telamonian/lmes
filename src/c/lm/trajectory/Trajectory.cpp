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
#include <vector>

#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/input/Input.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/tiling/Tilings.h"
#include "lm/trajectory/Trajectory.h"
#include "robertslab/pbuf/NDArraySerializer.h"

using lm::input::DiffusionModel;
using lm::input::ReactionModel;
using lm::io::TrajectoryState;
using lm::tiling::Tilings;
using std::list;
using std::map;
using std::string;
using std::vector;
using robertslab::pbuf::NDArraySerializer;

namespace lm {
namespace trajectory {

Trajectory::Trajectory(uint64_t id, uint64_t phase, const lm::io::TrajectoryState& initialState)
:id(static_cast<uint>(-1)),numberWorkUnitsPerformed(0),simulationPhase(phase),state(initialState),status(NOT_STARTED)
{
    setID(id);
}

Trajectory::Trajectory(uint64_t id, uint64_t phase, const lm::input::Input& input, bool reversed, bool useCMEState, bool useRDMEState, bool useDiffusionPDEState)
:id(id),numberWorkUnitsPerformed(0),simulationPhase(phase),state(),status(NOT_STARTED)
{
    initializeState(input, reversed, useCMEState, useRDMEState, useDiffusionPDEState);
}

Trajectory::~Trajectory()
{
}

void Trajectory::initializeState(const lm::input::Input& input, bool reversed, bool useCMEState, bool useRDMEState, bool useDiffusionPDEState)
{
    state.Clear();
    state.set_trajectory_id(id);
    if (useCMEState) initializeCMEState(input, reversed);
    if (useRDMEState) initializeRDMEState(input);
    if (useDiffusionPDEState) initializeDiffusionPDEState(input);
}

void Trajectory::initializeCMEState(const lm::input::Input& input, bool reversed)
{
    // Set cme state from the reaction model.
    if (input.hasReactionModel())
    {
        // Initialize the species counts
        const lm::input::ReactionModel& reactionModel = input.getReactionModelMsg();
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
        
        // Initialize the degree advancements.
        ndarray<uint64_t> initalDegreeAdvancementCounts(utuple(reactionModel.number_reactions()));
        NDArraySerializer::serializeInto(state.mutable_cme_state()->mutable_degree_advancements(), initalDegreeAdvancementCounts);

        // Initialize the first passage times in the cme state.
        if (input.getOutputOptions().fpt_species_to_track_size() > 0)
        {
            for (int i=0; i<input.getOutputOptions().fpt_species_to_track_size(); i++)
            {
                uint species = input.getOutputOptions().fpt_species_to_track(i);
                lm::io::FirstPassageTimes* fpt = state.mutable_cme_state()->add_first_passage_times();
                fpt->set_trajectory_id(id);
                fpt->set_species(species);
                ndarray<int32_t> counts(utuple(1));
                ndarray<double> times(utuple(1));
                counts[0] = reactionModel.initial_species_count(species);
                times[0] = 0.0;
                NDArraySerializer::serializeInto(fpt->mutable_counts(), counts);
                NDArraySerializer::serializeInto(fpt->mutable_first_passage_times(), times);
            }
        }

        // Initialize the order parameters values
        if (input.hasOrderParameters())
        {
            initializeOrderParameters(input);
        }
        
    }
    else
    {
        throw RuntimeException("Trajectory::initializeCMEState requires a reaction model.");
    }

    // Initialize the tiling hists
    if (input.hasTilings())
    {
        inititializeHists(input);
    }
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
    for (int i=0; i<opv->number_order_parameters(); i++)
    {
        opv->add_order_parameter_values(oparams.at(i)->calc(state));
    }
    opv->add_time(0.0);
}

void Trajectory::initializeRDMEState(const lm::input::Input& input)
{
    // Initialize the rdme state from the diffusion model.
    if (input.hasDiffusionModel())
    {
        // Initialize the lattice state to the initial lattice from the input.
        lm::types::Lattice* initialLattice = state.mutable_rdme_state()->mutable_lattice();
        initialLattice->CopyFrom(input.getDiffusionModelMsg().initial_lattice());
    }
}

void Trajectory::initializeDiffusionPDEState(const lm::input::Input& input)
{
    // Initialize the diffusion pde state from the input.
    if (input.hasMicroenvironmentModel())
    {
        lm::io::DiffusionPDEState* pdeState = state.mutable_diffusion_pde_state();
        pdeState->set_time(0.0);
        pdeState->mutable_concentrations()->CopyFrom(input.getMicroenvironmentModel().initial_concentrations());
    }
    else
    {
        throw RuntimeException("Trajectory::initializeDiffusionPDEState requires a microenvironment model.");
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

lm::io::TrajectoryState* Trajectory::getMutableState()
{
    return &state;
}

int64_t Trajectory::getWorkUnitsPerformed() const
{
    return numberWorkUnitsPerformed;
}

// mutators
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
