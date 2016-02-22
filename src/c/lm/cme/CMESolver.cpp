/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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
#include <limits>
#include <list>
#include <map>
#include <string>
#if defined(MACOSX)
#elif defined(LINUX)
#include <time.h>
#endif

#include <cstdio>

#include "lm/Math.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/cme/CMESolver.h"
#include "lm/cme/ReactionModel.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/me/PropensityFunction.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/oparam/OrderParameterFunction.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/rng/XORShift.h"
#ifdef OPT_CUDA
#include "lm/rng/XORWow.h"
#endif
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lm/Tune.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using std::string;
using std::list;
using std::map;

namespace lm {
namespace cme {

CMESolver::CMESolver(RandomGenerator::Distributions neededDists)
:neededDists(neededDists),rng(NULL),reactionModel(NULL),hasUpdateSpeciesCountsListeners(false),tilings(NULL),numberOrderParameters(0),orderParameterFunctions(NULL),status(lm::message::WorkUnitStatus::NONE),timeLimit(std::numeric_limits<double>::infinity()),numberLimits(0),limits(NULL),limitIndexReached(-1),limitReached(lm::io::TrajectoryLimits::NONE),writeSpeciesTimeSeries(false),speciesWriteInterval(0.0),numberFptTrackedSpecies(0),fptTrackedSpecies(NULL),trajectoryStarted(false),speciesCounts(NULL),time(0.0),timeStep(0.0),degreeAdvancements(NULL),orderParameterValues(NULL),orderParameterPreviousValues(NULL),tilingHists(NULL)
{
}

CMESolver::~CMESolver()
{
    // Free any model memory.
    if (reactionModel != NULL) delete reactionModel; reactionModel = NULL;

    // Free any memory associated with the state.
    if (degreeAdvancements != NULL) delete[] degreeAdvancements; degreeAdvancements = NULL;
    if (orderParameterFunctions != NULL) delete orderParameterFunctions; orderParameterFunctions = NULL;
    if (orderParameterValues != NULL) delete orderParameterValues; orderParameterValues = NULL;
    if (orderParameterPreviousValues != NULL) delete orderParameterPreviousValues; orderParameterPreviousValues = NULL;
    if (speciesCounts != NULL) delete[] speciesCounts; speciesCounts = NULL;
    if (tilings != NULL) delete tilings; tilings = NULL;

    // Free any other memory.
    if (rng != NULL) delete rng; rng = NULL;
    if (limits != NULL) delete[] limits; limits = NULL;
    if (fptTrackedSpecies != NULL) delete[] fptTrackedSpecies; fptTrackedSpecies = NULL;
    if (tilingHists!=NULL) delete[] tilingHists; tilingHists = NULL;
}

void CMESolver::setComputeResources(vector<int> cpus, vector<int> gpus)
{
    MESolver::setComputeResources(cpus, gpus);

    // Create the appropriate RNG.
    if (neededDists != RandomGenerator::NONE)
    {
        if (gpus.size() > 0)
        {
            #ifdef OPT_CUDA
            // Create the cuda based rng.
            rng = new lm::rng::XORWow(gpus[0], 0, 0, neededDists);
            #endif
        }

        if (rng == NULL)
        {
            rng = new lm::rng::XORShift(0, 0);
        }
    }
}

void CMESolver::setReactionModel(const lm::io::ReactionModel& rm)
{
    if (rm.number_reactions() != (uint)rm.reaction_size()) throw InvalidArgException("rm", "number of reaction does not agree with reaction list size");

    // Set the new reaction model.
    if (reactionModel != NULL) delete reactionModel;
    reactionModel = new ReactionModel(rm);

    // Allocate space for the species counts.
    if (speciesCounts != NULL) delete[] speciesCounts; speciesCounts = NULL;
    speciesCounts = new int[reactionModel->numberSpecies];
}

void CMESolver::setOrderParameters(const lm::io::OrderParameters& ops)
{
    if (orderParameterFunctions != NULL) delete orderParameterFunctions; orderParameterFunctions = NULL;
    if (orderParameterValues != NULL) delete orderParameterValues; orderParameterValues = NULL;
    if (orderParameterPreviousValues != NULL) delete orderParameterPreviousValues; orderParameterPreviousValues = NULL;

    // Allocate space for the order parameters.
    numberOrderParameters = ops.order_parameters_size();
    orderParameterFunctions = new lm::oparam::OrderParameterFunction*[reactionModel->numberSpecies];
    orderParameterValues = new double[numberOrderParameters];
    orderParameterPreviousValues = new double[numberOrderParameters];

    // Create the order parameter functions.
    lm::oparam::OrderParameterFunctionFactory fs;
    for (size_t i=0; i<numberOrderParameters; i++)
        orderParameterFunctions[i] = fs.createOrderParameterFunction(ops.order_parameters(i));

    // Mark that we have a listener to update whenever the speices counts changes.
    hasUpdateSpeciesCountsListeners = true;
}

void CMESolver::setTilings(const lm::io::Tilings& tilingsBuf)
{
    if (tilings != NULL) delete tilings; tilings = NULL;
    tilings = new lm::tiling::Tilings();
    tilings->init(tilingsBuf);
    hasUpdateSpeciesCountsListeners = true;
}

void CMESolver::setLimits(const lm::io::TrajectoryLimits& lm)
{
    // Free any previous limits;
    if (limits != NULL) delete[] limits; limits = NULL;

    // Set the time limit.
    if (lm.has_max_time_limit())
        timeLimit = lm.max_time_limit();
    else
        timeLimit = std::numeric_limits<double>::infinity();

    // Count the other limits.
    numberLimits = 0;
    numberLimits += lm.min_species_count_limit_size();
    numberLimits += lm.max_species_count_limit_size();
    numberLimits += lm.decreasing_order_parameter_limit_size();
    numberLimits += lm.increasing_order_parameter_limit_size();
    if (numberLimits > 0)
    {
        limits = new TrajectoryLimit[numberLimits];

        int limitIndex=0;
        for (int i=0; i<lm.min_species_count_limit_size(); i++, limitIndex++)
        {
            limits[limitIndex].type = lm::io::TrajectoryLimits::MINSPECIESCOUNT;
            limits[limitIndex].id = lm.min_species_count_limit(i).species_id();
            limits[limitIndex].ivalue = lm.min_species_count_limit(i).value();
        }
        for (int i=0; i<lm.max_species_count_limit_size(); i++, limitIndex++)
        {
            limits[limitIndex].type = lm::io::TrajectoryLimits::MAXSPECIESCOUNT;
            limits[limitIndex].id = lm.max_species_count_limit(i).species_id();
            limits[limitIndex].ivalue = lm.max_species_count_limit(i).value();
        }
        for (int i=0; i<lm.decreasing_order_parameter_limit_size(); i++, limitIndex++)
        {
            limits[limitIndex].type = lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER;
            limits[limitIndex].id = lm.decreasing_order_parameter_limit(i).order_parameter_id();
            limits[limitIndex].dvalue = lm.decreasing_order_parameter_limit(i).value(0);
            limits[limitIndex].arrangement = lm.decreasing_order_parameter_limit(i).arrangement();
        }
        for (int i=0; i<lm.increasing_order_parameter_limit_size(); i++, limitIndex++)
        {
            limits[limitIndex].type = lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER;
            limits[limitIndex].id = lm.increasing_order_parameter_limit(i).order_parameter_id();
            limits[limitIndex].dvalue = lm.increasing_order_parameter_limit(i).value(0);
            limits[limitIndex].arrangement = lm.increasing_order_parameter_limit(i).arrangement();
        }

        // Check for consistency.
        if (limitIndex != numberLimits)
            throw Exception("Consistency error in set limits",limitIndex,numberLimits);
    }
}

void CMESolver::reset()
{
    MESolver::reset();

    // Make sure we have a reaction model.
    if (reactionModel == NULL) throw Exception("Tried to reset state of CMESolver with no reaction model.");

    // Reset the degree advancements.
    for (uint i=0; i<numberDegreeAdvancements; i++)
        degreeAdvancements[i] = 0;

    // Reset the fpt tracking list.
    numberFptTrackedSpecies = 0;
    if (fptTrackedSpecies != NULL) delete[] fptTrackedSpecies; fptTrackedSpecies = NULL;

    // Reset the limits reached.
    limitIndexReached = -1;
    limitReached = lm::io::TrajectoryLimits::NONE;

    // Reset the order parameters.
    for (size_t i=0; i<numberOrderParameters; i++)
    {
        orderParameterValues[i] = 0.0;
        orderParameterPreviousValues[i] = 0.0;
    }

    // Reset the species counts.
    for (uint i=0; i<reactionModel->numberSpecies; i++)
        speciesCounts[i] = 0;

    // Reset the status.
    status = lm::message::WorkUnitStatus::NONE;

    // Reset the tiling histograms list.
    numberTilingHists = 0;
    if (tilingHists!=NULL) delete[] tilingHists; tilingHists = NULL;

    // Reset the time.
    time = 0.0;
    timeStep = 0.0;

    // Reset trajectory started.
    trajectoryStarted = false;
}

void CMESolver::getState(lm::io::TrajectoryState* state, uint trajectoryNumber)
{
    if (trajectoryNumber >= getSimultaneousTrajectories()) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());

