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
#include <algorithm>
#include <cmath>
#include <limits>
#include <list>
#include <map>
#include <string>

#include "lm/cme/CMESolver.h"
#include "lm/cme/ReactionModel.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/input/OrderParameters.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/Math.h"
#include "lm/me/PropensityFunction.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/oparam/OrderParameterFunction.h"
#include "lm/Print.h"
#include "lm/protowrap/Repeated.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/rng/XORShift.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lm/Tune.h"
#include "lm/Types.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

#if defined(MACOSX)
#elif defined(LINUX)
#include <time.h>
#endif
#ifdef OPT_CUDA
#include "lm/rng/XORWow.h"
#endif

using lm::limit::TrackingMapT;
using lm::protowrap::Repeated;
using std::list;
using std::map;
using std::string;

namespace lm {
namespace cme {

CMESolver::CMESolver(RandomGenerator::Distributions neededDists)
:neededDists(neededDists),rng(NULL),reactionModel(NULL),hasUpdateSpeciesCountsListeners(false),tilings(NULL),numberDegreeAdvancements(0),
 numberOrderParameters(0),orderParameterFunctions(NULL),status(lm::message::WorkUnitStatus::NONE),timeLimit(std::numeric_limits<double>::infinity()),
 numberLimits(0),limits(NULL),limitReached(NULL),limitIDReached(lm::limit::TrajectoryLimits::DEFAULT_LIMIT_ID),
 limitTypeReached(lm::input::TrajectoryLimit::NONE),workUnitCondenseOutput(false),writeDegreeAdvancementTimeSeries(false),
 writeOrderParameterTimeSeries(false),writeSpeciesTimeSeries(false),degreeAdvancementWriteInterval(0.0),orderParameterWriteInterval(0.0),
 speciesWriteInterval(0.0),numberFptTrackedSpecies(0),numberFptTrackedOrderParameters(0),fptTrackedSpecies(NULL),fptTrackedOrderParameters(NULL),
 trajectoryStarted(false),speciesCounts(NULL), time(0.0),timeStep(0.0),degreeAdvancements(NULL),orderParameterValues(NULL),
 orderParameterPreviousValues(NULL),tilingHists(NULL)
{
}

CMESolver::~CMESolver()
{
    // Free any model memory.
    if (reactionModel != NULL) delete reactionModel; reactionModel = NULL;

    // Free any memory associated with the state.
    if (degreeAdvancements != NULL) delete[] degreeAdvancements; degreeAdvancements = NULL;
    if (orderParameterFunctions != NULL)
    {
        for (int i=0;i<numberOrderParameters;i++) delete orderParameterFunctions[i];
        delete[] orderParameterFunctions; orderParameterFunctions = NULL;
    }
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

void CMESolver::setReactionModel(const lm::input::ReactionModel& rm)
{
    if (rm.number_reactions() != (uint)rm.reaction_size()) throw InvalidArgException("rm", "number of reaction does not agree with reaction list size");

    // Set the new reaction model.
    if (reactionModel != NULL) delete reactionModel;
    reactionModel = new ReactionModel(rm);

    // Allocate space for the degree advancement counts, if we're writing them.
    if (degreeAdvancements != NULL) delete[] degreeAdvancements; degreeAdvancements = NULL;
    degreeAdvancements = new uint64_t[reactionModel->numberReactions];

    // Allocate space for the species counts.
    if (speciesCounts != NULL) delete[] speciesCounts; speciesCounts = NULL;
    speciesCounts = new int[reactionModel->numberSpecies];
}

void CMESolver::setOrderParameters(const lm::input::OrderParameters& ops)
{
    if (orderParameterFunctions != NULL)
    {
        for (int i=0;i<numberOrderParameters;i++) delete orderParameterFunctions[i];
        delete[] orderParameterFunctions; orderParameterFunctions = NULL;
    }
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

void CMESolver::setTilings(const lm::input::Tilings& tilingsBuf)
{
    if (tilings != NULL) delete tilings; tilings = NULL;

    // TODO: reimplement?
//    tilings = new lm::tiling::Tilings();
//    tilings->init(tilingsBuf);
//    hasUpdateSpeciesCountsListeners = true;
}

void CMESolver::setLimits(const lm::input::TrajectoryLimits& lm)
{
    // Free any previous limits;
    if (limits != NULL) delete[] limits; limits = NULL;

    // use the TrajectoryLimits buffer to initialize the TrajectoryLimits object
    trajectoryLimits.rFB(lm);

    // Set the time limit.
    timeLimit = trajectoryLimits.getTimeLimitValue();

    // Count the non-time limits.
    numberLimits = trajectoryLimits.size();

    // If we have any limits, copy them over to a simple array
    if (numberLimits > 0)
    {
        limits = new lm::limit::TrajectoryLimit[numberLimits];
        std::copy(trajectoryLimits.vec().begin(), trajectoryLimits.vec().end(), limits);
    }

    // if any of the limits are degree advancement limits, make sure that we're tracking them
    if (trajectoryLimits.hasDegreeAdvancementLimit())
    {
        numberDegreeAdvancements = reactionModel->numberReactions;
        hasUpdateSpeciesCountsListeners = true;
    }
}

void CMESolver::reset()
{
    MESolver::reset();

    // Make sure we have a reaction model.
    if (reactionModel == NULL) throw Exception("Tried to reset state of CMESolver with no reaction model.");

    // Reset the degree advancements.
    for (uint i=0; i<reactionModel->numberReactions; i++)
        degreeAdvancements[i] = 0;

    // Reset the fpt tracking list.
    numberFptTrackedSpecies = 0;
    if (fptTrackedSpecies != NULL) delete[] fptTrackedSpecies; fptTrackedSpecies = NULL;

    // Reset the fpt tracking list.
    numberFptTrackedOrderParameters = 0;
    if (fptTrackedOrderParameters != NULL) delete[] fptTrackedOrderParameters; fptTrackedOrderParameters = NULL;

    // Reset the limits reached.
    limitIDReached = lm::limit::TrajectoryLimits::DEFAULT_LIMIT_ID;
    limitTypeReached = lm::input::TrajectoryLimit::NONE;

    // Reset the limit tracking.
    trackedLimits.clear();

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
    if (writeDegreeAdvancementTimeSeries)
    {
        state->mutable_cme_state()->mutable_degree_advancements()->set_trajectory_id(trajectoryId);
        state->mutable_cme_state()->mutable_degree_advancements()->set_number_reactions(reactionModel->numberReactions);
        state->mutable_cme_state()->mutable_degree_advancements()->set_number_entries(1);
        for (int i=0; i<reactionModel->numberReactions; i++)
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

    // Get the order parameter first passage times.
    for (int i=0; i<numberFptTrackedOrderParameters; i++)
    {
        fptTrackedOrderParameters[i].serializeTo(state->mutable_cme_state()->add_order_parameter_first_passage_times(), trajectoryId);
    }

    // Get the limit reached during the simulation.
    if (limitTypeReached==lm::input::TrajectoryLimit::TIME)
    {
        state->mutable_limit_reached()->CopyFrom(trajectoryLimits.getTimeLimitMsg());
    }
    else if (limitTypeReached!=lm::input::TrajectoryLimit::NONE)
    {
        state->mutable_limit_reached()->CopyFrom(*trajectoryLimits.findMsg(limitIDReached));
    }

    // if we're recording any limit tracking data to the trajectory state, get it. Otherwise, just get any changes to the limit tracking countdowns
    for (TrackingMapT::const_iterator it=trackedLimits.begin(); it!=trackedLimits.end(); ++it)
    {
        lm::limit::TrajectoryLimit& l = limits[it->second.limit_id];
        if (l.addTrackingToCMEState or l.addTrackingToOutput) throw Exception("LimitTrackingWrap instance created for limit %d, but no tracking was requested for this limit", l.limitID);

        limitTrackingWrap.setWrappedMsg(state->add_limit_trackings());
        if (l.addTrackingToCMEState)
        {
            limitTrackingWrap.serializeFrom(trajectoryId, it->second);
        }
        else
        {
            limitTrackingWrap.serializeMetadataFrom(trajectoryId, it->second);
        }
    }

    // Get the order parameter values.
    if (numberOrderParameters > 0)
    {
        state->mutable_cme_state()->mutable_order_parameter_values()->set_trajectory_id(trajectoryId);
        state->mutable_cme_state()->mutable_order_parameter_values()->set_number_order_parameters(numberOrderParameters);
        state->mutable_cme_state()->mutable_order_parameter_values()->set_number_entries(1);
        for (int i=0; i<numberOrderParameters; i++)
        {
            state->mutable_cme_state()->mutable_order_parameter_values()->add_order_parameter_values(orderParameterValues[i]);
        }
        state->mutable_cme_state()->mutable_order_parameter_values()->add_time(time);
    }

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

    // Set the order parameter first passage times.
    numberFptTrackedOrderParameters = state.cme_state().order_parameter_first_passage_times_size();
    if (numberFptTrackedOrderParameters > 0)
    {
        fptTrackedOrderParameters = new OParamFPTTracking[numberFptTrackedOrderParameters];
        for (int i=0; i<numberFptTrackedOrderParameters; i++)
        {
            fptTrackedOrderParameters[i].deserializeFrom(state.cme_state().order_parameter_first_passage_times(i));
        }
        hasUpdateSpeciesCountsListeners = true;
    }

    // Set the previous limit reached by the associated trajectory, if any.
    if (state.has_limit_reached())
    {
        limitIDReached = state.limit_reached().id();
        limitTypeReached = state.limit_reached().limit_type();
    }

    // if we're tracking any limits, set up the solver to output state information when the limit is reached
    for (Repeated<lm::io::LimitTracking>::const_iterator it=state.limit_trackings().begin(); it!=state.limit_trackings().end(); ++it)
    {
        // TODO: CV! my nemesis. Fix the need for the const_cast here
        limitTrackingWrap.setWrappedMsg(const_cast<lm::io::LimitTracking*>(&*it));
        limitTrackingWrap.deserializeTo(&trackedLimits[it->limit_id()]);
    }

//    // Set the order parameter values.
    for (int i=0; i<state.cme_state().order_parameter_values().order_parameter_values_size(); i++)
    {
        orderParameterValues[i] = state.cme_state().order_parameter_values().order_parameter_values(i);
        orderParameterPreviousValues[i] = orderParameterValues[i];
    }

    // Set the order parameters.
//    for (int i=0; i<numberOrderParameters; i++)
//    {
//        orderParameterValues[i] = orderParameterFunctions[i]->calculate(time, speciesCounts, reactionModel->numberSpecies);
//        orderParameterPreviousValues[i] = orderParameterValues[i];
//    }

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

void CMESolver::setOutputOptions(const lm::input::OutputOptions& outputOptions)
{
    if (outputOptions.has_record_name_prefix()) workUnitOutputPrefix.assign(outputOptions.record_name_prefix());
    if (outputOptions.has_condense_output()) workUnitCondenseOutput = outputOptions.condense_output();
    if (outputOptions.has_degree_advancement_write_interval())
    {
        writeDegreeAdvancementTimeSeries = true;
        degreeAdvancementWriteInterval = outputOptions.degree_advancement_write_interval();

        numberDegreeAdvancements = reactionModel->numberReactions;
        hasUpdateSpeciesCountsListeners = true;
    }
    if (outputOptions.has_order_parameter_write_interval())
    {
        writeOrderParameterTimeSeries = true;
        orderParameterWriteInterval = outputOptions.order_parameter_write_interval();

        hasUpdateSpeciesCountsListeners = true;
    }
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
        lm::limit::TrajectoryLimit& l = limits[i];
        bool limitReached = false;
        
        switch (l.type)
        {
        case TrajLimEnums::NONE: throw Exception("CMESolver tried to check a limit that did not have an associated LimitType"); break;
        case TrajLimEnums::TIME: throw Exception("When checking limits, CMESolver reached a time limit that was mixed in with the other limits"); break;

        case TrajLimEnums::SPECIES:
            switch (l.stoppingCondition)
            {
            case TrajLimEnums::MIN:
                if (l.includeEndpoint)
                {
                    check_limit_MIN_true(speciesCounts[l.valueID], l.ivalue, limitReached)
                }
                else
                {
                    check_limit_MIN_false(speciesCounts[l.valueID], l.ivalue, limitReached)
                }
                break;
            case TrajLimEnums::MAX:
                if (l.includeEndpoint)
                {
                    check_limit_MAX_true(speciesCounts[l.valueID], l.ivalue, limitReached)
                }
                else
                {
                    check_limit_MAX_false(speciesCounts[l.valueID], l.ivalue, limitReached)
                }
                break;
            case TrajLimEnums::DECREASING: throw Exception("unimplemented"); break;
            case TrajLimEnums::INCREASING: throw Exception("unimplemented"); break;
            }
            break;

        case TrajLimEnums::ORDER_PARAMETER:
            switch (l.stoppingCondition)
            {
            case TrajLimEnums::MIN:
                if (l.includeEndpoint)
                {
                    check_limit_MIN_true(orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                else
                {
                    check_limit_MIN_false(orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                break;
            case TrajLimEnums::MAX:
                if (l.includeEndpoint)
                {
                    check_limit_MAX_true(orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                else
                {
                    check_limit_MAX_false(orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                break;
            case TrajLimEnums::DECREASING:
                if (l.includeEndpoint)
                {
                    check_limit_DECREASING_true(orderParameterPreviousValues[l.valueID], orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                else
                {
                    check_limit_DECREASING_false(orderParameterPreviousValues[l.valueID], orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                break;
            case TrajLimEnums::INCREASING:
                if (l.includeEndpoint)
                {
                    check_limit_INCREASING_true(orderParameterPreviousValues[l.valueID], orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                else
                {
                    check_limit_INCREASING_false(orderParameterPreviousValues[l.valueID], orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                break;
            }
            break;

        case TrajLimEnums::DEGREE_ADVANCEMENT:
            switch (l.stoppingCondition)
            {
            case TrajLimEnums::MIN:
                if (l.includeEndpoint)
                {
                    check_limit_MIN_true(degreeAdvancements[l.valueID], l.uvalue, limitReached)
                }
                else
                {
                    check_limit_MIN_false(degreeAdvancements[l.valueID], l.uvalue, limitReached)
                }
                break;
            case TrajLimEnums::MAX:
                if (l.includeEndpoint)
                {
                    check_limit_MAX_true(degreeAdvancements[l.valueID], l.uvalue, limitReached)
                }
                else
                {
                    check_limit_MAX_false(degreeAdvancements[l.valueID], l.uvalue, limitReached)
                }
                break;
            case TrajLimEnums::DECREASING: throw Exception("unimplemented"); break;
            case TrajLimEnums::INCREASING: throw Exception("unimplemented"); break;
            }
            break;

        default: throw Exception("CMESolver tried to check a limit with an unknown LimitType"); break;
        }

        if (limitReached)
        {
            bool terminationSignaled;
            // if this limit is being tracked, handle that
            if (trackedLimits.count(l.limitID))
            {
                lm::limit::LimitTracking& limitTracking = trackedLimits[l.limitID];
                limitTracking.trackLimit();

                // track the state if limitTracking is enabled
                if (limitTracking.trackingEnabled(l.trackCount))
                {
                    if (numberDegreeAdvancements>0) {for (int j=0;j<numberDegreeAdvancements;j++) limitTracking.degree_advancements.push_back(degreeAdvancements[j]);}
                    if (numberOrderParameters>0) {for (int j=0;j<numberOrderParameters;j++) limitTracking.order_parameter_values.push_back(orderParameterValues[j]);}
                    for (int j=0;j<reactionModel->numberSpecies;j++) limitTracking.species_counts.push_back(speciesCounts[j]);
                    limitTracking.times.push_back(time);
                }

                // if we are done with tracking, terminate the trajectory
                terminationSignaled = limitTracking.terminationSignaled(l.trackCount);
            }
            // if this limit is not being tracked, just signal for termination of the trajectory
            else
            {
                terminationSignaled = true;
            }

            if (terminationSignaled)
            {
                status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                limitIDReached = l.limitID;
                limitTypeReached = l.type;
                return true;
            }
        }
    }
    return false;
}

}
}
