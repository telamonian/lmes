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
#include <zlib.h>

#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/Tune.h"
#include "lm/Math.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/avx/GillespieDSolverAVX.h"
#include "lm/cme/CMESolver.h"
#include "lm/cme/ReactionModel.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ProcessWorkUnitOutput.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
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
namespace avx {

bool GillespieDSolverAVX::registered=GillespieDSolverAVX::registerClass();

bool GillespieDSolverAVX::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::me::MESolver","lm::cme::GillespieDSolverAVX",&GillespieDSolverAVX::allocateObject);
    return true;
}

void* GillespieDSolverAVX::allocateObject()
{
    return new GillespieDSolverAVX();
}

GillespieDSolverAVX::GillespieDSolverAVX()
:CMESolver((RandomGenerator::Distributions)(RandomGenerator::EXPONENTIAL|RandomGenerator::UNIFORM)),
timeLimit(_mm256_set1_pd(std::numeric_limits<double>::infinity())),speciesCounts(NULL),propensities(NULL),time(_mm256_set1_pd(0.0)),timeStep(_mm256_set1_pd(0.0))
{
    // Initialize any array variables.
    for (int i=0; i<DOUBLES_PER_AVX; i++)
    {
        status[i] = lm::message::WorkUnitStatus::NONE;
        limitReached[i] = lm::io::TrajectoryLimits::NONE;
        trajectoryStarted[i] = false;
    }
}

GillespieDSolverAVX::~GillespieDSolverAVX()
{
    // Free any state.
    if (speciesCounts != NULL) free(speciesCounts); speciesCounts = NULL;
    if (propensities != NULL) free(propensities); propensities = NULL;
}

uint GillespieDSolverAVX::getSimultaneousTrajectories()
{
    return DOUBLES_PER_AVX;
}

void GillespieDSolverAVX::reset()
{
    CMESolver::reset();

    // Reset any array variables.
    for (int i=0; i<DOUBLES_PER_AVX; i++)
    {
        status[i] = lm::message::WorkUnitStatus::NONE;
        limitReached[i] = lm::io::TrajectoryLimits::NONE;
        trajectoryStarted[i] = false;
    }

    // Free any previous state.
    if (speciesCounts != NULL) free(speciesCounts); speciesCounts = NULL;
    if (propensities != NULL) free(propensities); propensities = NULL;

    // Allocate species counts table.
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&speciesCounts, DOUBLES_PER_AVX*sizeof(double), reactionModel->numberSpecies*DOUBLES_PER_AVX*sizeof(double)));

    // Reset the species counts.
    for (int i=0; i<reactionModel->numberSpecies*DOUBLES_PER_AVX; i++)
    {
        speciesCounts[i] = 0.0;
    }

    // Allocate reaction propensities table.
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&propensities, DOUBLES_PER_AVX*sizeof(double), reactionModel->numberReactions*DOUBLES_PER_AVX*sizeof(double)));

    // Reset the propensities.
    for (int i=0; i<reactionModel->numberReactions*DOUBLES_PER_AVX; i++)
    {
        propensities[i] = 0.0;
    }
}

void GillespieDSolverAVX::getState(lm::io::TrajectoryState* state, uint trajectoryNumber)
{
    if (trajectoryNumber >= getSimultaneousTrajectories()) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());
}

void GillespieDSolverAVX::setState(const lm::io::TrajectoryState& state, uint trajectoryNumber)
{
    if (trajectoryNumber >= getSimultaneousTrajectories()) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());

    // Set the propensities to their initial values.
    updateAllPropensities(reactionModel->numberSpecies);
}

lm::message::WorkUnitStatus::Status GillespieDSolverAVX::getStatus(uint trajectoryNumber)
{
    if (trajectoryNumber >= getSimultaneousTrajectories()) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());
    return status[trajectoryNumber];
}

