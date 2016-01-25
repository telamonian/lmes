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
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/me/PropensityFunctions.h"
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
:neededDists(neededDists),rng(NULL),oparams(NULL),reactionModel(NULL),maxTime(std::numeric_limits<double>::infinity()),numberSpeciesLimits(0),speciesLimits(NULL),numberFptTrackedSpecies(0),fptTrackedSpecies(NULL),tilingHists(NULL),trajectoryStarted(false),speciesCounts(NULL),time(0.0),timeStep(0.0),finalLimitType(static_cast<lm::io::TrajectoryLimits::LimitType>(0))
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
    if (speciesLimits != NULL) delete[] speciesLimits; speciesLimits = NULL;
    if (fptTrackedSpecies != NULL) delete[] fptTrackedSpecies; fptTrackedSpecies = NULL;
    if (tilingHists!=NULL) delete[] tilingHists; tilingHists = NULL;
}

CMESolver::ReactionModel::ReactionModel(uint numberSpecies, uint numberReactions)
:numberSpecies(numberSpecies),numberSpeciesToTrack(numberSpecies),numberReactions(numberReactions),initialSpeciesCounts(NULL),reactionTypes(NULL),S(ndarray<int>(utuple(numberSpecies,numberReactions))),D(ndarray<uint>(utuple(numberSpecies,numberReactions))),propensityFunctionArgs(NULL),propensityFunctionCalculators(NULL),numberDependentSpecies(NULL),dependentSpecies(NULL),dependentSpeciesChange(NULL),numberDependentReactions(NULL),dependentReactions(NULL)
{
    // Allocate species counts.
    initialSpeciesCounts = new int[numberSpecies];
    memset(initialSpeciesCounts, 0, numberSpecies*sizeof(*initialSpeciesCounts));

    if (numberReactions > 0)
    {
        // Allocate reaction/species matrices.
        reactionTypes = new uint [numberReactions];
        memset(reactionTypes, 0, numberReactions*sizeof(*reactionTypes));

        // Allocate propensity function tables.
        propensityFunctionArgs = new lm::me::PropensityFunction*[numberReactions];
        memset(propensityFunctionArgs, 0, numberReactions*sizeof(*propensityFunctionArgs));
        propensityFunctionCalculators = new lm::me::PropensityFunctionCalculator[numberReactions];
        memset(propensityFunctionCalculators, 0, numberReactions*sizeof(*propensityFunctionCalculators));

        // Allocate the species dependency tables.
        numberDependentSpecies = new uint[numberReactions];
        memset(numberDependentSpecies, 0, numberReactions*sizeof(*numberDependentSpecies));
        dependentSpecies = new uint*[numberReactions];
        memset(dependentSpecies, 0, numberReactions*sizeof(*dependentSpecies));
        dependentSpeciesChange = new int*[numberReactions];
        memset(dependentSpeciesChange, 0, numberReactions*sizeof(*dependentSpeciesChange));

        // Allocate the reaction dependency tables.
        numberDependentReactions = new uint[numberReactions];
        memset(numberDependentReactions, 0, numberReactions*sizeof(*numberDependentReactions));
        dependentReactions = new uint*[numberReactions];
        memset(dependentReactions, 0, numberReactions*sizeof(*dependentReactions));
    }
}

CMESolver::ReactionModel::~ReactionModel()
{
    if (initialSpeciesCounts != NULL) delete[] initialSpeciesCounts; initialSpeciesCounts = NULL;
    if (reactionTypes != NULL) delete[] reactionTypes; reactionTypes = NULL;

    // Free the propensity function arguments and array.
    if (propensityFunctionArgs != NULL)
    {
        for (uint i=0; i<numberReactions; i++)
        {
            if (propensityFunctionArgs[i] != NULL) delete propensityFunctionArgs[i]; propensityFunctionArgs[i]=NULL;
        }
        delete[] propensityFunctionArgs; propensityFunctionArgs = NULL;
    }

    // Free the propensity function calculator array.
    if (propensityFunctionCalculators != NULL) delete[] propensityFunctionCalculators; propensityFunctionCalculators = NULL;

    // Free the species dependency tables.
    if (numberDependentSpecies != NULL) delete[] numberDependentSpecies; numberDependentSpecies = NULL;
    if (dependentSpecies != NULL)
    {
        for (uint i=0; i<numberReactions; i++)
        {
            if (dependentSpecies[i] != NULL)
                delete[] dependentSpecies[i];
        }
        delete[] dependentSpecies;
        dependentSpecies = NULL;
    }
    if (dependentSpeciesChange != NULL)
    {
        for (uint i=0; i<numberReactions; i++)
        {
            if (dependentSpeciesChange[i] != NULL)
                delete[] dependentSpeciesChange[i];
        }
        delete[] dependentSpeciesChange;
        dependentSpeciesChange = NULL;
    }

    // Free the reaction dependency tables.
    if (numberDependentReactions != NULL) delete[] numberDependentReactions; numberDependentReactions = NULL;
    if (dependentReactions != NULL)
    {
        for (uint i=0; i<numberReactions; i++)
        {
            if (dependentReactions[i] != NULL)
                delete[] dependentReactions[i];
        }
        delete[] dependentReactions;
        dependentReactions = NULL;
    }

    // Reset the species and reaction counts.
    numberSpecies = 0;
    numberSpeciesToTrack = 0;
    numberReactions = 0;
}