    // Get the degree advancements.
    if (numberDegreeAdvancements > 0)
    {
        state->mutable_cme_state()->mutable_degree_advancements()->set_trajectory_id(trajectoryId);
        state->mutable_cme_state()->mutable_degree_advancements()->set_number_reactions(reactionModel->numberReactions);
        state->mutable_cme_state()->mutable_degree_advancements()->set_number_entries(1);
        for (int i=0; i<numberDegreeAdvancements; i++)
        {
            state->mutable_cme_state()->mutable_degree_advancements()->add_degree_advancements(degreeAdvancements[i]);
        }
        state->mutable_cme_state()->mutable_degree_advancements()->add_time(time);
    }

    // Get the first passage times.
    for (int i=0; i<numberFptTrackedSpecies; i++)
    {
        fptTrackedSpecies[i].serializeTo(trajectoryId, state->mutable_cme_state()->add_first_passage_times());
    }

    // Get the limit reached during the simulation.
    state->set_limit_index_reached(limitIndexReached);
    state->set_limit_reached(limitReached);

//    // Get the order parameter values.
//    if (numberOrderParameters > 0)
//    {
//        state->mutable_cme_state()->mutable_order_parameter_counts()->set_trajectory_id(trajectoryId);
//        state->mutable_cme_state()->mutable_order_parameter_counts()->set_number_order_parameters(numberOrderParameters);
//        state->mutable_cme_state()->mutable_order_parameter_counts()->set_number_entries(1);
//        for (int i=0; i<numberOrderParameters; i++)
//        {
//            state->mutable_cme_state()->mutable_order_parameter_counts()->add_order_parameter_counts(orderParameterCounts[i]);
//        }
//        state->mutable_cme_state()->mutable_order_parameter_counts()->add_time(time);
//    }

