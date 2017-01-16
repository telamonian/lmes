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
 * Author(s): Elijah Roberts
 */

#ifdef OPT_AVX

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/Tune.h"
#include "lm/Math.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/avx/GillespieDSolverAVX.h"
#include "lm/cme/CMESolver.h"
#include "lm/cme/ReactionModel.h"
#include "lm/io/DegreeAdvancementTimeSeries.pb.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
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
namespace avx {

bool GillespieDSolverAVX::registered=GillespieDSolverAVX::registerClass();

bool GillespieDSolverAVX::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::me::MESolver","lm::avx::GillespieDSolverAVX",&GillespieDSolverAVX::allocateObject);
    return true;
}

void* GillespieDSolverAVX::allocateObject()
{
    // Allocate aligned memory for the object since it contains avxd variables.
    GillespieDSolverAVX* obj;
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&obj, DOUBLES_PER_AVX*sizeof(double), sizeof(GillespieDSolverAVX)));

    // Construct the object in place.
    new (obj) GillespieDSolverAVX();

    return obj;
}

GillespieDSolverAVX::GillespieDSolverAVX()
:GillespieDSolver(),limitValues(NULL),fptAllocatedValues(0),fptValues(NULL),fptMinValues(NULL),fptMaxValues(NULL),speciesCounts(NULL),propensities(NULL),orderParameterValues(NULL),orderParameterPreviousValues(NULL),degreeAdvancements(NULL)
{
    // Initialize any avx variables.
    timeLimit = _mm256_set1_pd(std::numeric_limits<double>::infinity());
    time = _mm256_set1_pd(0.0);
    timeStep = _mm256_set1_pd(0.0);

    // Initialize any array variables.
    for (int i=0; i<DOUBLES_PER_AVX; i++)
    {
        initialized[i] = false;
        output[i] = new lm::message::WorkUnitOutput();
        status[i] = lm::message::WorkUnitStatus::NONE;
        limitIDReached[i] = lm::trajectory::TrajectoryLimits::DEFAULT_LIMIT_ID;
        limitTypeReached[i] = lm::io::TrajectoryLimits::NONE;
        trajectoryId[i] = std::numeric_limits<uint64_t>::max();
        previouslyStarted[i] = false;
    }
}

GillespieDSolverAVX::~GillespieDSolverAVX()
{
    // Free any array memory.
    for (int i=0; i<DOUBLES_PER_AVX; i++)
    {
        if (output[i] != NULL) delete output[i]; output[i] = NULL;
    }

    // Free any memory.
    if (limitValues != NULL) free(limitValues); limitValues = NULL;
    fptAllocatedValues = 0;
    if (fptValues != NULL) delete[] fptValues; fptValues = NULL;
    if (fptMinValues != NULL) free(fptMinValues); fptMinValues = NULL;
    if (fptMaxValues != NULL) free(fptMaxValues); fptMaxValues = NULL;
    if (speciesCounts != NULL) free(speciesCounts); speciesCounts = NULL;
    if (propensities != NULL) free(propensities); propensities = NULL;
    if (orderParameterValues != NULL) free(orderParameterValues); orderParameterValues = NULL;
    if (orderParameterPreviousValues != NULL) free(orderParameterPreviousValues); orderParameterPreviousValues = NULL;
    if (degreeAdvancements != NULL) free(degreeAdvancements); degreeAdvancements = NULL;
}

void GillespieDSolverAVX::allocateRngBuffers()
{
    if (expRngValues == NULL || rngValues == NULL)
    {
        POSIX_EXCEPTION_CHECK(posix_memalign((void**)&rngValues, DOUBLES_PER_AVX*sizeof(double), TUNE_LOCAL_RNG_CACHE_SIZE*sizeof(double)));
        POSIX_EXCEPTION_CHECK(posix_memalign((void**)&expRngValues, DOUBLES_PER_AVX*sizeof(double), TUNE_LOCAL_RNG_CACHE_SIZE*sizeof(double)));
        nextRngValue = TUNE_LOCAL_RNG_CACHE_SIZE;
    }
}

void GillespieDSolverAVX::deallocateRngBuffers()
{
    // Delete the rng caches.
    if (expRngValues != NULL) free(expRngValues); expRngValues = NULL;
    if (rngValues != NULL) free(rngValues); rngValues = NULL;
    nextRngValue = 0;
}

uint GillespieDSolverAVX::getSimultaneousTrajectories()
{
    return DOUBLES_PER_AVX;
}

void GillespieDSolverAVX::setReactionModel(const lm::input::ReactionModel& rm)
{
    GillespieDSolver::setReactionModel(rm);

    // Free any previous state.
    if (speciesCounts != NULL) free(speciesCounts); speciesCounts = NULL;
    if (propensities != NULL) free(propensities); propensities = NULL;
    if (degreeAdvancements != NULL) free(degreeAdvancements); propensities = NULL;

    // Allocate species counts table.
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&speciesCounts, DOUBLES_PER_AVX*sizeof(double), reactionModel->numberSpecies*DOUBLES_PER_AVX*sizeof(double)));

    // Allocate reaction propensities table.
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&propensities, DOUBLES_PER_AVX*sizeof(double), reactionModel->numberReactions*DOUBLES_PER_AVX*sizeof(double)));

    // Allocate degree advancements table.
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&degreeAdvancements, DOUBLES_PER_AVX*sizeof(double), reactionModel->numberReactions*DOUBLES_PER_AVX*sizeof(uint64_t)));
}

void GillespieDSolverAVX::setOrderParameters(const lm::io::OrderParameters& ops)
{
    GillespieDSolver::setOrderParameters(ops);

    // Free any previous state.
    if (orderParameterValues != NULL) free(orderParameterValues); orderParameterValues = NULL;
    if (orderParameterPreviousValues != NULL) free(orderParameterPreviousValues); orderParameterPreviousValues = NULL;

    // Allocate space for the order parameters.
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&orderParameterValues, DOUBLES_PER_AVX*sizeof(double), numberOrderParameters*DOUBLES_PER_AVX*sizeof(double)));
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&orderParameterPreviousValues, DOUBLES_PER_AVX*sizeof(double), numberOrderParameters*DOUBLES_PER_AVX*sizeof(double)));
}

