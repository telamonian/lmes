/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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

#include <iostream>
#include <string>
#include <map>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <google/protobuf/stubs/common.h>
#include "lm/Print.h"
#include "lm/Exceptions.h"
#include "lm/Types.h"
#include "lm/Math.h"
#ifdef OPT_CUDA
#include "lm/Cuda.h"
#endif
#include "lm/io/hdf5/HDF5.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.h"
#include "lm/main/DataOutputQueue.h"
#include "lm/main/LocalDataOutputWorker.h"
#include "lm/main/Main.h"
#include "lm/main/SignalHandler.h"
#include "lm/main/ReplicateRunner.h"
#include "lm/main/ResourceAllocator.h"
#include "lm/message/SimulationParameters.pb.h"
#include "lm/thread/Thread.h"
#include "lm/thread/WorkerManager.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using std::map;
using std::list;
using lm::Print;
using lm::Exception;
using lm::main::ReplicateRunner;
using lm::main::ResourceAllocator;
using lm::me::MESolverFactory;
using lm::thread::PthreadException;

void listDevices();
void executeSimulation();
ReplicateRunner * startReplicate(int replicate, MESolverFactory solverFactory, std::map<std::string,string> & simulationParameters, lm::io::ReactionModel * reactionModel, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize, ResourceAllocator & resourceAllocator) throw(Exception,PthreadException);
ReplicateRunner * popNextFinishedReplicate(list<ReplicateRunner *> & runningReplicates, ResourceAllocator & resourceAllocator);

// Allocate the profile space.
PROF_ALLOC;

int main(int argc, char** argv)
{	
    // Make sure we are using the correct protocol buffers library.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    PROF_INIT;
	try
	{
		printCopyright(argc, argv);
		parseArguments(argc, argv);
		
		if (functionOption == "help")
		{
			printUsage(argc, argv);
		}
		else if (functionOption == "version")
		{
		}
		else if (functionOption == "devices")
		{
		    listDevices();
		}
        else if (functionOption == "simulation")
        {
            executeSimulation();
        }
		else
		{
			throw lm::CommandLineArgumentException("unknown function.");
		}
        Print::printf(Print::INFO, "Program execution finished.");
	    PROF_WRITE;
	    google::protobuf::ShutdownProtobufLibrary();
		return 0;
	}
    catch (lm::CommandLineArgumentException & e)
    {
    	std::cerr << "Invalid command line argument: " << e.what() << std::endl << std::endl;
        printUsage(argc, argv);
    }
    catch (lm::Exception & e)
    {
    	std::cerr << "Exception during execution: " << e.what() << std::endl;
    }
    catch (std::exception & e)
    {
    	std::cerr << "Exception during execution: " << e.what() << std::endl;
    }
    catch (...)
    {
    	std::cerr << "Unknown Exception during execution." << std::endl;
    }
    PROF_WRITE;
    google::protobuf::ShutdownProtobufLibrary();
    return -1;
}

void listDevices()
{
    printf("Running with %d/%d processor(s)", numberCpuCores, getPhysicalCpuCores());

    #ifdef OPT_CUDA
    printf(" and %d/%d CUDA device(s)", (int)cudaDevices.size(), lm::CUDA::getNumberDevices());
    #endif
    printf(".\n");

    #ifdef OPT_CUDA
    if (shouldPrintCudaCapabilities)
    {
        for (int i=0; i<(int)cudaDevices.size(); i++)
        {
            printf("  %s\n", lm::CUDA::getCapabilitiesString(cudaDevices[i]).c_str());
        }
    }
    #endif
}

