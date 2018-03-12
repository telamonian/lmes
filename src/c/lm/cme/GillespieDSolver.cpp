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
 * Author(s): Elijah Roberts, Max Klein
 */

#include <cmath>
#include <cstdio>
#include <limits>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "lm/ClassFactory.h"
#include "lm/cme/CMESolver.h"
#include "lm/cme/GillespieDSolver.h"
#include "lm/io/DegreeAdvancementTimeSeries.pb.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/Math.h"
#include "lm/main/Globals.h"
#include "lm/message/Message.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/Print.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/rng/XORShift.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lm/Tune.h"
#include "lm/Types.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"
#include "robertslab/pbuf/NDArraySerializer.h"

using std::list;
using std::map;
using std::string;
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

GillespieDSolver::GillespieDSolver()
:CMESolver((RandomGenerator::Distributions)(RandomGenerator::EXPONENTIAL|RandomGenerator::UNIFORM)),
 rngValues(NULL),expRngValues(NULL),nextRngValue(0),propensities(NULL)
{
}

GillespieDSolver::~GillespieDSolver()
{
    // Free any state.
    if (propensities != NULL) delete[] propensities; propensities = NULL;
    deallocateRngBuffers();
}

void GillespieDSolver::allocateRngBuffers()
{
    if (expRngValues == NULL || rngValues == NULL)
    {
        rngValues = new double[TUNE_LOCAL_RNG_CACHE_SIZE];
        expRngValues = new double[TUNE_LOCAL_RNG_CACHE_SIZE];
        nextRngValue = TUNE_LOCAL_RNG_CACHE_SIZE;
    }
}

void GillespieDSolver::deallocateRngBuffers()
{
    if (expRngValues != NULL) delete[] expRngValues; expRngValues = NULL;
    if (rngValues != NULL) delete[] rngValues; rngValues = NULL;
    nextRngValue = 0;
}

void GillespieDSolver::reset()
{
    CMESolver::reset();

    // Make sure we have allocated the RNG buffers.
    allocateRngBuffers();

    // Free any previous state.
    if (propensities != NULL) delete[] propensities; propensities = NULL;

    // Allocate reaction propensities table.
    propensities = new double[reactionModel->numberReactions];

    // Set the propensities to their initial values.
    for (uint i=0; i<reactionModel->numberReactions; i++)
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
    updateAllPropensities();
}

