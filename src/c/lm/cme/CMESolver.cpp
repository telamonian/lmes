/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2015 Roberts Group,
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
#include "lm/oparam/OParams.h"
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
:neededDists(neededDists),rng(NULL),reactionModel(NULL),oparams(NULL),tilings(NULL),status(lm::message::WorkUnitStatus::NONE),timeLimit(std::numeric_limits<double>::infinity()),numberLimits(0),limits(NULL),limitReached(lm::io::TrajectoryLimits::NONE),writeSpeciesTimeSeries(false),speciesWriteInterval(0.0),numberFptTrackedSpecies(0),fptTrackedSpecies(NULL),trajectoryStarted(false),speciesCounts(NULL),time(0.0),timeStep(0.0),tilingHists(NULL)
{
}

CMESolver::~CMESolver()
{
    // Free any model memory.
    if (reactionModel != NULL) delete reactionModel; reactionModel = NULL;

    // Free any memory associated with the state.
    if (speciesCounts != NULL) delete[] speciesCounts; speciesCounts = NULL;
    if (oparams != NULL) delete oparams; oparams = NULL;
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
}

void CMESolver::setOrderParameters(const lm::io::OrderParameters& opsBuf)
{
    if (oparams != NULL) delete oparams; oparams = NULL;
    oparams = new lm::oparam::OParams();
    oparams->init(opsBuf);
}

void CMESolver::setTilings(const lm::io::Tilings& tilingsBuf)
{
    if (tilings != NULL) delete tilings; tilings = NULL;
    tilings = new lm::tiling::Tilings();
    tilings->init(tilingsBuf);
}


void CMESolver::reset()
{
    MESolver::reset();

    // Free any previous state.
    if (speciesCounts != NULL)
    {
        delete[] speciesCounts;
    }
    speciesCounts = NULL;

    // Make sure we have a reaction model.
    if (reactionModel == NULL) throw Exception("Tried to reset state of CMESolver with no reaction model.");

    // Allocate space for the new state.
    speciesCounts = new int[reactionModel->numberSpecies];

    // Reset the species counts.
    for (uint i=0; i<reactionModel->numberSpecies; i++)
    {
        speciesCounts[i] = 0;
    }

    // Reinitialize the order parameters, if required
    if (oparams != NULL)
    {
        oparams->initValues((uint*)speciesCounts);
    }

    // Reset the status.
    status = lm::message::WorkUnitStatus::NONE;

    // Reset the time.
    time = 0.0;

    // Reset the limits reached.
    limitReached = lm::io::TrajectoryLimits::NONE;

    // Reset the fpt tracking list.
    numberFptTrackedSpecies = 0;
    if (fptTrackedSpecies != NULL) delete[] fptTrackedSpecies; fptTrackedSpecies = NULL;

    // Reset the tiling histograms list.
    numberTilingHists = 0;
    if (tilingHists!=NULL) delete[] tilingHists; tilingHists = NULL;
}

void CMESolver::getState(lm::io::TrajectoryState* state)
{
    // Get the trajectory id.
    state->set_trajectory_id(trajectoryId);
    state->set_trajectory_started(true);

    // Get the species counts.
    state->mutable_cme_state()->mutable_species_counts()->set_trajectory_id(trajectoryId);
    state->mutable_cme_state()->mutable_species_counts()->set_number_species((int)reactionModel->numberSpecies);
    state->mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
    for (int i=0; i<(int)reactionModel->numberSpecies; i++)
    {
        state->mutable_cme_state()->mutable_species_counts()->add_species_count(speciesCounts[i]);
    }
    state->mutable_cme_state()->mutable_species_counts()->add_time(time);

    // Get the first passage times.
    for (int i=0; i<numberFptTrackedSpecies; i++)
    {
        fptTrackedSpecies[i].serializeTo(trajectoryId, state->mutable_cme_state()->add_first_passage_times());
    }

    // Get the tiling hists
    state->mutable_cme_state()->clear_tiling_hists();
    for (uint i=0;i<numberTilingHists;i++)
    {
        tilingHists[i].serializeTo(state->mutable_cme_state()->add_tiling_hists());
    }

    // Set the limit reached during the simulation.
    state->set_limit_reached(limitReached);
}

void CMESolver::setState(const lm::io::TrajectoryState& state)
{
    // Validate the state.
    if (!state.has_cme_state()) throw Exception("State object does not contain the necessary data to initialize the solver.");
    if (state.cme_state().species_counts().number_species() != (int)reactionModel->numberSpecies) throw Exception("State object and reaction model have differing species count",state.cme_state().species_counts().number_species(),reactionModel->numberSpecies);
    if (state.cme_state().species_counts().number_entries() != 1 || state.cme_state().species_counts().species_count_size() != (int)reactionModel->numberSpecies || state.cme_state().species_counts().time_size() != 1) throw Exception("State object has too many entries",state.cme_state().species_counts().number_entries());

    // Set the trajectory id.
    trajectoryId = state.trajectory_id();

    // Set the species counts.
    for (int i=0; i<state.cme_state().species_counts().species_count_size(); i++)
    {
        speciesCounts[i] = state.cme_state().species_counts().species_count(i);
    }

    // Reinitialize the order parameters, if required
    if (oparams !=  NULL)
    {
        oparams->initValues((uint*)speciesCounts);
    }

    time = state.cme_state().species_counts().time(0);
    trajectoryStarted = state.trajectory_started();

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
    }

    // Set the histogram bin values
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
}

