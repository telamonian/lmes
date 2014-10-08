/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2014 Roberts Group,
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
 * Author(s): Elijah Roberts
 */

#include <string>
#include <limits>
#include <list>
#include <map>
#include <cmath>
#if defined(MACOSX)
#elif defined(LINUX)
#include <time.h>
#endif
#include "lm/ClassFactory.h"
#include "lm/Tune.h"
#include "lm/Math.h"
#include "lm/Print.h"
#include "lm/cme/CMESolver.h"
#include "lm/cme/GillespieDSolver.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ProcessWorkUnitOutput.pb.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/rng/XORShift.h"
#ifdef OPT_CUDA
#include "lm/rng/XORWow.h"
#endif
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using std::string;
using std::list;
using std::map;
using lm::rng::RandomGenerator;

namespace lm {
namespace cme {

bool GillespieDSolver::registered=GillespieDSolver::registerClass();

bool GillespieDSolver::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::me::MESolver","lm::cme::GillespieDSolver",&GillespieDSolver::allocateObject);
    return true;
}

void* GillespieDSolver::allocateObject()
{
    return new GillespieDSolver();
}

GillespieDSolver::GillespieDSolver():CMESolver((RandomGenerator::Distributions)(RandomGenerator::EXPONENTIAL|RandomGenerator::UNIFORM)),propensities(NULL)
{
}

GillespieDSolver::~GillespieDSolver()
{
    // Free any state.
    if (propensities != NULL) delete[] propensities; propensities = NULL;
}

void GillespieDSolver::reset()
{
    CMESolver::reset();

    // Free any previous state.
    if (propensities != NULL) delete[] propensities; propensities = NULL;

    // Allocate reaction propensities table.
    propensities = new double[reactionModel->numberReactions];

    // Set the propensities to their initial values.
    updateAllPropensities(0.0);
}

void GillespieDSolver::getState(lm::io::TrajectoryState* state)
{
    CMESolver::getState(state);
}

void GillespieDSolver::setState(const lm::io::TrajectoryState& state)
{
    CMESolver::setState(state);

    // Set the propensities to their initial values.
    updateAllPropensities(time);
}

bool GillespieDSolver::generateTrajectory(long long maxSteps)
{
    if (reactionModel == NULL) throw Exception("GillespieDSolver did not have a reaction model.");
    if (propensities == NULL) throw Exception("GillespieDSolver state was not initialized.");

    // Make sure we have propensity functions for every reaction.
    for (uint i=0; i<reactionModel->numberReactions; i++)
        if (reactionModel->propensityFunctions[i] == NULL || reactionModel->propensityFunctionArgs[i] == NULL)
            throw Exception("A reaction did not have a valid propensity function",i);

    // Create local copies of the data for efficiency.
    uint numberSpecies = reactionModel->numberSpecies;
    uint numberReactions = reactionModel->numberReactions;

    // Initialize the total propensity.
    double totalPropensity = 0.0;
    for (uint i=0; i<numberReactions; i++) totalPropensity += propensities[i];

    // Create the output message.
    lm::message::Message msgp;
    lm::message::ProcessWorkUnitOutput* msg = msgp.add_process_work_unit_output();
    msg->set_work_unit_id(workUnitId);

    // Get the interval for writing species counts.
    double writeInterval=atof(simulationParameters["writeInterval"].c_str());
    bool writeTimeSteps = (writeInterval > 0.0);
    double nextSpeciesCountsWriteTime;
    lm::io::SpeciesCounts* speciesCountsDataSet = NULL;

    // If we are writing time steps, create the data set.
    if (writeTimeSteps)
    {
        // Initialize the data set.
        speciesCountsDataSet = msg->mutable_species_counts();
        speciesCountsDataSet->set_trajectory_id(trajectoryId);
        speciesCountsDataSet->set_number_species(reactionModel->numberSpeciesToTrack);
        speciesCountsDataSet->set_number_entries(0);

        // If this is the start of the trajectory, add the initial counts.
        if (time == 0.0 || trajectoryStarted==false)
        {
//        	printf("traj_id %d has_started %d\n", trajectoryId, trajectoryStarted);
            nextSpeciesCountsWriteTime=writeInterval;
            speciesCountsDataSet->set_number_entries(1);
            speciesCountsDataSet->add_time(0.0);
            for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesCountsDataSet->add_species_count(speciesCounts[i]);
        }
        else
        {
            nextSpeciesCountsWriteTime = ceil(time/writeInterval)*writeInterval;
        }
    }

    // Get the interval for writing parameters.
//    double nextParameterWriteTime = INFINITY;
//    double parameterWriteInterval = atof((*parameters)["parameterWriteInterval"].c_str());
//    if (trackedParameters.size() > 0 && parameterWriteInterval > 0.0)
//        nextParameterWriteTime = recordParameters(0.0, parameterWriteInterval, 0.0);

    // Local cache of random numbers.
    double rngValues[TUNE_LOCAL_RNG_CACHE_SIZE];
    double expRngValues[TUNE_LOCAL_RNG_CACHE_SIZE];
    rng->getRandomDoubles(rngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
    rng->getExpRandomDoubles(expRngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
    int rngNext=0;

    // Run the direct method.
    Print::printf(Print::DEBUG, "Running Gillespie direct simulation for %d steps with %d species, %d reactions, %d species limits\n", maxSteps, reactionModel->numberSpecies, reactionModel->numberReactions, numberSpeciesLimits);
    PROF_BEGIN(PROF_SIM_EXECUTE);
    long long steps=0;
    while (totalPropensity > 0 && steps < maxSteps && !reachedSpeciesLimit())
    {
        steps++;

        // See if we need to update our rng caches.
        if (rngNext >= TUNE_LOCAL_RNG_CACHE_SIZE)
        {
            rng->getRandomDoubles(rngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
            rng->getExpRandomDoubles(expRngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
            rngNext=0;
        }

        // Calculate the time to the next reaction.
        double expR = expRngValues[rngNext];
        time += expR/totalPropensity;

         // If the new time is past the end time, we are done.
        if (time >= maxTime)
        {
            break;
        }

        // If we are writing time steps, write out any time steps before this event occurred.
        if (writeTimeSteps)
        {
            // Write time steps until the next write time is past the current time.
            while (nextSpeciesCountsWriteTime <= (time+1e-9))
            {
                // Record the species counts.
                speciesCountsDataSet->set_number_entries(speciesCountsDataSet->number_entries()+1);
                speciesCountsDataSet->add_time(nextSpeciesCountsWriteTime);
                for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesCountsDataSet->add_species_count(speciesCounts[i]);
                nextSpeciesCountsWriteTime += writeInterval;
            }
        }

        // If we are recording parameter values, write out the values before this event occurred.
//        if (nextParameterWriteTime <= (time+1e-9))
//        {
//            nextParameterWriteTime = recordParameters(nextParameterWriteTime, parameterWriteInterval, time);
//            addedParameterValues = true;
//        }

        // Calculate which reaction it was.
        double rngValue = rngValues[rngNext]*totalPropensity;
        uint r=0;
        for (; r<(numberReactions-1); r++)
        {
            if (rngValue < propensities[r])
                break;
            else
                rngValue -= propensities[r];
        }

        // Update species counts and propensities given the reaction that occurred.
        performReactionEvent(r);
        updatePropensities(time, r);

        // Recalculate the total propensity.
        totalPropensity = 0.0;
        for (uint i=0; i<numberReactions; i++) totalPropensity += propensities[i];

        //Print::printf(Print::VERBOSE_DEBUG, "Step %d: time=%e, count=%d, prop=%e, totprop=%e",steps,time,speciesCounts[0],propensities[0],totalPropensity);

        // If we are recording every event, add it.
//        if (!writeTimeSteps)
//        {
//            speciesCountsDataSet.set_number_entries(speciesCountsDataSet.number_entries()+1);
//            speciesCountsDataSet.add_time(time);
//            for (uint i=0; i<numberSpeciesToTrack; i++) speciesCountsDataSet.add_species_count(speciesCounts[i]);
//        }


         // Go to the next rng pair.
        rngNext++;
    }
    PROF_END(PROF_SIM_EXECUTE);

    bool reachedLimit = false;

    // If we finished the total time or ran out of reactions, write out the remaining time steps.
    if (time >= maxTime || totalPropensity <= 0)
    {
        time = maxTime;
        Print::printf(Print::DEBUG, "Generated trajectory through time %e.", time);
        if (writeTimeSteps)
        {
            while (nextSpeciesCountsWriteTime <= (maxTime+1e-9))
            {
                // Record the species counts.
                speciesCountsDataSet->set_number_entries(speciesCountsDataSet->number_entries()+1);
                speciesCountsDataSet->add_time(nextSpeciesCountsWriteTime);
                for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesCountsDataSet->add_species_count(speciesCounts[i]);
                nextSpeciesCountsWriteTime += writeInterval;
            }

            // If we are recording parameter values, write out the remaining value intervals.
    //        if (nextParameterWriteTime <= (maxTime+1e-9))
    //        {
    ////            recordParameters(nextParameterWriteTime, parameterWriteInterval, maxTime);
    //        }
        }
        reachedLimit = true;
    }

    // See if we finished all of the steps.
    else if (steps >= maxSteps)
    {
        Print::printf(Print::DEBUG, "Generated trajectory with %llu steps.", steps);
    }

    // Otherwise we must have finished because of a species/order parameter limit, so just write out the last time.
    else
    {
        // Record the species counts.
        if (writeTimeSteps)
        {
            // Record the species counts.
            speciesCountsDataSet->set_number_entries(speciesCountsDataSet->number_entries()+1);
            speciesCountsDataSet->add_time(time);
            for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesCountsDataSet->add_species_count(speciesCounts[i]);
        }
        reachedLimit = true;
    }

    // If the simulation reached a limit and we are tracking first passage times, add them to the output message.
    if (reachedLimit && numberFptTrackedSpecies > 0)
    {
        for (int i=0; i<numberFptTrackedSpecies; i++)
        {
            fptTrackedSpecies[i].serializeTo(trajectoryId, msg->add_first_passage_times());
        }
    }

    // If the output message has any data, send it.
    if (msg->has_species_counts() || msg->first_passage_times_size() > 0)
    {
        communicator->sendMessage(outputProcess, outputThread, &msgp);
    }

    return reachedLimit;
}

void GillespieDSolver::updateAllPropensities(double time)
{
    // Update the propensities.
    for (uint i=0; i<reactionModel->numberReactions; i++)
    {
        double (*propensityFunction)(double, uint * speciesCounts, void * args) = (double (*)(double, uint*, void*))reactionModel->propensityFunctions[i];
        propensities[i] = (*propensityFunction)(time, speciesCounts, reactionModel->propensityFunctionArgs[i]);
    }
}

void GillespieDSolver::updatePropensities(double time, uint sourceReaction)
{
    // Update the propensities of the dependent reactions.
    for (uint i=0; i<reactionModel->numberDependentReactions[sourceReaction]; i++)
    {
        uint r = reactionModel->dependentReactions[sourceReaction][i];
        double (*propensityFunction)(double, uint * speciesCounts, void * args) = (double (*)(double, uint*, void*))reactionModel->propensityFunctions[r];
        propensities[r] = (*propensityFunction)(time, speciesCounts, reactionModel->propensityFunctionArgs[r]);
    }
}

}
}