long long GillespieDSolverAVX::generateTrajectory(long long maxSteps)
{
    if (reactionModel == NULL) throw Exception("GillespieDSolverAVX did not have a reaction model.");
    if (propensities == NULL) throw Exception("GillespieDSolverAVX state was not initialized.");

    // Make sure we have propensity functions for every reaction.
    for (uint i=0; i<reactionModel->numberReactions; i++)
        if (reactionModel->propensityFunctions[i] == NULL)
            throw Exception("A reaction did not have a valid propensity function",i);

    // Create local copies of the data for efficiency.
    const uint numberSpecies = reactionModel->numberSpecies;
    const uint numberReactions = reactionModel->numberReactions;

    // Initialize the total propensity.
    avxd totalPropensity = _mm256_setzero_pd();
    for (uint i=0; i<numberReactions; i++)
    {
        avxd propensity = _mm256_load_pd(&propensities[i*DOUBLES_PER_AVX]);
        totalPropensity = _mm256_add_pd(totalPropensity,propensity);
    }

    // Create the output message.
    lm::message::Message msgp;
    lm::message::ProcessWorkUnitOutput* msg = msgp.mutable_process_work_unit_output();
    msg->set_work_unit_id(workUnitId);
    lm::message::WorkUnitOutput* output[DOUBLES_PER_AVX];
    for (int i=0; i<DOUBLES_PER_AVX; i++)
        output[i] = msg->add_part_output();

    // Get the interval for writing species counts.
    avxd eps = _mm256_set1_pd(EPS);
    avxd nextSpeciesWriteTime;
    vector<int32_t> speciesTimeSeriesCounts[DOUBLES_PER_AVX];
    vector<double> speciesTimeSeriesTimes[DOUBLES_PER_AVX];

    // If we are writing time steps, create the data set.
    if (writeSpeciesTimeSeries)
    {
        // See if this is the start of the trajectory.
        for (int i=0; i<DOUBLES_PER_AVX; i++)
        {
            // If this element was true, save the reaction and set the random propensity to inf.
            if (((double*)&time)[i] == 0.0 || trajectoryStarted[i]==false)
            {
                ((double*)&nextSpeciesWriteTime)[i] = speciesWriteInterval;
                for (uint j=0; j<reactionModel->numberSpeciesToTrack; j++) speciesTimeSeriesCounts[i].push_back(lround(speciesCounts[j*numberSpecies+i]));
                speciesTimeSeriesTimes[i].push_back(0.0);
            }
            else
            {
                ((double*)&nextSpeciesWriteTime)[i] = ceil(((double*)&time)[i]/speciesWriteInterval)*speciesWriteInterval;
            }
        }
    }

    // Local cache of random numbers.
    double* rngValues = NULL;
    double* expRngValues = NULL;
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&rngValues, DOUBLES_PER_AVX*sizeof(double), TUNE_LOCAL_RNG_CACHE_SIZE*sizeof(double)));
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&expRngValues, DOUBLES_PER_AVX*sizeof(double), TUNE_LOCAL_RNG_CACHE_SIZE*sizeof(double)));
    int rngNext=TUNE_LOCAL_RNG_CACHE_SIZE;

    // Run the direct method.
    Print::printf(Print::DEBUG, "Running Gillespie direct avx simulation for %d steps with %d species, %d reactions, %d species limits\n", maxSteps, reactionModel->numberSpecies, reactionModel->numberReactions, numberLimits);
    PROF_BEGIN(PROF_SIM_EXECUTE);
    long long steps=0;
    int allFalse;
    int trueMask;
    avxd comp;
    while (true)
    {
        // See if we have finished the steps.
        if (steps >= maxSteps)
        {
            for (int i=0; i<DOUBLES_PER_AVX; i++)
            {
                status[i] = lm::message::WorkUnitStatus::STEPS_FINISHED;
            }
            break;
        }

        // Increment the steps.
        steps++;

        // See if we need to update our rng caches.
        if (rngNext >= TUNE_LOCAL_RNG_CACHE_SIZE)
        {
            rng->getRandomDoubles(rngValues,TUNE_LOCAL_RNG_CACHE_SIZE, true);
            rng->getExpRandomDoubles(expRngValues,TUNE_LOCAL_RNG_CACHE_SIZE, true);
            rngNext=0;
        }

        // Calculate the time to the next reaction.
        avxd expR = _mm256_load_pd(&expRngValues[rngNext]);
        avxd timestep = _mm256_div_pd(expR, totalPropensity);
        time = _mm256_add_pd(time,timestep);

//        {
//        double* res = (double*)&expR;
//        printf("ERNG: %8.2e %8.2e %8.2e %8.2e\n", res[0], res[1], res[2], res[3]);
//        res = (double*)&timestep;
//        printf("TS:   %8.2e %8.2e %8.2e %8.2e\n", res[0], res[1], res[2], res[3]);
//        res = (double*)&time;
//        printf("TIME: %8.2e %8.2e %8.2e %8.2e\n", res[0], res[1], res[2], res[3]);
//        }

         // If any new time is past the end time, we are done.
        comp = _mm256_cmp_pd(time, timeLimit, _CMP_GE_OQ);
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
                    limitReached[i] = lm::io::TrajectoryLimits::MAXTIME;
                }
                else
                {
                    // Otherwise set that we finsihed steps.
                    status[i] = lm::message::WorkUnitStatus::STEPS_FINISHED;
                }
            }
            break;
        }

        // If we are writing time steps, write out any time steps before this event occurred.
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
                        for (uint j=0; j<reactionModel->numberSpeciesToTrack; j++) speciesTimeSeriesCounts[i].push_back(lround(speciesCounts[j*numberSpecies+i]));
                        speciesTimeSeriesTimes[i].push_back(((double*)&nextSpeciesWriteTime)[i]);
                        ((double*)&nextSpeciesWriteTime)[i] += speciesWriteInterval;
                    }
                }
            }
        }

        // Calculate a random propensity to figure out the reaction.
        avxd rngValue = _mm256_load_pd(&rngValues[rngNext]);
        avxd rngPropensity = _mm256_mul_pd(rngValue, totalPropensity);

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
        for (uint r=0; r<(numberReactions-1); r++)
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
        performReactionEvent(reactionsToPerform);

        // If we are outside of the limits, stop the trajectory.
        if (isTrajectoryOutsideLimits())
            break;

        // Update the propensites given the reaction that occurred.
        //updatePropensities(time, r);
        updateAllPropensities(numberSpecies);

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
                    status[i] = lm::message::WorkUnitStatus::ERROR;
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

         // Go to the next rng pair.
        rngNext+=DOUBLES_PER_AVX;
    }
    PROF_END(PROF_SIM_EXECUTE);