void GillespieDSolverAVX::setLimits(const lm::io::TrajectoryLimits& lm)
{
    GillespieDSolver::setLimits(lm);

    // Set the time limit.
    timeLimit = _mm256_set1_pd(CMESolver::timeLimit);

    // Free any previous limit values.
    if (limitValues != NULL) free(limitValues); limitValues = NULL;

    if (numberLimits > 0)
    {
        // Allocate space for the limit values.
        POSIX_EXCEPTION_CHECK(posix_memalign((void**)&limitValues, DOUBLES_PER_AVX*sizeof(double), numberLimits*DOUBLES_PER_AVX*sizeof(double)));

        // Copy the limit values into the avx buffer.
        for (int i=0; i<(int)numberLimits; i++)
        {
            for (int j=0; j<DOUBLES_PER_AVX; j++)
                if (limits[i].type == lm::io::TrajectoryLimits::SPECIES)
                    limitValues[i*DOUBLES_PER_AVX+j] = double(limits[i].ivalue);
                else if (limits[i].type == lm::io::TrajectoryLimits::DEGREE_ADVANCEMENT)
                    limitValues[i*DOUBLES_PER_AVX+j] = double(limits[i].uvalue);
                else
                    limitValues[i*DOUBLES_PER_AVX+j] = limits[i].dvalue;
        }
    }
}

void GillespieDSolverAVX::reset()
{
    GillespieDSolver::reset();

    // Reset the species counts.
    for (int i=0; i<(int)reactionModel->numberSpecies*DOUBLES_PER_AVX; i++)
    {
        speciesCounts[i] = 0.0;
    }

    // Reset the propensities amd degree advancements.
    for (int i=0; i<(int)reactionModel->numberReactions*DOUBLES_PER_AVX; i++)
    {
        propensities[i] = 0.0;
        degreeAdvancements[i] = 0;
    }

    // Reset any array variables.
    for (int i=0; i<DOUBLES_PER_AVX; i++)
    {
        initialized[i] = false;
        if (output[i] != NULL) delete output[i];
        output[i] = new lm::message::WorkUnitOutput();
        status[i] = lm::message::WorkUnitStatus::NONE;
        limitIDReached[i] = lm::trajectory::TrajectoryLimits::DEFAULT_LIMIT_ID;
        limitTypeReached[i] = lm::io::TrajectoryLimits::NONE;
        trajectoryId[i] = std::numeric_limits<uint64_t>::max();
        previouslyStarted[i] = false;
    }

    // Reset the time.
    time = _mm256_set1_pd(0.0);
    timeStep = _mm256_set1_pd(0.0);

    // Reset the order parameters.
    for (int i=0; i<numberOrderParameters*DOUBLES_PER_AVX; i++)
    {
        orderParameterValues[i] = 0.0;
        orderParameterPreviousValues[i] = 0.0;
    }

    // Reset the fpt values.
    fptAllocatedValues = 0;
    if (fptValues != NULL) delete[] fptValues; fptValues = NULL;
    if (fptMinValues != NULL) free(fptMinValues); fptMinValues = NULL;
    if (fptMaxValues != NULL) free(fptMaxValues); fptMaxValues = NULL;
}

void GillespieDSolverAVX::getState(lm::io::TrajectoryState* state, uint trajectoryNumber)
{
    if (trajectoryNumber >= getSimultaneousTrajectories()) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());

    // Setup the base class state with the specified trajextory.
    copyTrajectoryStateToBaseSolver(trajectoryNumber);

    // Run the base get state method last.
    CMESolver::getState(state, trajectoryNumber);
}

void GillespieDSolverAVX::setState(const lm::io::TrajectoryState& state, uint trajectoryNumber)
{
    // Run the base set state first.
    CMESolver::setState(state, trajectoryNumber);

    // Allocate space for the fpt values, if necessary.
    if (numberFptSpecies > 0)
    {
        // See if we have not yet allocated space.
        if (fptAllocatedValues == 0)
        {
            fptValues = new lm::me::FPTDeque[numberFptSpecies*DOUBLES_PER_AVX];
            fptAllocatedValues = numberFptSpecies;
            POSIX_EXCEPTION_CHECK(posix_memalign((void**)&fptMinValues, DOUBLES_PER_AVX*sizeof(double), fptAllocatedValues*DOUBLES_PER_AVX*sizeof(double)));
            POSIX_EXCEPTION_CHECK(posix_memalign((void**)&fptMaxValues, DOUBLES_PER_AVX*sizeof(double), fptAllocatedValues*DOUBLES_PER_AVX*sizeof(double)));
        }
        //Otherwise, make sure the sizes match.
        else if (fptAllocatedValues != numberFptSpecies)
        {
            throw Exception("Mismatch between the number of fpt tracked values between trajectories",fptAllocatedValues,numberFptSpecies);
        }
    }

    // Copy the state from the base class.
    copyTrajectoryStateFromBaseSolver(trajectoryNumber);

    // Mark that this trajectory was initialized.
    initialized[trajectoryNumber] = true;
}