void executeSimulation()
{
    PROF_SET_THREAD(0);
    PROF_BEGIN(PROF_SIM_RUN);

    Print::printf(Print::DEBUG, "Master process started.");

    // Create the resource allocator, subtract one core for the data output thread.
    #ifdef OPT_CUDA
    Print::printf(Print::INFO, "Using %d processor(s) and %d CUDA device(s) per process.", numberCpuCores, (int)cudaDevices.size());
    Print::printf(Print::INFO, "Assigning %0.2f processor(s) and %0.2f CUDA device(s) per replicate.", cpuCoresPerReplicate, cudaDevicesPerReplicate);
    ResourceAllocator resourceAllocator(numberCpuCores-1, cpuCoresPerReplicate, cudaDevices, cudaDevicesPerReplicate);
    #else
    Print::printf(Print::INFO, "Using %d processor(s) per process.", numberCpuCores);
    Print::printf(Print::INFO, "Assigning %0.2f processor(s) per replicate.", cpuCoresPerReplicate);
    ResourceAllocator resourceAllocator(numberCpuCores-1, cpuCoresPerReplicate);
    #endif

    // Create a worker to handle any signals.
    lm::main::SignalHandler * signalHandler = new lm::main::SignalHandler();
    signalHandler->start();

    // Create the checkpoint signaler.
    lm::main::CheckpointSignaler * checkpointSignaler = new lm::main::CheckpointSignaler();
    checkpointSignaler->start();
    checkpointSignaler->startCheckpointing(checkpointInterval);

    // Open the file.
    lm::io::hdf5::SimulationFile * file = new lm::io::hdf5::SimulationFile(simulationFilename);

    // Start the data output thread.
    lm::main::LocalDataOutputWorker * dataOutputWorker = new lm::main::LocalDataOutputWorker(file);
    dataOutputWorker->start();

    // Set the data output handler to be the worker.
    lm::main::DataOutputQueue::setInstance(dataOutputWorker);

    // Create a table for tracking the replicate assignments.
    int assignedSimulations=0;

    // Get the maximum number of simulations that can be started on each process.
    int maxSimulations = resourceAllocator.getMaxSimultaneousReplicates();
    Print::printf(Print::INFO, "Number of simultaneous replicates is %d", maxSimulations);
    if (maxSimulations == 0) throw Exception("Invalid configuration, no replicates can be processed.");

    // Create a table for the simulation status.
    map<int,int> simulationStatusTable;
    map<int,time_t> simulationStartTimeTable;
    for (vector<int>::iterator it=replicates.begin(); it<replicates.end(); it++)
    {
        simulationStatusTable[*it] = 0;
        simulationStartTimeTable[*it] = 0;
    }

    // Get the simulation parameters.
    std::map<std::string,string> simulationParameters = file->getParameters();

    // Get the reaction model.
    lm::io::ReactionModel reactionModel;
    if (solverFactory.needsReactionModel())
    {
        file->getReactionModel(&reactionModel);
    }

    // Get the diffusion model.
    lm::io::DiffusionModel diffusionModel;
    uint8_t * lattice=NULL, * latticeSites=NULL;
    size_t latticeSize=0, latticeSitesSize=0;
    if (solverFactory.needsDiffusionModel())
    {
        file->getDiffusionModel(&diffusionModel);
        latticeSize = diffusionModel.lattice_x_size()*diffusionModel.lattice_y_size()*diffusionModel.lattice_z_size()*diffusionModel.particles_per_site();
        lattice = new uint8_t[latticeSize];
        latticeSitesSize = diffusionModel.lattice_x_size()*diffusionModel.lattice_y_size()*diffusionModel.lattice_z_size();
        latticeSites = new uint8_t[latticeSitesSize];
        file->getDiffusionModelLattice(&diffusionModel, lattice, latticeSize, latticeSites, latticeSitesSize);
    }

    // Distribute the simulations to the processes.
    Print::printf(Print::INFO, "Starting %d replicates from file %s.", replicates.size(), simulationFilename.c_str());
    list<ReplicateRunner *> runningReplicates;
    unsigned long long noopLoopCycles=0;
    while (!globalAbort)
    {
        // Increment the loop counter.
        noopLoopCycles++;

        // Check for finished simulations in our process.
        ReplicateRunner * finishedReplicate;
        while ((finishedReplicate=popNextFinishedReplicate(runningReplicates, resourceAllocator)) != NULL)
        {
            PROF_BEGIN(PROF_MASTER_FINISHED_THREAD);
            Print::printf(Print::INFO, "Replicate %d completed with exit code %d in %d seconds.", finishedReplicate->getReplicate(), finishedReplicate->getReplicateExitCode(), time(NULL)-simulationStartTimeTable[finishedReplicate->getReplicate()]);
            assignedSimulations--;
            simulationStatusTable[finishedReplicate->getReplicate()] = 2;
            noopLoopCycles = 0;
            finishedReplicate->stop();
            delete finishedReplicate; // We are responsible for deleting the replicate runner.
            PROF_END(PROF_MASTER_FINISHED_THREAD);
        }

        // See if we need to start any new simulations and then wait a while.
        if (noopLoopCycles > 1000)
        {
            // Find a simulation to perform.
            int replicate = -1;
            bool allFinished=true;
            for (vector<int>::iterator it=replicates.begin(); it<replicates.end(); it++)
            {
                if (simulationStatusTable[*it] == 0)
                {
                    replicate = *it;
                    allFinished = false;
                    break;
                }
                if (simulationStatusTable[*it] != 2) allFinished = false;
            }

            // If all of the simulations are finished, we are done.
            if (allFinished) break;

            // Find a process to perform the simulation.
            if (replicate >= 0 && assignedSimulations < maxSimulations)
            {
				// Otherwise it must be us, so start the replicate.
				runningReplicates.push_back(startReplicate(replicate, solverFactory, simulationParameters, &reactionModel, &diffusionModel, lattice, latticeSize, latticeSites, latticeSitesSize, resourceAllocator));

				assignedSimulations++;
				simulationStatusTable[replicate] = 1;
				simulationStartTimeTable[replicate] = time(NULL);
				continue;
            }

            PROF_BEGIN(PROF_MASTER_SLEEP);
            unsigned int sleepTime = 1000000;
            if (noopLoopCycles > 2000) sleepTime = 10000000;
            if (noopLoopCycles > 2100) sleepTime = 100000000;
            if (noopLoopCycles >= 3000 && noopLoopCycles%1000 == 0)
            {
                int replicatesRunning=0, replicatesRemaining=0;
                for (vector<int>::iterator it=replicates.begin(); it<replicates.end(); it++)
                {
                    if (simulationStatusTable[*it] == 0)
                        replicatesRemaining++;
                    else if (simulationStatusTable[*it] == 1)
                        replicatesRunning++;
                }
                Print::printf(Print::INFO, "Master sleeping, waiting for %d replicates to finish, %d left to start.",replicatesRunning,replicatesRemaining);
            }
            struct timespec requested, remainder;
            requested.tv_sec  = 0;
            requested.tv_nsec = sleepTime;
            if (nanosleep(&requested, &remainder) != 0 && errno != EINTR) throw lm::Exception("Sleep failed.");
            PROF_END(PROF_MASTER_SLEEP);
        }
    }

    Print::printf(Print::INFO, "Master shutting down.");

    // Stop checkpointing.
    checkpointSignaler->stopCheckpointing();


    // If this was a global abort, stop the workers quickly.
    if (globalAbort)
    {
        Print::printf(Print::WARNING, "Aborting worker threads.");
        lm::thread::WorkerManager::getInstance()->abortWorkers();
    }

    // Otherwise, let them finish at their own pace.
    else
    {
        Print::printf(Print::DEBUG, "Stopping worker threads.");
        lm::thread::WorkerManager::getInstance()->stopWorkers();
    }

    // Close the simulation file.
    delete file;
    Print::printf(Print::INFO, "Simulation file closed.");

    // Cleanup any resources.
    delete checkpointSignaler;
    delete signalHandler;
    delete dataOutputWorker;
    if (lattice != NULL) delete [] lattice; lattice = NULL;
    if (latticeSites != NULL) delete [] latticeSites; latticeSites = NULL;

    Print::printf(Print::DEBUG, "Master process finished.");


    PROF_END(PROF_SIM_RUN);
}

