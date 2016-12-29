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

#include "lm/ClassFactory.h"
#include "lm/Tune.h"
#include "lm/Math.h"
#include "lm/Print.h"
#include "lm/cme/CMESolver.h"
#include "lm/cme/GillespieDSolver.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/main/Globals.h"
#include "lm/message/Message.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/rng/XORShift.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"
#include "robertslab/pbuf/NDArraySerializer.h"

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

GillespieDSolver::GillespieDSolver()
:CMESolver((RandomGenerator::Distributions)(RandomGenerator::EXPONENTIAL|RandomGenerator::UNIFORM)),
 rngValues(NULL),expRngValues(NULL),nextRngValue(0),propensities(NULL)
{
    allocateRngBuffers();
}

GillespieDSolver::~GillespieDSolver()
{
    // Free any state.
    if (propensities != NULL) delete[] propensities; propensities = NULL;
    deallocateRngBuffers();
}

void GillespieDSolver::allocateRngBuffers()
{
    rngValues = new double[TUNE_LOCAL_RNG_CACHE_SIZE];
    expRngValues = new double[TUNE_LOCAL_RNG_CACHE_SIZE];
    nextRngValue = TUNE_LOCAL_RNG_CACHE_SIZE;
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
    vector<int32_t> degreeAdvancementTimeSeriesCounts;
    vector<double> degreeAdvancementTimeSeriesTimes;
    if (writeDegreeAdvancementTimeSeries)
    {
        nextDegreeAdvancementWriteTime = ceil(time/degreeAdvancementWriteInterval)*degreeAdvancementWriteInterval;
    }

    // Get the interval for writing order parameters.
    double nextOrderParameterWriteTime;
    vector<double> orderParameterTimeSeriesCounts, orderParameterTimeSeriesTimes;
    if (writeOrderParameterTimeSeries)
    {
        nextOrderParameterWriteTime = ceil(time/orderParameterWriteInterval)*orderParameterWriteInterval;
    }

    // If we are writing time steps, create the data set.
    double nextSpeciesWriteTime;
    vector<int32_t> speciesTimeSeriesCounts;
    vector<double> speciesTimeSeriesTimes;
    if (writeSpeciesTimeSeries)
    {
        if (!previouslyStarted)
        {
            for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
            speciesTimeSeriesTimes.push_back(time);
            nextSpeciesWriteTime = time+speciesWriteInterval;
        }
        else
        {
            nextSpeciesWriteTime = ceil((time+EPS)/speciesWriteInterval)*speciesWriteInterval;
        }
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
            limitIDReached = lm::trajectory::TrajectoryLimits::TIME_LIMIT_ID;
            limitTypeReached = lm::io::TrajectoryLimits::TIME;
            break;
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
                limitIDReached = lm::trajectory::TrajectoryLimits::TIME_LIMIT_ID;
                limitTypeReached = lm::io::TrajectoryLimits::TIME;
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

    // If we finished the total time, write out the remaining time steps.
    else if (status == lm::message::WorkUnitStatus::LIMIT_REACHED && limitTypeReached == lm::io::TrajectoryLimits::TIME)
    {
        time = timeLimit;
        Print::printf(Print::DEBUG, "Generated trajectory through time %e.", time);
        if (writeOrderParameterTimeSeries && !ffluxFlag)
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

        if (writeSpeciesTimeSeries && !ffluxFlag)
        {
            while (nextSpeciesWriteTime <= (timeLimit+EPS))
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
        // Record the order parameter counts.
        if (writeOrderParameterTimeSeries && !ffluxFlag)
        {
            for (int i=0; i<numberOrderParameters; i++) orderParameterTimeSeriesCounts.push_back(orderParameterValues[i]);
            orderParameterTimeSeriesTimes.push_back(nextOrderParameterWriteTime);
        }

        // Record the species counts.
        if (writeSpeciesTimeSeries && !ffluxFlag)
        {
            for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
            speciesTimeSeriesTimes.push_back(time);
        }
    }

    // If we have any species time series data, add them to the output message.
    if (speciesTimeSeriesCounts.size() > 0 || speciesTimeSeriesTimes.size() > 0)
    {
        // Mark that the message does contain some data.
        output->set_has_output(true);

        // Make sure the arrays are of a consistent size.
        if (speciesTimeSeriesCounts.size() == speciesTimeSeriesTimes.size()*reactionModel->numberSpeciesToTrack)
        {
            lm::io::SpeciesTimeSeries* speciesTimeSeriesDataSet = output->mutable_species_time_series();
            speciesTimeSeriesDataSet->set_trajectory_id(trajectoryId);

            // Serialize the times.
            robertslab::pbuf::NDArraySerializer::serializeInto<double>(speciesTimeSeriesDataSet->mutable_times(), speciesTimeSeriesTimes.data(), utuple(speciesTimeSeriesTimes.size()));

            // Serialize the species counts.
            robertslab::pbuf::NDArraySerializer::serializeInto<int32_t>(speciesTimeSeriesDataSet->mutable_counts(), speciesTimeSeriesCounts.data(), utuple(speciesTimeSeriesTimes.size(),reactionModel->numberSpeciesToTrack));
        }
        else
        {
            Print::printf(Print::ERROR, "Species time series counts and time mismatch %d,%d,%d", speciesTimeSeriesCounts.size(), reactionModel->numberSpeciesToTrack, speciesTimeSeriesTimes.size());
        }
    }

    // If we have any order parameter time series data, add them to the output message.
    if (orderParameterTimeSeriesCounts.size() > 0 || orderParameterTimeSeriesTimes.size() > 0)
    {
        // Mark that the message does contain some data.
        output->set_has_output(true);

        // Make sure the arrays are of a consistent size.
        if (orderParameterTimeSeriesCounts.size() == orderParameterTimeSeriesTimes.size()*numberOrderParameters)
        {
            lm::io::OrderParameterTimeSeries* orderParameterTimeSeriesDataSet = output->mutable_order_parameter_time_series();
            orderParameterTimeSeriesDataSet->set_trajectory_id(trajectoryId);

            opCounts.setMsgPtr(orderParameterTimeSeriesDataSet->mutable_values());
            opCounts.shape() << orderParameterTimeSeriesTimes.size() << numberOrderParameters;
            opCounts.set_data(orderParameterTimeSeriesCounts, robertslab::pbuf::NDArray::float64, true);

            opTimes.setMsgPtr(orderParameterTimeSeriesDataSet->mutable_times());
            opTimes.shape() << orderParameterTimeSeriesTimes.size();
            opTimes.set_data(orderParameterTimeSeriesTimes, robertslab::pbuf::NDArray::float64, true);
        }
        else
        {
            Print::printf(Print::ERROR, "Order parameter time series counts and time mismatch %d,%d,%d", orderParameterTimeSeriesCounts.size(), numberOrderParameters, orderParameterTimeSeriesTimes.size());
        }
    }

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