void GillespieDSolverAVX::copyTrajectoryStateToBaseSolver(uint trajectoryNumber)
{
    // Set the status.
    CMESolver::status = status[trajectoryNumber];

    // Set the limit reached.
    CMESolver::limitIDReached = limitIDReached[trajectoryNumber];
    CMESolver::limitTypeReached = limitTypeReached[trajectoryNumber];

    // Set the trajectory id.
    CMESolver::trajectoryId = trajectoryId[trajectoryNumber];

    // Set the trajectory started flag.
    CMESolver::previouslyStarted = previouslyStarted[trajectoryNumber];

    // Set the species counts.
    for (uint i=0; i<reactionModel->numberSpecies; i++)
    {
        CMESolver::speciesCounts[i] = lround(speciesCounts[i*DOUBLES_PER_AVX+trajectoryNumber]);
    }

    // Set the time.
    CMESolver::time = ((double*)&time)[trajectoryNumber];
    CMESolver::timeStep = ((double*)&timeStep)[trajectoryNumber];

    // Set the order parameters.
    for (int i=0; i<numberOrderParameters; i++)
    {
        CMESolver::orderParameterValues[i] = orderParameterValues[i*DOUBLES_PER_AVX+trajectoryNumber];
        CMESolver::orderParameterPreviousValues[i] = orderParameterPreviousValues[i*DOUBLES_PER_AVX+trajectoryNumber];
    }

    // Set the first passage times.
    for (int i=0; i<numberFptSpecies; i++)
    {
        CMESolver::fptValues[i] = fptValues[i*DOUBLES_PER_AVX+trajectoryNumber];
    }

    // Set the degree advancements.
    for (uint i=0; i<reactionModel->numberReactions; i++)
    {
        CMESolver::degreeAdvancements[i] = degreeAdvancements[i*DOUBLES_PER_AVX+trajectoryNumber];
    }


    // Set the histogram bin values.
    //TODO: implement
}

void GillespieDSolverAVX::copyTrajectoryStateFromBaseSolver(uint trajectoryNumber)
{
    // Set the status.
    status[trajectoryNumber] = CMESolver::status;

    // Set the limit reached.
    limitIDReached[trajectoryNumber] = CMESolver::limitIDReached;
    limitTypeReached[trajectoryNumber] = CMESolver::limitTypeReached;

    // Set the trajectory id.
    trajectoryId[trajectoryNumber] = CMESolver::trajectoryId;

    // Set the trajectory started flag.
    previouslyStarted[trajectoryNumber] = CMESolver::previouslyStarted;

    // Set the species counts.
    for (uint i=0; i<reactionModel->numberSpecies; i++)
    {
        speciesCounts[i*DOUBLES_PER_AVX+trajectoryNumber] = double(CMESolver::speciesCounts[i]);
    }

    // Set the time.
    ((double*)&time)[trajectoryNumber] = CMESolver::time;
    ((double*)&timeStep)[trajectoryNumber] = CMESolver::timeStep;

    // Set the order parameters.
    for (int i=0; i<numberOrderParameters; i++)
    {
        orderParameterValues[i*DOUBLES_PER_AVX+trajectoryNumber] = CMESolver::orderParameterValues[i];
        orderParameterPreviousValues[i*DOUBLES_PER_AVX+trajectoryNumber] = CMESolver::orderParameterPreviousValues[i];
    }

    // Set the first passage times.
    for (int i=0; i<numberFptSpecies; i++)
    {
        fptValues[i*DOUBLES_PER_AVX+trajectoryNumber] = CMESolver::fptValues[i];
        fptMinValues[i*DOUBLES_PER_AVX+trajectoryNumber] = double(CMESolver::fptValues[i].minValue);
        fptMaxValues[i*DOUBLES_PER_AVX+trajectoryNumber] = double(CMESolver::fptValues[i].maxValue);
    }

    // Set the species counts.
    for (uint i=0; i<reactionModel->numberReactions; i++)
    {
        degreeAdvancements[i*DOUBLES_PER_AVX+trajectoryNumber] = CMESolver::degreeAdvancements[i];
    }

    // Set the histogram bin values.
    //TODO: implement
}

void GillespieDSolverAVX::copyOutputToBaseSolver(uint trajectoryNumber)
{
    CMESolver::output->CopyFrom(*output[trajectoryNumber]);
}


void GillespieDSolverAVX::copyOutputFromBaseSolver(uint trajectoryNumber)
{
    output[trajectoryNumber]->CopyFrom(*CMESolver::output);
}

lm::message::WorkUnitOutput* GillespieDSolverAVX::getOutput(uint trajectoryNumber)
{
    if (trajectoryNumber >= getSimultaneousTrajectories()) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());

    // Get the output pointer.
    lm::message::WorkUnitOutput* ret = output[trajectoryNumber];

    // Forget about the pointer, since the caller is now responsible for it.
    output[trajectoryNumber] = NULL;

    return ret;

}

lm::message::WorkUnitStatus::Status GillespieDSolverAVX::getStatus(uint trajectoryNumber)
{
    if (trajectoryNumber >= getSimultaneousTrajectories()) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());
    return status[trajectoryNumber];
}