ReplicateRunner * startReplicate(int replicate, MESolverFactory solverFactory, std::map<std::string,string> & simulationParameters, lm::io::ReactionModel * reactionModel, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize, ResourceAllocator & resourceAllocator) throw(Exception,PthreadException)
{
    // Allocate resources for the replicate.
    ResourceAllocator::ComputeResources resources = resourceAllocator.assignReplicate(replicate);

    // Start a new thread for the replicate.
    Print::printf(Print::DEBUG, "Starting replicate %d (%s).", replicate, resources.toString().c_str());
    ReplicateRunner * runner = new ReplicateRunner(replicate, solverFactory, &simulationParameters, reactionModel, diffusionModel, lattice, latticeSize, latticeSites, latticeSitesSize, resources);
    runner->start();
    return runner;
}

ReplicateRunner * popNextFinishedReplicate(list<ReplicateRunner *> & runningReplicates, ResourceAllocator & resourceAllocator)
{
    for (list<ReplicateRunner *>::iterator it=runningReplicates.begin(); it != runningReplicates.end(); it++)
    {
        ReplicateRunner * runner = *it;
        if (runner->hasReplicateFinished())
        {
            runningReplicates.erase(it);
            resourceAllocator.removeReplicate(runner->getReplicate());
            return runner;
        }
    }
    return NULL;
}