void CMESolver::setLimits(const lm::io::TrajectoryLimits& limits)
{
    // Set the time limit, if we have one.
    if (limits.has_max_time_limit())
        timeLimit = limits.max_time_limit();

    // Count the other limits.
    numberLimits = 0;
    numberLimits += limits.min_species_count_limit_size();
    numberLimits += limits.max_species_count_limit_size();
    numberLimits += limits.decreasing_order_parameter_limit_size();
    numberLimits += limits.increasing_order_parameter_limit_size();
    if (numberLimits > 0)
        this->limits = new TrajectoryLimit[numberLimits];

    int limitIndex=0;
    for (int i=0; i<limits.min_species_count_limit_size(); i++, limitIndex++)
    {
        this->limits[limitIndex].type = lm::io::TrajectoryLimits::MINSPECIESCOUNT;
        this->limits[limitIndex].id = limits.min_species_count_limit(i).species_id();
        this->limits[limitIndex].ivalue = limits.min_species_count_limit(i).value();
    }
    for (int i=0; i<limits.max_species_count_limit_size(); i++, limitIndex++)
    {
        this->limits[limitIndex].type = lm::io::TrajectoryLimits::MAXSPECIESCOUNT;
        this->limits[limitIndex].id = limits.max_species_count_limit(i).species_id();
        this->limits[limitIndex].ivalue = limits.max_species_count_limit(i).value();
    }
    for (int i=0; i<limits.decreasing_order_parameter_limit_size(); i++, limitIndex++)
    {
        this->limits[limitIndex].type = lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER;
        this->limits[limitIndex].id = limits.decreasing_order_parameter_limit(i).order_parameter_id();
        this->limits[limitIndex].dvalue = limits.decreasing_order_parameter_limit(i).value(0);
        this->limits[limitIndex].arrangement = limits.decreasing_order_parameter_limit(i).arrangement();
    }
    for (int i=0; i<limits.increasing_order_parameter_limit_size(); i++, limitIndex++)
    {
        this->limits[limitIndex].type = lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER;
        this->limits[limitIndex].id = limits.increasing_order_parameter_limit(i).order_parameter_id();
        this->limits[limitIndex].dvalue = limits.increasing_order_parameter_limit(i).value(0);
        this->limits[limitIndex].arrangement = limits.increasing_order_parameter_limit(i).arrangement();
    }

    // Check for consistency.
    if (limitIndex != numberLimits)
        throw Exception("Consistency error in set limits",limitIndex,numberLimits);
}

void CMESolver::setOutputOptions(const lm::io::OutputOptions& outputOptions)
{
    if (outputOptions.has_species_write_interval())
    {
        writeSpeciesTimeSeries = true;
        speciesWriteInterval = outputOptions.species_write_interval();
    }
}

void CMESolver::performReactionEvent(uint r)
{
    // Update the counts according to the dependency tables.
    for (int i=0; i<(int)reactionModel->numberDependentSpecies[r]; i++)
    {
        speciesCounts[reactionModel->dependentSpecies[r][i]] += reactionModel->dependentSpeciesChange[r][i];
        updatedSpeciesCounts();
    }

    // Update any order parameters.
    if (oparams != NULL)
    {
        for (uint i=0; i<oparams->size(); i++)
        {
            (*oparams)[i]->calc((uint*)speciesCounts);
        }
    }

    // Update any tilingHists.
    if (tilings != NULL)
    {
        for (int i=0;i<numberTilingHists;i++)
        {
            tilingHists[i].tileVals[(*tilings)[tilingHists[i].tilingID]->getTileIndex((*oparams)[(*tilings)[tilingHists[i].tilingID]->getOrderParameterID()]->get())] += timeStep;
        }
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
                if ((*oparams)[l.id]->getPrev() >= l.dvalue && (*oparams)[l.id]->get() < l.dvalue)
                {
                    status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                    limitReached = limits[i].type;
                    return true;
                }
            }
            else
            {
                if ((*oparams)[l.id]->getPrev() > l.dvalue && (*oparams)[l.id]->get() <= l.dvalue)
                {
                    status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                    limitReached = limits[i].type;
                    return true;
                }
            }
            break;
        case lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER:
            if (l.arrangement == lm::io::TrajectoryLimits::ASCENDING)
            {
                if ((*oparams)[l.id]->getPrev() < l.dvalue && (*oparams)[l.id]->get() >= l.dvalue)
                {
                    status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                    limitReached = limits[i].type;
                    return true;
                }
            }
            else
            {
                if ((*oparams)[l.id]->getPrev() <= l.dvalue && (*oparams)[l.id]->get() > l.dvalue)
                {
                    status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                    limitReached = limits[i].type;
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