uint64_t GillespieDSolverAVX::generateTrajectory(uint64_t maxSteps)
{
    // See how many trajectories were initialized.
    uint numberInitialized=0;
    for (int i=0; i<DOUBLES_PER_AVX; i++)
        if (initialized[i])
            numberInitialized++;

    // If any trajectories were not initialized, run them all with the base Gillespie solver.
    if (numberInitialized != DOUBLES_PER_AVX)
    {
        uint64_t steps=0;
        for (int i=0; i<DOUBLES_PER_AVX; i++)
        {
            if (initialized[i])
            {
                Print::printf(Print::DEBUG, "GillespieDSolverAVX started without a full set of trajectories, running trajectory %llu with the GillespieDSolver.", trajectoryId[i]);
                copyOutputToBaseSolver(i);
                copyTrajectoryStateToBaseSolver(i);
                GillespieDSolver::updateAllPropensities();
                steps += GillespieDSolver::generateTrajectory(maxSteps);
                copyTrajectoryStateFromBaseSolver(i);
                copyOutputFromBaseSolver(i);
            }
        }

        // Skip any nedcessary rng values to get us back into the proper alignemnt frame.
        nextRngValue += nextRngValue%DOUBLES_PER_AVX;

        return steps;
    }

    if (reactionModel == NULL) throw Exception("GillespieDSolverAVX did not have a reaction model.");
    if (propensities == NULL) throw Exception("GillespieDSolverAVX state was not initialized.");

    // Make sure we have propensity functions for every reaction.
    for (uint i=0; i<reactionModel->numberReactions; i++)
        if (reactionModel->propensityFunctions[i] == NULL)
            throw Exception("A reaction did not have a valid propensity function",i);

    // Create local copies of the data for efficiency.
    const uint numberReactions = reactionModel->numberReactions;

    // Set the propensities to their initial values.
    updateAllPropensities();

    // Initialize the total propensity.
    avxd totalPropensity = _mm256_setzero_pd();
    for (uint i=0; i<numberReactions; i++)
    {
        avxd propensity = _mm256_load_pd(&propensities[i*DOUBLES_PER_AVX]);
        totalPropensity = _mm256_add_pd(totalPropensity,propensity);
    }

    // Create the degree advancement data sets.
    avxd nextDegreeAdvancementWriteTime;
    vector<uint64_t> degreeAdvancementTimeSeriesCounts[DOUBLES_PER_AVX];
    vector<double> degreeAdvancementTimeSeriesTimes[DOUBLES_PER_AVX];
    if (writeDegreeAdvancementTimeSeries)
    {
        for (int i=0; i<DOUBLES_PER_AVX; i++)
        {
            if (!previouslyStarted[i])
            {
                for (uint j=0; j<reactionModel->numberReactions; j++) degreeAdvancementTimeSeriesCounts[i].push_back(degreeAdvancements[j*DOUBLES_PER_AVX+i]);
                degreeAdvancementTimeSeriesTimes[i].push_back(((double*)&time)[i]);
                ((double*)&nextDegreeAdvancementWriteTime)[i] = ((double*)&time)[i]+degreeAdvancementWriteInterval;
            }
            else
            {
                ((double*)&nextDegreeAdvancementWriteTime)[i] = ceil((((double*)&time)[i]+EPS)/degreeAdvancementWriteInterval)*degreeAdvancementWriteInterval;
            }
        }
    }

    // Get the interval for writing species counts.
    avxd eps = _mm256_set1_pd(EPS);
    avxd nextSpeciesWriteTime;
    vector<int32_t> speciesTimeSeriesCounts[DOUBLES_PER_AVX];
    vector<double> speciesTimeSeriesTimes[DOUBLES_PER_AVX];

    // If we are writing time steps, create the data set.
    if (writeSpeciesTimeSeries)
    {
        for (int i=0; i<DOUBLES_PER_AVX; i++)
        {
            if (!previouslyStarted[i])
            {
                for (uint j=0; j<reactionModel->numberSpeciesToTrack; j++) speciesTimeSeriesCounts[i].push_back(lround(speciesCounts[j*DOUBLES_PER_AVX+i]));
                speciesTimeSeriesTimes[i].push_back(((double*)&time)[i]);
                ((double*)&nextSpeciesWriteTime)[i] = ((double*)&time)[i]+speciesWriteInterval;
            }
            else
            {
                ((double*)&nextSpeciesWriteTime)[i] = ceil((((double*)&time)[i]+EPS)/speciesWriteInterval)*speciesWriteInterval;
            }
        }
    }

    // Run the direct method.
    Print::printf(Print::DEBUG, "Running Gillespie direct AVX simulation for %d steps with %d species, %d reactions, %d species limits\n", maxSteps, reactionModel->numberSpecies, reactionModel->numberReactions, numberLimits);
    PROF_BEGIN(PROF_SIM_EXECUTE);
    uint64_t steps=0;
    int allFalse;
    int trueMask;
    avxd comp;
    avxd nextTimeStep;
    avxd nextTime;
    while (true)
    {
        // See if we have finished the steps.
        if (steps >= maxSteps)
        {
            for (int i=0; i<DOUBLES_PER_AVX; i++)
                status[i] = lm::message::WorkUnitStatus::STEPS_FINISHED;
            break;
        }

        // Increment the steps.
        steps++;

        // See if we need to update our rng caches.
        if (nextRngValue >= TUNE_LOCAL_RNG_CACHE_SIZE)
        {
            rng->getRandomDoubles(rngValues,TUNE_LOCAL_RNG_CACHE_SIZE, true);
            rng->getExpRandomDoubles(expRngValues,TUNE_LOCAL_RNG_CACHE_SIZE, true);
            nextRngValue=0;
        }

        // Get the random values for this iteration though the loop.
        avxd randomValue = _mm256_load_pd(&rngValues[nextRngValue]);
        avxd expRandomValue = _mm256_load_pd(&expRngValues[nextRngValue]);

        // Go to the next rng pair.
        nextRngValue+=DOUBLES_PER_AVX;

        // Calculate the time to the next reaction.
        nextTimeStep = _mm256_div_pd(expRandomValue, totalPropensity);
        nextTime = _mm256_add_pd(time,nextTimeStep);

         // If any new time is past the end time, we are done.
        comp = _mm256_cmp_pd(nextTime, timeLimit, _CMP_GE_OQ);
        allFalse = _mm256_testz_pd(comp,comp);
        if (!allFalse)
        {
            // Get a bitmask of all values that were true.
            trueMask = _mm256_movemask_pd(comp);

            // Go through the mask.
            for (int i=0; i<DOUBLES_PER_AVX; i++)
            {
                // If this element was true, set that the max time limit was reached.
                if (trueMask&(1<<i))
                {
                    status[i] = lm::message::WorkUnitStatus::LIMIT_REACHED;
                    limitIDReached[i] = lm::trajectory::TrajectoryLimits::TIME_LIMIT_ID;
                    limitTypeReached[i] = lm::io::TrajectoryLimits::TIME;
                }
                else
                {
                    // Otherwise set that we finished steps.
                    status[i] = lm::message::WorkUnitStatus::STEPS_FINISHED;
                }
            }
            break;
        }

        // Otherwise, set the time and timestep since no reactions were over the max time.
        timeStep = nextTimeStep;
        time = nextTime;

        // If we are writing degree advancements, write out any time steps before this event occurred.
        if (writeDegreeAdvancementTimeSeries)
        {
            // Loop until we have finished writing out all elements.
            while (true)
            {
                // See if any elements still need time steps written.
                comp = _mm256_cmp_pd(nextDegreeAdvancementWriteTime, _mm256_add_pd(time, eps), _CMP_LE_OQ);
                trueMask = _mm256_movemask_pd(comp);
                if (!trueMask) break;

                // Go through the mask.
                for (int i=0; i<DOUBLES_PER_AVX; i++)
                {
                    // If this element was true, write its counts.
                    if (trueMask&(1<<i))
                    {
                        // Record the degree advancement counts.
                        for (uint j=0; j<reactionModel->numberReactions; j++) degreeAdvancementTimeSeriesCounts[i].push_back(degreeAdvancements[j*DOUBLES_PER_AVX+i]);
                        degreeAdvancementTimeSeriesTimes[i].push_back(((double*)&nextDegreeAdvancementWriteTime)[i]);
                        ((double*)&nextDegreeAdvancementWriteTime)[i] += degreeAdvancementWriteInterval;
                    }
                }
            }
        }

        // If we are writing species counts, write out any time steps before this event occurred.
        if (writeSpeciesTimeSeries)
        {
            // Loop until we have finished writing out all elements.
            while (true)
            {
                // See if any elements still need time steps written.
                comp = _mm256_cmp_pd(nextSpeciesWriteTime, _mm256_add_pd(time, eps), _CMP_LE_OQ);
                trueMask = _mm256_movemask_pd(comp);
                if (!trueMask) break;

                // Go through the mask.
                for (int i=0; i<DOUBLES_PER_AVX; i++)
                {
                    // If this element was true, write its counts.
                    if (trueMask&(1<<i))
                    {
                        // Record the species counts.
                        for (uint j=0; j<reactionModel->numberSpeciesToTrack; j++) speciesTimeSeriesCounts[i].push_back(lround(speciesCounts[j*DOUBLES_PER_AVX+i]));
                        speciesTimeSeriesTimes[i].push_back(((double*)&nextSpeciesWriteTime)[i]);
                        ((double*)&nextSpeciesWriteTime)[i] += speciesWriteInterval;
                    }
                }
            }
        }

        // Calculate a random propensity to figure out the reaction.
        avxd rngPropensity = _mm256_mul_pd(randomValue, totalPropensity);

//        {
//        double* res = (double*)&totalPropensity;
//        printf("%8.2f %8.2f %8.2f %8.2f\n", res[0], res[1], res[2], res[3]);
//        printf("RNG1: %8.2f %8.2f %8.2f %8.2f (%d)\n", rngValues[rngNext], rngValues[rngNext+1], rngValues[rngNext+2], rngValues[rngNext+3], rngNext);
//        res = (double*)&rngValue;
//        printf("RNG2: %8.2f %8.2f %8.2f %8.2f\n", res[0], res[1], res[2], res[3]);
//        }

        // Figure out which reaction it was.
        uint reactionsSelected = 0;
        uint reactionsToPerform[DOUBLES_PER_AVX];
        memset(reactionsToPerform, 0xFF, DOUBLES_PER_AVX*sizeof(uint));
        for (uint r=0; r<numberReactions; r++)
        {
            // Load the propensities.
            avxd propensity = _mm256_load_pd(&propensities[r*DOUBLES_PER_AVX]);

            // Compare the random propensity with this entry.
            comp = _mm256_cmp_pd(rngPropensity, propensity, _CMP_LT_OQ);
            allFalse = _mm256_testz_pd(comp,comp);

//            printf("Reaction: %d\n",r);
//            res = (double*)&rngPropensity;
//            printf("%8.2f %8.2f %8.2f %8.2f\n", res[0], res[1], res[2], res[3]);
//            res = (double*)&propensity;
//            printf("%8.2f %8.2f %8.2f %8.2f\n", res[0], res[1], res[2], res[3]);
//            res = (double*)&comp;
//            printf("%8.2f %8.2f %8.2f %8.2f\n", res[0], res[1], res[2], res[3]);
//            printf("All false: %d\n",allFalse);

            // If any were true, figure out which.
            if (!allFalse)
            {
                // Get a bitmask of all values that were true.
                trueMask = _mm256_movemask_pd(comp);

                // Go through the mask.
                for (int i=0; i<DOUBLES_PER_AVX; i++)
                {
                    // If this element was true, save the reaction and set the random propensity to inf.
                    if (trueMask&(1<<i))
                    {
                        reactionsToPerform[i] = r;
                        ((double*)&rngPropensity)[i] = std::numeric_limits<double>::infinity();
                        reactionsSelected++;
                    }
                }

                // If we found all of the reactions, we are done.
                if (reactionsSelected == DOUBLES_PER_AVX)
                    break;
            }


            // Subtract this entry from the random propensity and loop again.
            rngPropensity = _mm256_sub_pd(rngPropensity, propensity);
        }

        // If any reactions were not set, they must be the last reaction.
        if (reactionsSelected != DOUBLES_PER_AVX)
        {
            for (int i=0; i<DOUBLES_PER_AVX; i++)
            {
                if (reactionsToPerform[i] >= 0xFFFFFFFF)
                    reactionsToPerform[i] = numberReactions-1;
            }
        }

        //printf("Reaction to perform: %d %d %d %d\n", reactionsToPerform[0], reactionsToPerform[1], reactionsToPerform[2], reactionsToPerform[3]);

        // Update the species counts.
        performReactionEventAVX(reactionsToPerform);

        // If we are outside of the limits, stop the trajectory.
        if (numberLimits > 0 && isTrajectoryOutsideLimitsAVX()) break;

        // Update the propensites given the reaction that occurred.
        updatePropensities(time, reactionsToPerform);

        // Recalculate the total propensity.
        totalPropensity = _mm256_setzero_pd();
        for (uint i=0; i<numberReactions; i++)
        {
            avxd propensity = _mm256_load_pd(&propensities[i*DOUBLES_PER_AVX]);
            totalPropensity = _mm256_add_pd(totalPropensity,propensity);
        }

        // If any total propensities is zero, stop the trajectory.
        avxd comp = _mm256_cmp_pd(totalPropensity, _mm256_setzero_pd(), _CMP_LE_OQ);
        allFalse = _mm256_testz_pd(comp,comp);
        if (!allFalse)
        {
            // Get a bitmask of all values that were true.
            trueMask = _mm256_movemask_pd(comp);

            // Go through the mask.
            for (int i=0; i<DOUBLES_PER_AVX; i++)
            {
                // If this element was true, save the reaction and set the random propensity to inf.
                if (trueMask&(1<<i))
                {
                    // If we have a time limit, say that we reached it.
                    if (((double*)&timeLimit)[i] < std::numeric_limits<double>::infinity())
                    {
                        ((double*)&timeStep)[i] = ((double*)&timeLimit)[i]-((double*)&time)[i];
                        ((double*)&time)[i] = ((double*)&timeLimit)[i];
                        status[i] = lm::message::WorkUnitStatus::LIMIT_REACHED;
                        limitIDReached[i] = lm::trajectory::TrajectoryLimits::TIME_LIMIT_ID;
                        limitTypeReached[i] = lm::io::TrajectoryLimits::TIME;
                    }

                    // Otherwise, zero propensity is an error.
                    else
                    {
                        status[i] = lm::message::WorkUnitStatus::ERROR;
                    }
                }
                else
                {
                    status[i] = lm::message::WorkUnitStatus::STEPS_FINISHED;
                }
            }
            break;
        }

        //Print::printf(Print::VERBOSE_DEBUG, "Step %d: time=%e, count=%d, prop=%e, totprop=%e",steps,time,speciesCounts[0],propensities[0],totalPropensity);

//        double* p = (double*)&time;
//        printf("Time:             %8.2f %8.2f %8.2f %8.2f\n", p[0], p[1], p[2], p[3]);
//        p = (double*)speciesCounts;
//        printf("Species counts:   %8.2f %8.2f %8.2f %8.2f\n", p[0], p[1], p[2], p[3]);
//        //printf("Species counts:   %8.2f %8.2f %8.2f %8.2f\n", p[4], p[5], p[6], p[7]);
//        p = (double*)&totalPropensity;
//        printf("Total Propensity: %8.2f %8.2f %8.2f %8.2f\n", p[0], p[1], p[2], p[3]);
//        printf("-----------------\n");
    }
    PROF_END(PROF_SIM_EXECUTE);

//    double* p = (double*)&time;
//    printf("Final Time:             %8.2f %8.2f %8.2f %8.2f\n", p[0], p[1], p[2], p[3]);
//    p = (double*)speciesCounts;
//    printf("Final Species counts:   %8.2f %8.2f %8.2f %8.2f\n", p[0], p[1], p[2], p[3]);
//    //printf("Final Species counts:   %8.2f %8.2f %8.2f %8.2f\n", p[4], p[5], p[6], p[7]);
//    p = (double*)&totalPropensity;
//    printf("Final Total Propensity: %8.2f %8.2f %8.2f %8.2f\n", p[0], p[1], p[2], p[3]);

    // Finalize each of the trajectories.
    for (int i=0; i<DOUBLES_PER_AVX; i++)
    {
        // See if we finished all of the steps.
        if (status[i] == lm::message::WorkUnitStatus::STEPS_FINISHED)
        {
            Print::printf(Print::DEBUG, "Generated trajectory %llu with %llu steps through time %e.", trajectoryId[i], steps, ((double*)&time)[i]);
        }

        // If we finished the total time, write out the remaining time steps.
        else if (status[i] == lm::message::WorkUnitStatus::LIMIT_REACHED && limitTypeReached[i] == lm::io::TrajectoryLimits::TIME)
        {
            ((double*)&time)[i] = ((double*)&timeLimit)[i];
            Print::printf(Print::DEBUG, "Generated trajectory %llu through time %e.", trajectoryId[i], ((double*)&time)[i]);

            if (writeDegreeAdvancementTimeSeries)
            {
                while (((double*)&nextDegreeAdvancementWriteTime)[i] <= (((double*)&timeLimit)[i]+EPS))
                {
                    // Record the degree advancements.
                    for (uint j=0; j<reactionModel->numberReactions; j++) degreeAdvancementTimeSeriesCounts[i].push_back(degreeAdvancements[j*DOUBLES_PER_AVX+i]);
                    degreeAdvancementTimeSeriesTimes[i].push_back(((double*)&nextDegreeAdvancementWriteTime)[i]);
                    ((double*)&nextDegreeAdvancementWriteTime)[i] += degreeAdvancementWriteInterval;
                }
            }

            if (writeSpeciesTimeSeries)
            {
                while (((double*)&nextSpeciesWriteTime)[i] <= (((double*)&timeLimit)[i]+EPS))
                {
                    // Record the species counts.
                    for (uint j=0; j<reactionModel->numberSpeciesToTrack; j++) speciesTimeSeriesCounts[i].push_back(lround(speciesCounts[j*DOUBLES_PER_AVX+i]));
                    speciesTimeSeriesTimes[i].push_back(((double*)&nextSpeciesWriteTime)[i]);
                    ((double*)&nextSpeciesWriteTime)[i] += speciesWriteInterval;
                }
            }
        }

        // Otherwise we must have finished because of a species/order parameter limit, so just write out the last time.
        else if (status[i] == lm::message::WorkUnitStatus::LIMIT_REACHED)
        {
            // Record the degree advancements.
            if (writeDegreeAdvancementTimeSeries)
            {
                for (uint j=0; j<reactionModel->numberReactions; j++) degreeAdvancementTimeSeriesCounts[i].push_back(degreeAdvancements[j*DOUBLES_PER_AVX+i]);
                degreeAdvancementTimeSeriesTimes[i].push_back(((double*)&time)[i]);
            }

            // Record the species counts.
            if (writeSpeciesTimeSeries)
            {
                for (uint j=0; j<reactionModel->numberSpeciesToTrack; j++) speciesTimeSeriesCounts[i].push_back(lround(speciesCounts[j*DOUBLES_PER_AVX+i]));
                speciesTimeSeriesTimes[i].push_back(((double*)&time)[i]);
            }
        }

        // If we have any degree advancement data, add them to the output message.
        if (degreeAdvancementTimeSeriesCounts[i].size() > 0 || degreeAdvancementTimeSeriesTimes[i].size() > 0)
        {
            // Mark that the message does contain some data.
            output[i]->set_has_output(true);

            // Make sure the arrays are of a consistent size.
            if (degreeAdvancementTimeSeriesCounts[i].size() == degreeAdvancementTimeSeriesTimes[i].size()*reactionModel->numberReactions)
            {
                lm::io::DegreeAdvancementTimeSeries* dataSet = output[i]->mutable_degree_advancement_time_series();
                dataSet->set_trajectory_id(trajectoryId[i]);
                robertslab::pbuf::NDArraySerializer::serializeInto<double>(dataSet->mutable_times(), degreeAdvancementTimeSeriesTimes[i].data(), utuple(degreeAdvancementTimeSeriesTimes[i].size()));
                robertslab::pbuf::NDArraySerializer::serializeInto<uint64_t>(dataSet->mutable_counts(), degreeAdvancementTimeSeriesCounts[i].data(), utuple(degreeAdvancementTimeSeriesTimes[i].size(),reactionModel->numberReactions));
            }
            else
            {
                Print::printf(Print::ERROR, "Degree advancement time series counts and time mismatch %d,%d,%d", degreeAdvancementTimeSeriesCounts[i].size(), reactionModel->numberReactions, degreeAdvancementTimeSeriesTimes[i].size());
            }
        }

        // If we have any species time series data, add them to the output message.
        if (speciesTimeSeriesCounts[i].size() > 0 || speciesTimeSeriesTimes[i].size() > 0)
        {
            // Mark that the message does contain some data.
            output[i]->set_has_output(true);

            // Make sure the arrays are of a consistent size.
            if (speciesTimeSeriesCounts[i].size() == speciesTimeSeriesTimes[i].size()*reactionModel->numberSpeciesToTrack)
            {
                lm::io::SpeciesTimeSeries* speciesTimeSeriesDataSet = output[i]->mutable_species_time_series();
                speciesTimeSeriesDataSet->set_trajectory_id(trajectoryId[i]);
                robertslab::pbuf::NDArraySerializer::serializeInto<double>(speciesTimeSeriesDataSet->mutable_times(), speciesTimeSeriesTimes[i].data(), utuple(speciesTimeSeriesTimes[i].size()));
                robertslab::pbuf::NDArraySerializer::serializeInto<int32_t>(speciesTimeSeriesDataSet->mutable_counts(), speciesTimeSeriesCounts[i].data(), utuple(speciesTimeSeriesTimes[i].size(),reactionModel->numberSpeciesToTrack));
            }
            else
            {
                Print::printf(Print::ERROR, "Species time series counts and time mismatch %d,%d,%d", speciesTimeSeriesCounts[i].size(), reactionModel->numberSpeciesToTrack, speciesTimeSeriesTimes[i].size());
            }
        }

        // If the simulation reached a limit and we are tracking first passage times, add them to the output message.
        if (status[i] == lm::message::WorkUnitStatus::LIMIT_REACHED && numberFptSpecies > 0)
        {
            // Mark that the message does contain some data.
            output[i]->set_has_output(true);

            for (int j=0; j<numberFptSpecies; j++)
            {
                fptValues[j*DOUBLES_PER_AVX+i].serializeInto(output[i]->add_first_passage_times());
                printf("avx gillespie writing fpt for %d %d,%d\n", i, (int)output[i]->first_passage_times(j).trajectory_id(), output[i]->first_passage_times(j).species()); fflush(stdout);
            }
        }
    }

    return steps*DOUBLES_PER_AVX;
}