    // Get the species counts.
    state->mutable_cme_state()->mutable_species_counts()->set_trajectory_id(trajectoryId);
    state->mutable_cme_state()->mutable_species_counts()->set_number_species((int)reactionModel->numberSpecies);
    state->mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
    for (int i=0; i<(int)reactionModel->numberSpecies; i++)
    {
        state->mutable_cme_state()->mutable_species_counts()->add_species_count(speciesCounts[i]);
    }

    // Get the tiling hists
    state->mutable_cme_state()->clear_tiling_hists();
    for (uint i=0;i<numberTilingHists;i++)
    {
        tilingHists[i].serializeTo(state->mutable_cme_state()->add_tiling_hists());
    }

    // Get the time.
    state->mutable_cme_state()->mutable_species_counts()->add_time(time);

    // Get the trajectory id.
    state->set_trajectory_id(trajectoryId);
    state->set_trajectory_started(true);
}

void CMESolver::setState(const lm::io::TrajectoryState& state, uint trajectoryNumber)
{
    if (trajectoryNumber >= getSimultaneousTrajectories()) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());

    // Validate the state.
    if (!state.has_cme_state()) throw Exception("State object does not contain the necessary data to initialize the solver.");
    if (state.cme_state().species_counts().number_species() != (int)reactionModel->numberSpecies) throw Exception("State object and reaction model have differing species count",state.cme_state().species_counts().number_species(),reactionModel->numberSpecies);
    if (state.cme_state().species_counts().number_entries() != 1 || state.cme_state().species_counts().species_count_size() != (int)reactionModel->numberSpecies || state.cme_state().species_counts().time_size() != 1) throw Exception("State object has too many entries",state.cme_state().species_counts().number_entries());

    // Set the degree advancements.
    for (int i=0; i<state.cme_state().degree_advancements().degree_advancements_size(); i++)
    {
        degreeAdvancements[i] = state.cme_state().degree_advancements().degree_advancements(i);
    }