void CMESolver::ReactionModel::build(const uint numberSpeciesA, const uint numberReactionsA, const uint * initialSpeciesCountsA, const uint * reactionTypesA, const double * K, const int * SA, const uint * DA, const uint kCols)
{
    if (numberReactionsA > 0 && kCols == 0) throw InvalidArgException("K", "must have at least 1 column");

    // Set the initial species counts.
    for (uint i=0; i<numberSpecies; i++)
    {
        initialSpeciesCounts[i] = initialSpeciesCountsA[i];
    }

    // Set the reaction types.
    for (uint i=0; i<numberReactions; i++)
    {
        reactionTypes[i] = reactionTypesA[i];
    }

    // Set the stoichiometric and dependency matrices.
    for (uint row=0,index=0; row<numberSpecies; row++)
        for (uint col=0; col<numberReactions; col++, index++)
        {
            S[utuple(row,col)] = SA[index];
            D[utuple(row,col)] = DA[index];
        }

    // Create the propensity functions table.
    lm::me::PropensityFunctionFactory fs;
    for (uint i=0; i<numberReactions; i++)
    {
        // Create the rate constant tuple.
        list<double>ks;
        for (uint kcol=0; kcol<kCols; i++)
            ks.push_back(K[i*kCols+kcol]);

        // Get the propensity function and arguments.
        propensityFunctionCalculators[i] = fs.getPropensityFunctionCalculator(reactionTypes[i]);
        propensityFunctionArgs[i] = fs.createPropensityFunction(reactionTypes[i], i, S, D, tuple<double>(ks));
    }

    // Create the species dependency tables from the S matrix.
    for (uint i=0; i<numberReactions; i++)
    {
        numberDependentSpecies[i]=0;
        for (uint j=0, index=i; j<numberSpecies; j++, index+=numberReactions)
            if (S[index] != 0)
                numberDependentSpecies[i]++;
        dependentSpecies[i] = new uint[numberDependentSpecies[i]];
        dependentSpeciesChange[i] = new int[numberDependentSpecies[i]];
        for (uint j=0, index=i, k=0; j<numberSpecies; j++, index+=numberReactions)
        {
            if (S[index] != 0 && k < numberDependentSpecies[i])
            {
                dependentSpecies[i][k] = j;
                dependentSpeciesChange[i][k] = S[index];
                k++;
            }
        }
    }

    // Create the reaction dependency tables from the other tables.
    for (uint r=0; r<numberReactions; r++)
    {
        list<uint> dependentReactionList;

        // Go through all of the species changed by this reaction.
        for (uint d=0; d<numberDependentSpecies[r]; d++)
        {
            uint s = dependentSpecies[r][d];

            // Find all of the reactions that depend on this species.
            for (uint i=0, index=s*numberReactions; i<numberReactions; i++, index++)
            {
                if (D[index] > 0) dependentReactionList.push_back(i);
            }
        }

        // Eliminate any duplicates from the list.
        dependentReactionList.sort();
        dependentReactionList.unique();

        // Create the table.
        numberDependentReactions[r] = dependentReactionList.size();
        dependentReactions[r] = new uint[numberDependentReactions[r]];
        uint i=0;
        for (list<uint>::iterator it=dependentReactionList.begin(); it != dependentReactionList.end() && i<numberDependentReactions[r]; it++, i++)
        {
            dependentReactions[r][i] = *it;
        }
    }
}