void GillespieDSolverAVX::updateAllPropensities()
{
    // Update the propensities.
    for (uint i=0; i<reactionModel->numberReactions; i++)
    {
        avxd propensity = reactionModel->propensityFunctions[i]->calculateAvx(time, speciesCounts, reactionModel->numberSpecies);
        _mm256_store_pd(&propensities[i*DOUBLES_PER_AVX], propensity);
    }
}

void GillespieDSolverAVX::updatePropensities(avxd time, uint* sourceReaction)
{
    updateAllPropensities();
    // TODO: implement
//    // Update the propensities of the dependent reactions.
//    for (uint i=0; i<reactionModel->numberDependentReactions[sourceReaction]; i++)
//    {
//        uint r = reactionModel->dependentReactions[sourceReaction][i];
//        propensities[r] = reactionModel->propensityFunctions[i]->calculateAvx(time, speciesCounts, reactionModel->numberSpecies);
//    }
}

void GillespieDSolverAVX::performReactionEventAVX(uint* reactionsToPerform)
{
    // Update the counts according to the dependency tables.
    for (uint j=0; j<DOUBLES_PER_AVX; j++)
    {
        uint r = reactionsToPerform[j];
        for (int i=0; i<(int)reactionModel->numberDependentSpecies[r]; i++)
        {
            speciesCounts[(reactionModel->dependentSpecies[r][i])*DOUBLES_PER_AVX+j] += double(reactionModel->dependentSpeciesChange[r][i]);
        }
    }
    if (hasUpdateSpeciesCountsListeners) callUpdateSpeciesCountsListenersAVX(reactionsToPerform);
}