uint64_t GillespieDSolver::generateTrajectory(uint64_t maxSteps)
{
    if (reactionModel == NULL) throw Exception("GillespieDSolver did not have a reaction model.");
    if (propensities == NULL) throw Exception("GillespieDSolver state was not initialized.");

    // Make sure we have propensity functions for every reaction.
    for (uint i=0; i<reactionModel->numberReactions; i++)
        if (reactionModel->propensityFunctions[i] == NULL)
            throw Exception("A reaction did not have a valid propensity function",i);

    // Create local copies of the data for efficiency.
    const uint numberReactions = reactionModel->numberReactions;

    // Initialize the total propensity.
    double totalPropensity = 0.0;
    for (uint i=0; i<numberReactions; i++) totalPropensity += propensities[i];

    // Get the interval for writing degree advancements.
    double nextDegreeAdvancementWriteTime;
    vector<uint64_t> degreeAdvancementCounts;
    vector<double> degreeAdvancementTimes;
    if (writeDegreeAdvancementTimeSeries)
    {
        setInitialWriteInterval(degreeAdvancementWriteInterval, &nextDegreeAdvancementWriteTime, degreeAdvancements, numberDegreeAdvancements, &degreeAdvancementCounts, &degreeAdvancementTimes);
    }

    // Get the interval for writing order parameters.
    double nextOrderParameterWriteTime;
    vector<double> orderParameterTimeSeriesCounts, orderParameterTimeSeriesTimes;
    if (writeOrderParameterTimeSeries)
    {
        setInitialWriteInterval(orderParameterWriteInterval, &nextOrderParameterWriteTime, orderParameterValues, numberOrderParameters, &orderParameterTimeSeriesCounts, &orderParameterTimeSeriesTimes);
    }

    // Get the interval for writing species counts.
    double nextSpeciesWriteTime;
    vector<int32_t> speciesTimeSeriesCounts;
    vector<double> speciesTimeSeriesTimes;
    if (writeSpeciesTimeSeries)
    {
        setInitialWriteInterval(speciesWriteInterval, &nextSpeciesWriteTime, speciesCounts, reactionModel->numberSpecies, &speciesTimeSeriesCounts, &speciesTimeSeriesTimes);
    }

    // Run the direct method.
    Print::printf(Print::DEBUG, "Running Gillespie direct simulation for %d steps with %d species, %d reactions, %d species limits\n", maxSteps, reactionModel->numberSpecies, reactionModel->numberReactions, numberLimits);
    PROF_BEGIN(PROF_SIM_EXECUTE);
    uint64_t steps=0;
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
        if (nextRngValue >= TUNE_LOCAL_RNG_CACHE_SIZE)
        {
            rng->getRandomDoubles(rngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
            rng->getExpRandomDoubles(expRngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
            nextRngValue=0;
        }

        // Get the random values for this iteration though the loop.
        double randomValue = rngValues[nextRngValue];
        double expRandomValue = expRngValues[nextRngValue];

        // Go to the next rng pair.
        nextRngValue++;

        // Calculate the time to the next reaction.
        timeStep = expRandomValue/totalPropensity;
        time += timeStep;

        // If we are outside of the time limit, stop the trajectory.
        if (time >= timeLimit)
        {
            status = lm::message::WorkUnitStatus::LIMIT_REACHED;
            limitIDReached = lm::limit::TrajectoryLimits::TIME_LIMIT_ID;
            limitTypeReached = lm::input::TrajectoryLimit::TIME;
            break;
        }

        // If we are writing degree advancement time steps, write out any degree advancement time steps before this event occurred.
        if (writeDegreeAdvancementTimeSeries)
        {
            // Write degree advancement time steps until the next write time is past the current time.
            while (nextDegreeAdvancementWriteTime <= (time+EPS))
            {
                // Record the degree advancements.
                for (uint i=0; i<reactionModel->numberReactions; i++) degreeAdvancementCounts.push_back(degreeAdvancements[i]);
                degreeAdvancementTimes.push_back(nextDegreeAdvancementWriteTime);
                nextDegreeAdvancementWriteTime += degreeAdvancementWriteInterval;
            }
        }

        // If we are writing order parameter time steps, write out any order parameter time steps before this event occurred.
        if (writeOrderParameterTimeSeries)
        {
            // Write order parameter time steps until the next write time is past the current time.
            while (nextOrderParameterWriteTime <= (time+EPS))
            {
                // Record the order parameter counts.
                for (int i=0; i<numberOrderParameters; i++) orderParameterTimeSeriesCounts.push_back(orderParameterValues[i]);
                orderParameterTimeSeriesTimes.push_back(nextOrderParameterWriteTime);
                nextOrderParameterWriteTime += orderParameterWriteInterval;
            }
        }

        // If we are writing species time steps, write out any species time steps before this event occurred.
        if (writeSpeciesTimeSeries)
        {
            // Write time steps until the next write time is past the current time.
            while (nextSpeciesWriteTime <= (time+EPS))
            {
                // Record the species counts.
                for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
                speciesTimeSeriesTimes.push_back(nextSpeciesWriteTime);
                nextSpeciesWriteTime += speciesWriteInterval;
            }
        }

        // Calculate which reaction it was.
        double rngPropensity = randomValue*totalPropensity;
        uint r=0;
        for (; r<(numberReactions-1); r++)
        {
            if (rngPropensity < propensities[r])
                break;
            else
                rngPropensity -= propensities[r];
        }

        // Update species counts.
        performReactionEvent(r);

        // If we are outside of the limits, stop the trajectory.
        if (numberLimits > 0 && isTrajectoryOutsideLimits()) break;

        // Update the propensites given the reaction that occurred.
        updatePropensities(r);

        // Recalculate the total propensity.
        totalPropensity = 0.0;
        for (uint i=0; i<numberReactions; i++) totalPropensity += propensities[i];

        // If the total propensity is zero, stop the simulation.
        if (totalPropensity <= 0)
        {
            // If we have a time limit, say that we reached it.
            if (timeLimit < std::numeric_limits<double>::infinity())
            {
                timeStep = timeLimit-time;
                time = timeLimit;
                status = lm::message::WorkUnitStatus::LIMIT_REACHED;
                limitIDReached = lm::limit::TrajectoryLimits::TIME_LIMIT_ID;
                limitTypeReached = lm::input::TrajectoryLimit::TIME;
            }

            // Otherwise, zero propensity is an error.
            else
            {
                status = lm::message::WorkUnitStatus::ERROR;
            }
            break;
        }

        Print::printf(Print::VERBOSE_DEBUG, "Step %d: time=%e, count=%d, prop=%e, totprop=%e",steps,time,speciesCounts[0],propensities[0],totalPropensity);
    }
    PROF_END(PROF_SIM_EXECUTE);

    // See if we finished all of the steps.
    if (status == lm::message::WorkUnitStatus::STEPS_FINISHED)
    {
        Print::printf(Print::DEBUG, "Generated trajectory with %llu steps through time %e.", steps, time);
    }

    // If we finished the total time, write out all (but the last) remaining time steps. The last time step will be written later, dependent on writeFinalTrajectoryState
    else if (status == lm::message::WorkUnitStatus::LIMIT_REACHED && limitTypeReached == lm::input::TrajectoryLimit::TIME)
    {
        time = timeLimit;
        Print::printf(Print::DEBUG, "Generated trajectory through time %e.", time);

        if (writeDegreeAdvancementTimeSeries)
        {
            // Write degree advancement time steps until the next write time is past the current time.
            while (nextDegreeAdvancementWriteTime < timeLimit)
            {
                // Record the degree advancements.
                for (uint i=0; i<reactionModel->numberReactions; i++) degreeAdvancementCounts.push_back(degreeAdvancements[i]);
                degreeAdvancementTimes.push_back(nextDegreeAdvancementWriteTime);
                nextDegreeAdvancementWriteTime += degreeAdvancementWriteInterval;
            }
        }

        if (writeOrderParameterTimeSeries)
        {
            // Write order parameter time steps until the next write time is past the current time.
            while (nextOrderParameterWriteTime < timeLimit)
            {
                // Record the order parameter counts.
                for (int i=0; i<numberOrderParameters; i++) orderParameterTimeSeriesCounts.push_back(orderParameterValues[i]);
                orderParameterTimeSeriesTimes.push_back(nextOrderParameterWriteTime);
                nextOrderParameterWriteTime += orderParameterWriteInterval;
            }
        }

        if (writeSpeciesTimeSeries)
        {
            while (nextSpeciesWriteTime < timeLimit)
            {
                // Record the species counts.
                for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
                speciesTimeSeriesTimes.push_back(nextSpeciesWriteTime);
                nextSpeciesWriteTime += speciesWriteInterval;
            }
        }
    }

    // If we hit any limit, write out the final trajectory state if requested
    if (status == lm::message::WorkUnitStatus::LIMIT_REACHED and writeFinalTrajectoryState)
    {
        // Record the degree advancement counts.
        if (writeDegreeAdvancementTimeSeries)
        {
            for (uint i=0; i<numberDegreeAdvancements; i++) degreeAdvancementCounts.push_back(degreeAdvancements[i]);
            degreeAdvancementTimes.push_back(time);
        }

        // Record the order parameter counts.
        if (writeOrderParameterTimeSeries)
        {
            for (uint i=0; i<numberOrderParameters; i++) orderParameterTimeSeriesCounts.push_back(orderParameterValues[i]);
            orderParameterTimeSeriesTimes.push_back(time);
        }

        // Record the species counts.
        if (writeSpeciesTimeSeries)
        {
            for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
            speciesTimeSeriesTimes.push_back(time);
        }
    }

    // If we have any degree advancement time series data, add them to the output message.
    daTimeSeriesWrap.set_arrays_in_output_msg(output, degreeAdvancementCounts, degreeAdvancementTimes, trajectoryId, numberDegreeAdvancements, true);

    // If we have any order parameter time series data, add them to the output message.
    opTimeSeriesWrap.set_arrays_in_output_msg(output, orderParameterTimeSeriesCounts, orderParameterTimeSeriesTimes, trajectoryId, numberOrderParameters, true);

    // If we have any order parameter time series data, add them to the output message.
    speciesTimeSeriesWrap.set_arrays_in_output_msg(output, speciesTimeSeriesCounts, speciesTimeSeriesTimes, trajectoryId, reactionModel->numberSpeciesToTrack, true);

    // If the simulation reached a limit and we are tracking first passage times, add them to the output message.
    if (status == lm::message::WorkUnitStatus::LIMIT_REACHED && numberFptSpecies > 0)
    {
        // Mark that the message does contain some data.
        output->set_has_output(true);

        for (int i=0; i<numberFptSpecies; i++)
        {
            fptValues[i].serializeInto(output->add_first_passage_times());
        }
    }

    // If the simulation reached a limit and we are tracking order parameter first passage times, add them to the output message.
    if (status == lm::message::WorkUnitStatus::LIMIT_REACHED && numberFptTrackedOrderParameters > 0)
    {
        // Mark that the message does contain some data.
        output->set_has_output(true);

        for (int i=0; i<numberFptTrackedOrderParameters; i++)
        {
            fptTrackedOrderParameters[i].serializeTo(output->add_order_parameter_first_passage_times(), trajectoryId);
        }
    }

    // If any limit tracking is set up to write out to disk, add them to the output message.
    for (lm::limit::TrackingMap::const_iterator it=trackedLimits.begin();it!=trackedLimits.end();it++)
    {
        // Mark that the message does contain some data.
        output->set_has_output(true);

        if (writeLimitTracking and limits[it->second.limit_id].addTrackingToOutput)
        {
            limitTrackingWrap.setWrappedMsg(output->add_limit_tracking());
            limitTrackingWrap.serializeFrom(trajectoryId, it->second);
        }
    }

    return steps;
}

void GillespieDSolver::updateAllPropensities()
{
    // Update the propensities.
    for (uint i=0; i<reactionModel->numberReactions; i++)
    {
        propensities[i] = reactionModel->propensityFunctions[i]->calculate(time, speciesCounts, reactionModel->numberSpecies);
    }
}

void GillespieDSolver::updatePropensities(uint sourceReaction)
{
    // Update the propensities of the dependent reactions.
    for (uint i=0; i<reactionModel->numberDependentReactions[sourceReaction]; i++)
    {
        uint r = reactionModel->dependentReactions[sourceReaction][i];
        propensities[r] = reactionModel->propensityFunctions[r]->calculate(time, speciesCounts, reactionModel->numberSpecies);
    }
}

}
}