void CMESolver::ReactionModel::setPropensityFunction(uint reaction, lm::me::PropensityFunction* propensityFunctionArg, lm::me::PropensityFunctionCalculator propensityFunctionCalculator)
{
    if (reaction >= numberReactions) throw InvalidArgException("reaction", "reaction index exceeded the number of reactions",reaction);
    if (propensityFunctionArgs[reaction] != NULL) delete propensityFunctionArgs[reaction];
    propensityFunctionArgs[reaction] = propensityFunctionArg;
    propensityFunctionCalculators[reaction] = propensityFunctionCalculator;
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

    if (reactionModel != NULL) delete reactionModel;
    reactionModel = new ReactionModel(rm.number_species(), rm.number_reactions());

    // Figure out the max number of columns we need in the k matrix.
    uint kCols = 0;
    for (uint i=0; i<rm.number_reactions(); i++)
        kCols = max(kCols,(uint)rm.reaction(i).rate_constant_size());

    // Set the K and reaction type tables.
    uint * reactionType = new uint[rm.number_reactions()];
    double * K = new double[rm.number_reactions()*kCols];
    for (uint i=0; i<rm.number_reactions(); i++)
    {
        reactionType[i] = rm.reaction(i).type();
        for (uint j=0; j<(uint)rm.reaction(i).rate_constant_size(); j++)
        {
            K[i*kCols+j] = rm.reaction(i).rate_constant(j);
        }
    }

    // Build the model.
    reactionModel->build(rm.number_species(), rm.number_reactions(), rm.initial_species_count().data(), reactionType, K, rm.stoichiometric_matrix().data(), rm.dependency_matrix().data(), kCols);

    // Free any resources.
    if (reactionType !=  NULL) delete [] reactionType; reactionType = NULL;
    if (K !=  NULL) delete [] K; K = NULL;
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

    // Reset the time.
    time = 0.0;

    // Reset the max time;
    maxTime = std::numeric_limits<double>::infinity();

    // Reset the species limits.
    numberSpeciesLimits = 0;
    if (speciesLimits != NULL)
    {
        delete[] speciesLimits;
    }
    speciesLimits = NULL;

    // Reset the fpt tracking list.
    numberFptTrackedSpecies = 0;
    if (fptTrackedSpecies != NULL) delete[] fptTrackedSpecies; fptTrackedSpecies = NULL;

    // Reset the tracked parameters list.
    trackedParameters.clear();

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
    // Set the max time.
    if (limits.has_max_time())
        maxTime = limits.max_time();

    // Set any upper or lower bounds.
    for (int i=0; i<limits.min_species_count_size(); i++)
        if (limits.min_species_count(i) != -1)
            setSpeciesLowerLimit(i, limits.min_species_count(i));
    for (int i=0; i<limits.max_species_count_size(); i++)
        if (limits.max_species_count(i) != -1)
            setSpeciesUpperLimit(i, limits.max_species_count(i));

    // Set any increasing/decreasing order parameter bound crossing detections.
    for (int i=0; i<limits.decreasing_order_parameter_limit_size(); i++)
    {
        for (int j=0; j<limits.decreasing_order_parameter_limit(i).value_size(); j++)
        {
            setSpeciesDecreasingLimit(limits.decreasing_order_parameter_limit(i).arrangement(), limits.decreasing_order_parameter_limit(i).order_parameter_id(), limits.decreasing_order_parameter_limit(i).value(j));
        }
    }
    for (int i=0; i<limits.increasing_order_parameter_limit_size(); i++)
    {
        for (int j=0; j<limits.increasing_order_parameter_limit(i).value_size(); j++)
        {
            setSpeciesIncreasingLimit(limits.increasing_order_parameter_limit(i).arrangement(), limits.increasing_order_parameter_limit(i).order_parameter_id(), limits.increasing_order_parameter_limit(i).value(j));
        }
    }
}

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

lm::io::TrajectoryLimits::LimitType CMESolver::getFinalLimitType()
{
    if (finalLimitType!=0)
    {
        return finalLimitType;
    }
    else
    {
        return lm::io::TrajectoryLimits::MAXTIME;
    }
}

void CMESolver::addToParameterTrackingList(pair<string,double*> parameter)
{
    trackedParameters.push_back(TrackedParameter(parameter.first, parameter.second));
}

/*double CMESolver::recordParameters(double nextRecordTime, double recordInterval, double simulationTime)
{
	if (recordInterval > 0.0)
	{
		// Write parameter values until the next write time is past the current time.
		do
		{
			// Update the parameter values.
			for (list<TrackedParameter>::iterator it = trackedParameters.begin(); it != trackedParameters.end(); it++)
			{
				// Record the species counts.
				it->dataSet.add_time(nextRecordTime);
				it->dataSet.add_value(*(it->valuePointer));
			}
			nextRecordTime += recordInterval;
		}
		while (nextRecordTime <= (simulationTime+1e-9));

		// Output the data, if necessary.
		queueRecordedParameters(false);
	}

    return nextRecordTime;
}
*/

/*
void CMESolver::queueRecordedParameters(bool flush)
{
    for (list<TrackedParameter>::iterator it = trackedParameters.begin(); it != trackedParameters.end(); it++)
    {
        if (it->dataSet.value_size() >= TUNE_PARAMETER_VALUES_BUFFER_SIZE || (flush && it->dataSet.value_size() > 0))
        {
            // Push it to the output queue.
            PROF_BEGIN(PROF_SERIALIZE_PV);
            lm::main::DataOutputQueue::getInstance()->pushDataSet(lm::main::DataOutputQueue::PARAMETER_VALUES, replicate, &it->dataSet);
            PROF_END(PROF_SERIALIZE_PV);

            // Reset the data set.
            it->dataSet.Clear();
            it->dataSet.set_parameter(it->name);
        }
    }
}
*/

}
}