void GillespieDSolverAVX::callUpdateSpeciesCountsListenersAVX(uint* reactionsToPerform)
{
    // Update the degree advancement, if we are tracking it.
    if (trackingDegreeAdvancements)
    {
        for (uint i=0; i<DOUBLES_PER_AVX; i++)
        {
            uint r = reactionsToPerform[i];
            degreeAdvancements[r*DOUBLES_PER_AVX+i]++;
        }
    }

    // Update the first passage time tables.
    for (int i=0; i<numberFptSpecies; i++)
    {
        uint speciesIndex = fptValues[i*DOUBLES_PER_AVX+0].species;
        avxd counts = _mm256_load_pd(&speciesCounts[speciesIndex*DOUBLES_PER_AVX]);

        // Check if any trajectory went below the previous min or above the previous max.
        avxd comp = _mm256_cmp_pd(counts, _mm256_load_pd(&fptMinValues[i*DOUBLES_PER_AVX]), _CMP_LT_OQ);
        int minAllFalse = _mm256_testz_pd(comp,comp);
        comp = _mm256_cmp_pd(counts, _mm256_load_pd(&fptMaxValues[i*DOUBLES_PER_AVX]), _CMP_GT_OQ);
        int maxAllFalse = _mm256_testz_pd(comp,comp);
        if (!minAllFalse || !maxAllFalse)
        {
            // At least on trajectory was outside the range, so update all of them.
            for (int j=0; j<DOUBLES_PER_AVX; j++)
            {
                // Update the fpt.
                fptValues[i*DOUBLES_PER_AVX+j].insert(lround(((double*)&counts)[j]), ((double*)&time)[j]);

                // Reload the min and and max.
                fptMinValues[i*DOUBLES_PER_AVX+j] = double(fptValues[i*DOUBLES_PER_AVX+j].minValue);
                fptMaxValues[i*DOUBLES_PER_AVX+j] = double(fptValues[i*DOUBLES_PER_AVX+j].maxValue);
            }
        }
    }

    // Update any order parameters.
    if (numberOrderParameters > 0)
    {
        // Copy the old order parameters.
        memcpy(orderParameterPreviousValues, orderParameterValues, numberOrderParameters*sizeof(double)*DOUBLES_PER_AVX);

        // Update any order parameters.
        for (int i=0; i<numberOrderParameters; i++)
        {
            avxd value = orderParameterFunctions[i]->calculateAvx(time, speciesCounts, reactionModel->numberSpecies);
            _mm256_store_pd(&orderParameterValues[i*DOUBLES_PER_AVX], value);
        }
    }

    // Update any tilingHists.
}