//    double* p = (double*)&time;
//    printf("Final Time:             %8.2f %8.2f %8.2f %8.2f\n", p[0], p[1], p[2], p[3]);
//    p = (double*)speciesCounts;
//    printf("Final Species counts:   %8.2f %8.2f %8.2f %8.2f\n", p[0], p[1], p[2], p[3]);
//    //printf("Final Species counts:   %8.2f %8.2f %8.2f %8.2f\n", p[4], p[5], p[6], p[7]);
//    p = (double*)&totalPropensity;
//    printf("Final Total Propensity: %8.2f %8.2f %8.2f %8.2f\n", p[0], p[1], p[2], p[3]);



    // Delete the rng caches.
    free(rngValues);
    rngValues = NULL;
    free(expRngValues);
    expRngValues = NULL;

    for (int i=0; i<DOUBLES_PER_AVX; i++)
    {
        printf("Trajectory %d (%lu)\n---------------------\n",i,speciesTimeSeriesCounts[i].size());
        for (int j=0; j<speciesTimeSeriesCounts[i].size(); j++)
        {
            printf("%12.4f: %6d\n",speciesTimeSeriesTimes[i][j],speciesTimeSeriesCounts[i][j]);
        }
        printf("---------------------\n");
    }

//    bool reachedLimit = false;

//    // If we finished the total time or ran out of reactions, write out the remaining time steps.
//    if (time >= maxTime || totalPropensity <= 0)
//    {
//        time = maxTime;
//        Print::printf(Print::DEBUG, "Generated trajectory through time %e.", time);
//        if (writeSpeciesTimeSeries)
//        {
//            while (nextSpeciesWriteTime <= (maxTime+1e-9))
//            {
//                // Record the species counts.
//                for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
//                speciesTimeSeriesTimes.push_back(nextSpeciesWriteTime);
//                nextSpeciesWriteTime += speciesWriteInterval;
//            }

//            // If we are recording parameter values, write out the remaining value intervals.
//    //        if (nextParameterWriteTime <= (maxTime+1e-9))
//    //        {
//    ////            recordParameters(nextParameterWriteTime, parameterWriteInterval, maxTime);
//    //        }
//        }
//        reachedLimit = true;
//    }

//    // See if we finished all of the steps.
//    else if (steps >= maxSteps)
//    {
//        Print::printf(Print::DEBUG, "Generated trajectory with %llu steps through time %e.", steps, time);
//    }

//    // Otherwise we must have finished because of a species/order parameter limit, so just write out the last time.
//    else
//    {
//        // Record the species counts.
//        if (writeSpeciesTimeSeries)
//        {
//            // Record the species counts.
//            for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
//            speciesTimeSeriesTimes.push_back(time);
//        }
//        reachedLimit = true;
//    }

