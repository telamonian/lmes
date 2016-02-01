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
:neededDists(neededDists),rng(NULL),reactionModel(NULL),oparams(NULL),status(lm::message::WorkUnitStatus::NONE),timeLimit(std::numeric_limits<double>::infinity()),numberLimits(0),limits(NULL),limitReached(lm::io::TrajectoryLimits::NONE),trajectoryStarted(false),speciesCounts(NULL),time(0.0),timeStep(0.0),numberFptTrackedSpecies(0),fptTrackedSpecies(NULL),tilingHists(NULL)
{
}

CMESolver::~CMESolver()
{
    // Free any model memory.
    if (reactionModel != NULL) delete reactionModel; reactionModel = NULL;

    // Free any memory associated with the state.
    if (speciesCounts != NULL) delete[] speciesCounts; speciesCounts = NULL;

    // Free any memory being used by the order parameters
    if (oparams != NULL) delete oparams; oparams = NULL;

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
    oparams = new lm::oparam::OParams();
    oparams->init(opsBuf);
}

void CMESolver::setTilings(const lm::io::Tilings& tilingsBuf)
{
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
    if (needsOrderParameters())
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
    if (needsOrderParameters())
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
    // Count the limits.
    numberLimits = 0;
    numberLimits += limits.min_species_count_limit_size();
    numberLimits += limits.max_species_count_limit_size();
    numberLimits += limits.decreasing_order_parameter_limit_size();
    numberLimits += limits.increasing_order_parameter_limit_size();
    if (numberLimits > 0)
        this->limits = new TrajectoryLimit[numberLimits];
}

/*
void CMESolver::setSpeciesLowerLimit(int species, int limit)
{
    // Allocate a larger list for the limits/limit crossings.
    SpeciesLimit * newSpeciesLimits = new SpeciesLimit[++numberSpeciesLimits];
    if (numberSpeciesLimits > 1)
    {
        for (uint i=0; i<numberSpeciesLimits-1; i++)
            newSpeciesLimits[i] = speciesLimits[i];
        delete[] speciesLimits;
    }
    speciesLimits = newSpeciesLimits;
    speciesLimits[numberSpeciesLimits-1].type = SpeciesLimit::MIN;
    speciesLimits[numberSpeciesLimits-1].species = species;
    speciesLimits[numberSpeciesLimits-1].limit = limit;
}

void CMESolver::setSpeciesUpperLimit(int species, int limit)
{
    // Allocate a larger list for the limits/limit crossings.
    SpeciesLimit * newSpeciesLimits = new SpeciesLimit[++numberSpeciesLimits];
    if (numberSpeciesLimits > 1)
    {
        for (uint i=0; i<numberSpeciesLimits-1; i++)
            newSpeciesLimits[i] = speciesLimits[i];
        delete[] speciesLimits;
    }
    speciesLimits = newSpeciesLimits;
    speciesLimits[numberSpeciesLimits-1].type = SpeciesLimit::MAX;
    speciesLimits[numberSpeciesLimits-1].species = species;
    speciesLimits[numberSpeciesLimits-1].limit = limit;
}

void CMESolver::setSpeciesDecreasingLimit(lm::io::TrajectoryLimits::Arrangement arr, int opID, double limit)
{
	// Allocate a larger list for the limits/limit crossings.
	SpeciesLimit* newSpeciesLimits = new SpeciesLimit[++numberSpeciesLimits];
	if (numberSpeciesLimits > 1)
	{
		memcpy(newSpeciesLimits, speciesLimits, sizeof(SpeciesLimit)*(numberSpeciesLimits-1));
		delete[] speciesLimits;
	}
	speciesLimits = newSpeciesLimits;
	speciesLimits[numberSpeciesLimits-1].type = (arr==lm::io::TrajectoryLimits::ASCENDING) ? SpeciesLimit::DECREASING_ASCENDING : SpeciesLimit::DECREASING_DESCENDING;
	speciesLimits[numberSpeciesLimits-1].species = opID;
	speciesLimits[numberSpeciesLimits-1].limit = limit;
}

void CMESolver::setSpeciesIncreasingLimit(lm::io::TrajectoryLimits::Arrangement arr, int opID, double limit)
{
	// Allocate a larger list for the limits/limit crossings.
	SpeciesLimit* newSpeciesLimits = new SpeciesLimit[++numberSpeciesLimits];
	if (numberSpeciesLimits > 1)
	{
		memcpy(newSpeciesLimits, speciesLimits, sizeof(SpeciesLimit)*(numberSpeciesLimits-1));
		delete[] speciesLimits;
	}
	speciesLimits = newSpeciesLimits;
	speciesLimits[numberSpeciesLimits-1].type = (arr==lm::io::TrajectoryLimits::ASCENDING) ? SpeciesLimit::INCREASING_ASCENDING : SpeciesLimit::INCREASING_DESCENDING;
	speciesLimits[numberSpeciesLimits-1].species = opID;
	speciesLimits[numberSpeciesLimits-1].limit = limit;
}
*/

bool CMESolver::isTrajectoryOutsideLimits()
{
    /*
    for (uint i=0; i<numberSpeciesLimits; i++)
    {
        SpeciesLimit l = speciesLimits[i];
        switch (l.type)
        {
        case SpeciesLimit::MIN:
            if (int(speciesCounts[l.species]) <= l.limit)
            {
                finalLimitType = lm::io::TrajectoryLimits::MINSPECIESCOUNT;
                return true;
            }
            break;
        case SpeciesLimit::MAX:
            if (int(speciesCounts[l.species]) >= l.limit)
            {
                finalLimitType = lm::io::TrajectoryLimits::MAXSPECIESCOUNT;
                return true;
            }
            break;
        // use the ASCENDING limit checks when starting to the left of the limit
        case SpeciesLimit::DECREASING_ASCENDING:
            if ((*oparams)[l.species]->getPrev() >= l.limit && (*oparams)[l.species]->get() < l.limit)
            {
                finalLimitType = lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER;
                return true;
            }
            break;
        case SpeciesLimit::INCREASING_ASCENDING:
            if ((*oparams)[l.species]->getPrev() < l.limit && (*oparams)[l.species]->get() >= l.limit)
            {
                finalLimitType = lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER;
                return true;
            }
            break;
        // use the DESCENDING limit checks when starting to the right of the limit
        case SpeciesLimit::DECREASING_DESCENDING:
            if ((*oparams)[l.species]->getPrev() > l.limit && (*oparams)[l.species]->get() <= l.limit)
            {
                finalLimitType = lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER;
                return true;
            }
            break;
        case SpeciesLimit::INCREASING_DESCENDING:
            if ((*oparams)[l.species]->getPrev() <= l.limit && (*oparams)[l.species]->get() > l.limit)
            {
                finalLimitType = lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER;
                return true;
            }
            break;
        }

    }*/
    //status = lm::message::WorkUnitStatus::LIMIT_REACHED;
    return false;
}

}
}