bool GillespieDSolverAVX::isTrajectoryOutsideLimitsAVX()
{
    // Go through the limits.
    for (uint i=0; i<numberLimits; i++)
    {
        int outsideLimitMask=0;
        TrajectoryLimit& l = limits[i];
        avxd limitValue = _mm256_load_pd(&limitValues[i*DOUBLES_PER_AVX]);
        avxd comp1;
        avxd comp2;

        switch (l.type)
        {
        case EH::NONE: throw Exception("GillespieDSolverAVX tried to check a limit that did not have an associated LimitType"); break;
        case EH::TIME: throw Exception("GillespieDSolverAVX reached a time limit that was mixed in with the other limits"); break;

        case EH::SPECIES:
            switch (l.stoppingCondition)
            {
            case EH::MIN:
                if (l.includeEndpoint)
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&speciesCounts[l.valueID * DOUBLES_PER_AVX]), limitValue, _CMP_LE_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1);
                }
                else 
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&speciesCounts[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_LT_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1);
                } 
                break;
            case EH::MAX:
                if (l.includeEndpoint)
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&speciesCounts[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_GE_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1);
                }
                else
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&speciesCounts[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_GT_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1);
                } 
                break;
            case EH::INCREASING: throw Exception("unimplemented"); break;
            case EH::DECREASING: throw Exception("unimplemented"); break;
            } 
            break;

        case EH::ORDER_PARAMETER:
            switch (l.stoppingCondition)
            {
            case EH::MIN:
                if (l.includeEndpoint)
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterValues[l.valueID * DOUBLES_PER_AVX]), limitValue, _CMP_LE_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1);
                }
                else
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_LT_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1);
                }
                break;
            case EH::MAX:
                if (l.includeEndpoint)
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_GE_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1);
                }
                else
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_GT_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1);
                }
                break;
            case EH::DECREASING:
                if (l.includeEndpoint)
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterPreviousValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_GE_OQ);
                    comp2 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_LT_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1)&_mm256_movemask_pd(comp2);
                }
                else
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterPreviousValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_GT_OQ);
                    comp2 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_LE_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1)&_mm256_movemask_pd(comp2);
                }
                break;
            case EH::INCREASING:
                if (l.includeEndpoint)
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterPreviousValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_LE_OQ);
                    comp2 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_GT_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1)&_mm256_movemask_pd(comp2);
                }
                else
                {
                    comp1 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterPreviousValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_LT_OQ);
                    comp2 = _mm256_cmp_pd(_mm256_load_pd(&orderParameterValues[l.valueID*DOUBLES_PER_AVX]), limitValue, _CMP_GE_OQ);
                    outsideLimitMask = _mm256_movemask_pd(comp1)&_mm256_movemask_pd(comp2);
                }
                break;
            } 
            break;

        case EH::DEGREE_ADVANCEMENT:
            switch (l.stoppingCondition)
            {
            case EH::MIN: throw Exception("unimplemented"); break;
            case EH::MAX: throw Exception("unimplemented"); break;
            case EH::INCREASING: throw Exception("unimplemented"); break;
            case EH::DECREASING: throw Exception("unimplemented"); break;
            }
            break;
            
        default:
            break;
        }

        // Check if the limit was triggered.
        if (outsideLimitMask)
        {
            // Go through the mask.
            for (int j=0; j<DOUBLES_PER_AVX; j++)
            {
                // If this element was true, set that a limit was reached and make a note of which one.
                if (outsideLimitMask&(1<<j))
                {
                    status[j] = lm::message::WorkUnitStatus::LIMIT_REACHED;
                    limitIDReached[j] = l.limitID;
                    limitTypeReached[j] = l.type;
                }
                else
                {
                    // Otherwise set that we finished steps.
                    status[j] = lm::message::WorkUnitStatus::STEPS_FINISHED;
                }
            }
            return true;
        }
    }
    return false;
}


}
}

#endif
