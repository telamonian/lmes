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

long long GillespieDSolver::generateTrajectory(long long maxSteps)
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
    double writeInterval = atof(simulationParameters["writeInterval"].c_str());
    bool writeTimeSteps = (writeInterval > 0.0);
    double nextSpeciesWriteTime;
    vector<int32_t> speciesTimeSeriesCounts;
    vector<double> speciesTimeSeriesTimes;

    // Get the interval for writing order parameters.
    double writeIntervalOP = atof(simulationParameters["writeIntervalOP"].c_str());
    bool writeTimeStepsOP = (writeIntervalOP > 0.0);
    double nextOrderParameterWriteTime;
    vector<double> orderParameterTimeSeriesCounts, orderParameterTimeSeriesTimes;

    // If we are writing time steps, create the data set.
    if (writeTimeSteps)
    {
        // If this is the start of the trajectory, add the initial counts.
        if ((time == 0.0 || trajectoryStarted==false) && !ffluxFlag)
        {
        	//printf("traj_id %d has_started %d\n", trajectoryId, trajectoryStarted);
            nextSpeciesWriteTime = ceil(time/writeInterval)*writeInterval;
            for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
            speciesTimeSeriesTimes.push_back(time);
        }
        else
        {
            nextSpeciesWriteTime = ceil(time/writeInterval)*writeInterval;
        }
    }

    // If we are writing order parameter time steps, create the data set.
    if (writeTimeStepsOP)
    {
        // If this is the start of the trajectory, add the initial counts.
        if ((time == 0.0 || trajectoryStarted==false) && !ffluxFlag)
        {
            nextOrderParameterWriteTime = ceil(time/writeIntervalOP)*writeIntervalOP;
            for (uint i=0; i<oparams->size(); i++) orderParameterTimeSeriesCounts.push_back(orderParameterCounts[i]);
            orderParameterTimeSeriesTimes.push_back(time);
        }
        else
        {
            nextOrderParameterWriteTime = ceil(time/writeIntervalOP)*writeIntervalOP;
        }
    }

    // Local cache of random numbers.
    double rngValues[TUNE_LOCAL_RNG_CACHE_SIZE];
    double expRngValues[TUNE_LOCAL_RNG_CACHE_SIZE];
    rng->getRandomDoubles(rngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
    rng->getExpRandomDoubles(expRngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
    int rngNext=0;

//    if (trajectoryID==0)
//    {
//        if (trajectoryStarted==false)
//        {
//            printf("trajectoryID: %d starting with\n", trajectoryID);
//        }
//        else
//        {
//            printf("trajectoryID: %d restarting with\n", trajectoryID);
//        }
//        printf("order parameter: %.3f\n", (*oparams)[0]->get());
//        printf("species counts: ");
//        for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) printf("%d,", speciesCounts[i]);
//        printf("\n");
//        printf("time: %.10f\n", time);
//    }

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
        timeStep = expR/totalPropensity;
        time += timeStep;

         // If the new time is past the end time, we are done.
        if (time >= maxTime)
        {
            break;
        }

        // If we are writing species time steps, write out any species time steps before this event occurred.
        if (writeTimeSteps)
        {
            // Write species time steps until the next write time is past the current time.
            while (nextSpeciesWriteTime <= (time+1e-9))
            {
                // Record the species counts.
                for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
                speciesTimeSeriesTimes.push_back(nextSpeciesWriteTime);
                nextSpeciesWriteTime += writeInterval;
            }
        }

        // If we are writing order parameter time steps, write out any order parameter time steps before this event occurred.
        if (writeTimeStepsOP)
        {
            // Write order parameter time steps until the next write time is past the current time.
            while (nextOrderParameterWriteTime <= (time+1e-9))
            {
                // Record the order parameter counts.
                for (uint i=0; i<oparams->size(); i++) orderParameterTimeSeriesCounts.push_back(orderParameterCounts[i]);
                orderParameterTimeSeriesTimes.push_back(nextOrderParameterWriteTime);
                nextOrderParameterWriteTime += writeIntervalOP;
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

//    if (trajectoryID==0 && steps==1000)
//    {
//        printf("trajectoryID: %d stopping with\n", trajectoryID);
//        printf("order parameter: %.3f\n", (*oparams)[0]->get());
//        printf("species counts: ");
//        for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) printf("%d,", speciesCounts[i]);
//        printf("\n");
//        printf("steps: %d\n", steps);
//        printf("time: %.10f\n", time);
//    }

    bool reachedLimit = false;

    // If we finished the total time or ran out of reactions, write out the remaining time steps.
    if (time >= maxTime || totalPropensity <= 0)
    {
        time = maxTime;
        Print::printf(Print::DEBUG, "Generated trajectory through time %e.", time);
        if (writeTimeSteps && !ffluxFlag)
        {
            while (nextSpeciesWriteTime <= (maxTime+1e-9))
            {
                // Record the species counts.
                for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
                speciesTimeSeriesTimes.push_back(nextSpeciesWriteTime);
                nextSpeciesWriteTime += writeInterval;
            }

            // If we are recording parameter values, write out the remaining value intervals.
    //        if (nextParameterWriteTime <= (maxTime+1e-9))
    //        {
    ////            recordParameters(nextParameterWriteTime, parameterWriteInterval, maxTime);
    //        }
        }

        if (writeTimeStepsOP && !ffluxFlag)
        {
            while (nextOrderParameterWriteTime <= (maxTime+1e-9))
            {
                // Record the order parameter counts.
                for (uint i=0; i<oparams->size(); i++) orderParameterTimeSeriesCounts.push_back(orderParameterCounts[i]);
                orderParameterTimeSeriesTimes.push_back(nextOrderParameterWriteTime);
                nextOrderParameterWriteTime += writeIntervalOP;
            }
        }
        reachedLimit = true;
    }

    // Otherwise we must have finished because of a species/order parameter limit, so just write out the last time.
    else if (reachedSpeciesLimit())
    {
        // Record the species counts.
        if (writeTimeSteps && !ffluxFlag)
        {
            for (uint i=0; i<reactionModel->numberSpeciesToTrack; i++) speciesTimeSeriesCounts.push_back(speciesCounts[i]);
            speciesTimeSeriesTimes.push_back(time);
        }

        // Record the order parameter counts.
        if (writeTimeStepsOP && !ffluxFlag)
        {
            for (uint i=0; i<oparams->size(); i++) orderParameterTimeSeriesCounts.push_back(orderParameterCounts[i]);
            orderParameterTimeSeriesTimes.push_back(nextOrderParameterWriteTime);
        }
        reachedLimit = true;
    }

    // See if we finished all of the steps.
    else //if (steps >= maxSteps)
    {
        Print::printf(Print::DEBUG, "Generated trajectory with %llu steps through time %e.", steps, time);
    }

    // If we have any species time series data, add them to the output message.
    if (speciesTimeSeriesCounts.size() > 0 || speciesTimeSeriesTimes.size() > 0)
    {
        // Make sure the arrays are of a consistent size.
        if (speciesTimeSeriesCounts.size() == speciesTimeSeriesTimes.size()*reactionModel->numberSpeciesToTrack)
        {
            lm::io::SpeciesTimeSeries* speciesTimeSeriesDataSet = msg->mutable_species_time_series();
            speciesTimeSeriesDataSet->set_trajectory_id(trajectoryID);

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

    // If we have any order parameter time series data, add them to the output message.
    if (orderParameterTimeSeriesCounts.size() > 0 || orderParameterTimeSeriesTimes.size() > 0)
    {
        // Make sure the arrays are of a consistent size.
        if (orderParameterTimeSeriesCounts.size() == orderParameterTimeSeriesTimes.size()*oparams->size())
        {
            lm::io::SpeciesTimeSeries* orderParameterTimeSeriesDataSet = msg->mutable_order_parameter_time_series();
            orderParameterTimeSeriesDataSet->set_trajectory_id(trajectoryID);

            opCounts.setBuf(orderParameterTimeSeriesDataSet->mutable_counts());
            opCounts.shape() << orderParameterTimeSeriesTimes.size() << oparams->size();
            opCounts.set_data(orderParameterTimeSeriesCounts, robertslab::pbuf::NDArray::float64);

            opTimes.setBuf(orderParameterTimeSeriesDataSet->mutable_times());
            opTimes.shape() << orderParameterTimeSeriesTimes.size();
            opTimes.set_data(orderParameterTimeSeriesTimes, robertslab::pbuf::NDArray::float64);
        }
        else
        {
            Print::printf(Print::ERROR, "Order parameter time series counts and time mismatch %d,%d,%d", orderParameterTimeSeriesCounts.size(), oparams->size(), orderParameterTimeSeriesTimes.size());
        }
    }

    // If the simulation reached a limit and we are tracking first passage times, add them to the output message.
    if (reachedLimit && numberFptTrackedSpecies > 0)
    {
        for (int i=0; i<numberFptTrackedSpecies; i++)
        {
            fptTrackedSpecies[i].serializeTo(trajectoryID, msg->add_first_passage_times());
        }
    }

    // If the output message has any data, send it.
//    if ((msg->has_species_time_series() && msg->species_time_series().number_entries() > 0) || msg->first_passage_times_size() > 0)
    if (msg->has_order_parameter_time_series() || msg->has_species_time_series() || msg->first_passage_times_size() > 0)
    {
        // for fflux simulation, the output process/thread values are ultimately set in FFluxSupervisor::receivedStartedOutputWriter
        communicator->sendMessage(outputProcess, outputThread, &msgp);
    }

    if (reachedLimit && steps>=maxSteps)
    {
        steps = maxSteps - 1;
    }

    return steps;
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