    // Set the first passage times.
    numberFptTrackedSpecies = state.cme_state().first_passage_times_size();
    if (numberFptTrackedSpecies > 0)
    {
        fptTrackedSpecies = new FPTTracking[numberFptTrackedSpecies];
        for (int i=0; i<numberFptTrackedSpecies; i++)
        {
            fptTrackedSpecies[i].species = state.cme_state().first_passage_times(i).species();
            fptTrackedSpecies[i].minValueAchieved = state.cme_state().first_passage_times(i).species_count(0);
            fptTrackedSpecies[i].maxValueAchieved = state.cme_state().first_passage_times(i).species_count(state.cme_state().first_passage_times(i).number_entries()-1);
            for (int j=0; j<state.cme_state().first_passage_times(i).number_entries(); j++)
            {
                fptTrackedSpecies[i].fptValues.push_back(std::pair<int,double>(state.cme_state().first_passage_times(i).species_count(j),state.cme_state().first_passage_times(i).first_passage_time(j)));
            }
        }
        hasUpdateSpeciesCountsListeners = true;
    }

    // Set the limit reached during the simulation.
    limitIndexReached = state.limit_index_reached();
    limitReached = state.limit_reached();

//    // Set the order parameter values.
//    for (int i=0; i<state.cme_state().order_parameter_counts().order_parameter_counts_size(); i++)
//    {
//        orderParameterValues[i] = state.cme_state().order_parameter_counts().order_parameter_counts(i);
//        orderParameterPreviousValues[i] = orderParameterValues[i];
//    }

    // Set the order parameters.
    for (int i=0; i<numberOrderParameters; i++)
    {
        orderParameterValues[i] = orderParameterFunctions[i]->calculate(time, speciesCounts, reactionModel->numberSpecies);
        orderParameterPreviousValues[i] = orderParameterValues[i];
    }

    // Set the species counts.
    for (int i=0; i<state.cme_state().species_counts().species_count_size(); i++)
    {
        speciesCounts[i] = state.cme_state().species_counts().species_count(i);
    }

    // Set the histogram bin values.
    numberTilingHists = state.cme_state().tiling_hists_size();
    if (state.cme_state().tiling_hists_size() > 0)
    {
        tilingHists = new TilingHist[numberTilingHists];
        for (uint i=0;i<numberTilingHists;i++)
        {
            tilingHists[i] = TilingHist();
            tilingHists[i].init(state.cme_state().tiling_hists(i));
        }
    }

    // Set the time.
    time = state.cme_state().species_counts().time(0);
    trajectoryStarted = state.trajectory_started();

    // Set the trajectory id.
    trajectoryId = state.trajectory_id();
}

lm::message::WorkUnitStatus::Status CMESolver::getStatus(uint trajectoryNumber)
{
    if (trajectoryNumber >= getSimultaneousTrajectories()) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());
    return status;
}

void CMESolver::setOutputOptions(const lm::io::OutputOptions& outputOptions)
{
    if (outputOptions.has_species_write_interval())
    {
        writeSpeciesTimeSeries = true;
        speciesWriteInterval = outputOptions.species_write_interval();
    }
}

bool CMESolver::isTrajectoryOutsideLimits()
{
    for (uint i=0; i<numberLimits; i++)
    {
        TrajectoryLimit& l = limits[i];
        switch (l.type)
        {
        case lm::io::TrajectoryLimits::MINSPECIESCOUNT:
            if (speciesCounts[l.id] <= l.ivalue)
            {
                status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                limitReached = l.type;
                return true;
            }
            break;
        case lm::io::TrajectoryLimits::MAXSPECIESCOUNT:
            if (speciesCounts[l.id] >= l.ivalue)
            {
                status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                limitReached = l.type;
                return true;
            }
            break;
        case lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER:
            if (l.arrangement == lm::io::TrajectoryLimits::ASCENDING)
            {
                if (orderParameterPreviousValues[l.id] >= l.dvalue && orderParameterValues[l.id] < l.dvalue)
                {
                    status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                    limitReached = l.type;
                    return true;
                }
            }
            else
            {
                if (orderParameterPreviousValues[l.id] > l.dvalue && orderParameterValues[l.id] <= l.dvalue)
                {
                    status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                    limitReached = l.type;
                    return true;
                }
            }
            break;
        case lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER:
            if (l.arrangement == lm::io::TrajectoryLimits::ASCENDING)
            {
                if (orderParameterPreviousValues[l.id] < l.dvalue && orderParameterValues[l.id] >= l.dvalue)
                {
                    status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                    limitReached = l.type;
                    return true;
                }
            }
            else
            {
                if (orderParameterPreviousValues[l.id] <= l.dvalue && orderParameterValues[l.id] > l.dvalue)
                {
                    status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                    limitReached = l.type;
                    return true;
                }
            }
            break;
        default:
            break;
        }
    }
    return false;
}

}
}
