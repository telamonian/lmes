/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
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
 * Author(s): Elijah Roberts
 */

#include <cmath>
#include <cstdio>
#include <limits>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <zlib.h>

#include "lm/ClassFactory.h"
#include "lm/Tune.h"
#include "lm/Math.h"
#include "lm/Print.h"
#include "lm/cme/CMESolver.h"
#include "lm/cme/GillespieDSolver.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ProcessWorkUnitOutput.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/rng/XORShift.h"
#ifdef OPT_CUDA
#include "lm/rng/XORWow.h"
#endif
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"
#include "robertslab/pbuf/NDArray.pb.h"

using std::string;
using std::list;
using std::map;
using std::vector;
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
    for (int i=0; i<reactionModel->numberReactions; i++)
    {
        propensities[i] = 0.0;
    }
}

void GillespieDSolver::getState(lm::io::TrajectoryState* state, uint trajectoryNumber)
{
    CMESolver::getState(state, trajectoryNumber);
}

void GillespieDSolver::setState(const lm::io::TrajectoryState& state, uint trajectoryNumber)
{
    CMESolver::setState(state, trajectoryNumber);

    // Set the propensities to their initial values.
    updateAllPropensities(time, reactionModel->numberSpecies);
}

long long GillespieDSolver::generateTrajectory(long long maxSteps)
{
    if (reactionModel == NULL) throw Exception("GillespieDSolver did not have a reaction model.");
    if (propensities == NULL) throw Exception("GillespieDSolver state was not initialized.");

    // Make sure we have propensity functions for every reaction.
    for (uint i=0; i<reactionModel->numberReactions; i++)
        if (reactionModel->propensityFunctions[i] == NULL)
            throw Exception("A reaction did not have a valid propensity function",i);

    // Create local copies of the data for efficiency.
    const uint numberSpecies = reactionModel->numberSpecies;
    const uint numberReactions = reactionModel->numberReactions;

    // Initialize the total propensity.
    double totalPropensity = 0.0;
    for (uint i=0; i<numberReactions; i++) totalPropensity += propensities[i];

    // Create the output message.
    lm::message::Message msgpp;
    lm::message::ProcessWorkUnitOutput* msgp = msgpp.mutable_process_work_unit_output();
    msgp->set_work_unit_id(workUnitId);
    lm::message::WorkUnitOutput* msg = msgp->add_part_output();

    // Get the interval for writing species counts.
    double nextSpeciesWriteTime;
    vector<int32_t> speciesTimeSeriesCounts;
    vector<double> speciesTimeSeriesTimes;

    // If we are writing time steps, create the data set.
    if (writeSpeciesTimeSeries)
    {
        // If this is the start of the trajectory, add the initial counts.
        if (time == 0.0 || trajectoryStarted==false)
        {
            nextSpeciesWriteTime=speciesWriteInterval;
            for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
            speciesTimeSeriesTimes.push_back(0.0);
        }
        else
        {
            nextSpeciesWriteTime = ceil(time/speciesWriteInterval)*speciesWriteInterval;
        }
    }

    // Local cache of random numbers.
    double rngValues[TUNE_LOCAL_RNG_CACHE_SIZE];
    double expRngValues[TUNE_LOCAL_RNG_CACHE_SIZE];
    int rngNext=TUNE_LOCAL_RNG_CACHE_SIZE;

    // Run the direct method.
    Print::printf(Print::DEBUG, "Running Gillespie direct simulation for %d steps with %d species, %d reactions, %d species limits\n", maxSteps, reactionModel->numberSpecies, reactionModel->numberReactions, numberLimits);
    PROF_BEGIN(PROF_SIM_EXECUTE);
    long long steps=0;
    while (true)
    {
        // See if we have finished the steps.
        if (steps >= maxSteps)
        {
            status = lm::message::WorkUnitStatus::STEPS_FINISHED;
            break;
        }

        // Increment the steps.
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
        timeStep = expR/totalPropensity;
        time += timeStep;

        // If we are outside of the time limit, stop the trajectory.
        if (time >= timeLimit)
        {
            status = lm::message::WorkUnitStatus::LIMIT_REACHED;
            limitReached = lm::io::TrajectoryLimits::MAXTIME;
            break;
        }

        // If we are writing time steps, write out any time steps before this event occurred.
        if (writeSpeciesTimeSeries)
        {
            // Write time steps until the next write time is past the current time.
            while (nextSpeciesWriteTime <= (time+1e-9))
            {
                // Record the species counts.
                for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
                speciesTimeSeriesTimes.push_back(nextSpeciesWriteTime);
                nextSpeciesWriteTime += speciesWriteInterval;
            }
        }

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

        // Update species counts.
        performReactionEvent(r);

        // If we are outside of the limits, stop the trajectory.
        if (numberLimits > 0 && isTrajectoryOutsideLimits()) break;

        // Update the propensites given the reaction that occurred.
        updatePropensities(time, r, numberSpecies);

        // Recalculate the total propensity.
        totalPropensity = 0.0;
        for (uint i=0; i<numberReactions; i++) totalPropensity += propensities[i];

        // If the total propensity is zero, return an error.
        if (totalPropensity <= 0)
        {
            // If we have a time limit, say that we reached it.
            if (timeLimit < std::numeric_limits<double>::infinity())
            {
                timeStep = timeLimit-time;
                time = timeLimit;
                status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                limitReached = lm::io::TrajectoryLimits::MAXTIME;
            }

            // Otherwise, zero propensity is an error.
            else
            {
                status = lm::message::WorkUnitStatus::ERROR;
                break;
            }
        }

        //Print::printf(Print::VERBOSE_DEBUG, "Step %d: time=%e, count=%d, prop=%e, totprop=%e",steps,time,speciesCounts[0],propensities[0],totalPropensity);

         // Go to the next rng pair.
        rngNext++;
    }
    PROF_END(PROF_SIM_EXECUTE);

    // See if we finished all of the steps.
    if (status == lm::message::WorkUnitStatus::STEPS_FINISHED)
    {
        Print::printf(Print::DEBUG, "Generated trajectory with %llu steps through time %e.", steps, time);
    }

    // If we finished the total time, write out the remaining time steps.
    else if (status == lm::message::WorkUnitStatus::LIMIT_REACHED && limitReached == lm::io::TrajectoryLimits::MAXTIME)
    {
        time = timeLimit;
        Print::printf(Print::DEBUG, "Generated trajectory through time %e.", time);
        if (writeSpeciesTimeSeries)
        {
            while (nextSpeciesWriteTime <= (timeLimit+1e-9))
            {
                // Record the species counts.
                for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
                speciesTimeSeriesTimes.push_back(nextSpeciesWriteTime);
                nextSpeciesWriteTime += speciesWriteInterval;
            }
        }
    }

    // Otherwise we must have finished because of a species/order parameter limit, so just write out the last time.
    else if (status == lm::message::WorkUnitStatus::LIMIT_REACHED)
    {
        // Record the species counts.
        if (writeSpeciesTimeSeries)
        {
            // Record the species counts.
            for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
            speciesTimeSeriesTimes.push_back(time);
        }
    }

    // If we have any species time series data, add them to the output message.
    if (speciesTimeSeriesCounts.size() > 0 || speciesTimeSeriesTimes.size() > 0)
    {
        // Make sure the arrays are of a consistent size.
        if (speciesTimeSeriesCounts.size() == speciesTimeSeriesTimes.size()*reactionModel->numberSpeciesToTrack)
        {
            lm::io::SpeciesTimeSeries* speciesTimeSeriesDataSet = msg->mutable_species_time_series();
            speciesTimeSeriesDataSet->set_trajectory_id(trajectoryId);

            robertslab::pbuf::NDArray* counts = speciesTimeSeriesDataSet->mutable_counts();
            counts->set_data_type(robertslab::pbuf::NDArray::int32);
            counts->set_compressed_deflate(true);
            counts->add_shape(speciesTimeSeriesTimes.size());
            counts->add_shape(reactionModel->numberSpeciesToTrack);
            std::string* data = counts->mutable_data();
            size_t dataSizeEstimate=compressBound(speciesTimeSeriesCounts.size()*sizeof(int32_t));
            data->resize(dataSizeEstimate);
            ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*data)[0]), &dataSizeEstimate, (unsigned char*)speciesTimeSeriesCounts.data(), speciesTimeSeriesCounts.size()*sizeof(int32_t)));
            data->resize(dataSizeEstimate);

            robertslab::pbuf::NDArray* times = speciesTimeSeriesDataSet->mutable_times();
            times->set_data_type(robertslab::pbuf::NDArray::float64);
            times->set_compressed_deflate(true);
            times->add_shape(speciesTimeSeriesTimes.size());
            data = times->mutable_data();
            dataSizeEstimate=compressBound(speciesTimeSeriesTimes.size()*sizeof(double));
            data->resize(dataSizeEstimate);
            ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*data)[0]), &dataSizeEstimate, (unsigned char*)speciesTimeSeriesTimes.data(), speciesTimeSeriesTimes.size()*sizeof(double)));
            data->resize(dataSizeEstimate);
        }
        else
        {
            Print::printf(Print::ERROR, "Species time series counts and time mismatch %d,%d,%d", speciesTimeSeriesCounts.size(), reactionModel->numberSpeciesToTrack, speciesTimeSeriesTimes.size());
        }
    }

    // If the simulation reached a limit and we are tracking first passage times, add them to the output message.
    if (status == lm::message::WorkUnitStatus::LIMIT_REACHED && numberFptTrackedSpecies > 0)
    {
        for (int i=0; i<numberFptTrackedSpecies; i++)
        {
            fptTrackedSpecies[i].serializeTo(trajectoryId, msg->add_first_passage_times());
        }
    }

    // If the output message has any data, send it.
    if ((msg->has_species_time_series() || msg->first_passage_times_size() > 0) && !ffluxFlag)		// these messages aren't useful for fflux simulation
    {
//    	printf("gillespiedsolver outputProcess: %d outputThread: %d\n", outputProcess, outputThread);
        communicator->sendMessage(outputProcess, outputThread, &msgpp);
    }

    return steps;
}

void GillespieDSolver::updateAllPropensities(double time, const uint numberSpecies)
{
    // Update the propensities.
    for (uint i=0; i<reactionModel->numberReactions; i++)
    {
        propensities[i] = reactionModel->propensityFunctions[i]->calculate(time, speciesCounts, numberSpecies);
    }
}

void GillespieDSolver::updatePropensities(double time, uint sourceReaction, const uint numberSpecies)
{
    // Update the propensities of the dependent reactions.
    for (uint i=0; i<reactionModel->numberDependentReactions[sourceReaction]; i++)
    {
        uint r = reactionModel->dependentReactions[sourceReaction][i];
        propensities[r] = reactionModel->propensityFunctions[r]->calculate(time, speciesCounts, numberSpecies);
    }
}

}
}