//    // If we have any species time series data, add them to the output message.
//    if (speciesTimeSeriesCounts.size() > 0 || speciesTimeSeriesTimes.size() > 0)
//    {
//        // Make sure the arrays are of a consistent size.
//        if (speciesTimeSeriesCounts.size() == speciesTimeSeriesTimes.size()*reactionModel->numberSpeciesToTrack)
//        {
//            lm::io::SpeciesTimeSeries* speciesTimeSeriesDataSet = msg->mutable_species_time_series();
//            speciesTimeSeriesDataSet->set_trajectory_id(trajectoryId);

//            robertslab::pbuf::NDArray* counts = speciesTimeSeriesDataSet->mutable_counts();
//            counts->set_data_type(robertslab::pbuf::NDArray::int32);
//            counts->set_compressed_deflate(true);
//            counts->add_shape(speciesTimeSeriesTimes.size());
//            counts->add_shape(reactionModel->numberSpeciesToTrack);
//            std::string* data = counts->mutable_data();
//            size_t dataSizeEstimate=compressBound(speciesTimeSeriesCounts.size()*sizeof(int32_t));
//            data->resize(dataSizeEstimate);
//            ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*data)[0]), &dataSizeEstimate, (unsigned char*)speciesTimeSeriesCounts.data(), speciesTimeSeriesCounts.size()*sizeof(int32_t)));
//            data->resize(dataSizeEstimate);

//            robertslab::pbuf::NDArray* times = speciesTimeSeriesDataSet->mutable_times();
//            times->set_data_type(robertslab::pbuf::NDArray::float64);
//            times->set_compressed_deflate(true);
//            times->add_shape(speciesTimeSeriesTimes.size());
//            data = times->mutable_data();
//            dataSizeEstimate=compressBound(speciesTimeSeriesTimes.size()*sizeof(double));
//            data->resize(dataSizeEstimate);
//            ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*data)[0]), &dataSizeEstimate, (unsigned char*)speciesTimeSeriesTimes.data(), speciesTimeSeriesTimes.size()*sizeof(double)));
//            data->resize(dataSizeEstimate);
//        }
//        else
//        {
//            Print::printf(Print::ERROR, "Species time series counts and time mismatch %d,%d,%d", speciesTimeSeriesCounts.size(), reactionModel->numberSpeciesToTrack, speciesTimeSeriesTimes.size());
//        }
//    }

//    // If the simulation reached a limit and we are tracking first passage times, add them to the output message.
//    if (reachedLimit && numberFptTrackedSpecies > 0)
//    {
//        for (int i=0; i<numberFptTrackedSpecies; i++)
//        {
//            fptTrackedSpecies[i].serializeTo(trajectoryId, msg->add_first_passage_times());
//        }
//    }

//    // If the output message has any data, send it.
//    if ((msg->has_species_time_series() || msg->first_passage_times_size() > 0) && !ffluxFlag)		// these messages aren't useful for fflux simulation
//    {
////    	printf("GillespieDSolverAVX outputProcess: %d outputThread: %d\n", outputProcess, outputThread);
//        communicator->sendMessage(outputProcess, outputThread, &msgp);
//    }

    return steps;
}

void GillespieDSolverAVX::updateAllPropensities(const uint numberSpecies)
{
    // Update the propensities.
    for (uint i=0; i<reactionModel->numberReactions; i++)
    {
        avxd propensity = reactionModel->propensityFunctions[i]->calculateAvx(time, speciesCounts, numberSpecies);
        _mm256_store_pd(&propensities[i*DOUBLES_PER_AVX], propensity);
    }
}

//void GillespieDSolverAVX::updatePropensities(avxd time, uint sourceReaction)
//{
//    // Update the propensities of the dependent reactions.
//    for (uint i=0; i<reactionModel->numberDependentReactions[sourceReaction]; i++)
//    {
//        uint r = reactionModel->dependentReactions[sourceReaction][i];
//        propensities[r] = reactionModel->propensityFunctions[i]->calculateAvx(time, speciesCounts, reactionModel->numberSpecies);
//    }
//}

void GillespieDSolverAVX::performReactionEvent(uint* reactionsToPerform)
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
}

bool GillespieDSolverAVX::isTrajectoryOutsideLimits()
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
    return false;
}


}
}

#endif
