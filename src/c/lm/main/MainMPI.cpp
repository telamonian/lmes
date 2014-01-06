/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012 Roberts Group,
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

#include <iostream>
#include <string>
#include <map>
#include <cstdio>
#include <cstring>
#include <ctime>
#if defined(MACOSX)
#include <sys/time.h>
#endif
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
#include "lm/MPI.h"
#ifdef OPT_CUDA
#include "lm/Cuda.h"
#endif
#include "lm/io/hdf5/HDF5.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.h"
#include "lm/main/BruteRunner.h"
#include "lm/main/CheckpointSignaler.h"
#include "lm/main/DataOutputQueue.h"
#include "lm/main/ForwardFluxRunner.h"
#include "lm/main/LocalDataOutputWorker.h"
#include "lm/main/LocalReplicateSupervisor.h"
#include "lm/main/MPINodeResourceMap.h"
#include "lm/main/MPIRemoteDataOutputQueue.h"
#include "lm/main/Main.h"
#include "lm/main/ReplicateManager.h"
#include "lm/main/ReplicateRunner.h"
#include "lm/main/ResourceAllocator.h"
#include "lm/main/SignalHandler.h"
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
using lm::main::BruteRunner;
using lm::main::ForwardFluxRunner;
using lm::main::ResourceAllocator;
using lm::main::MPINodeResourceMap;
using lm::me::MESolverFactory;
using lm::thread::PthreadException;

void listDevicesMPI();
void executeSimulationMPI();
void executeSimulationMPISingleMaster();
void executeSimulationMPISingleSlave();
//void broadcastSimulationParameters(void * staticDataBuffer, map<string,string> & simulationParameters);
//void broadcastReactionModel(void * staticDataBuffer, lm::io::ReactionModel * reactionModel);
//void broadcastDiffusionModel(void * staticDataBuffer, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize);
//map<string,string> receiveSimulationParameters(void * staticDataBuffer);
//void receiveReactionModel(void * staticDataBuffer, lm::io::ReactionModel * reactionModel);
//void receiveDiffusionModel(void * staticDataBuffer, lm::io::DiffusionModel * diffusionModel, uint8_t ** lattice, size_t * latticeSize, uint8_t ** latticeSites, size_t * latticeSitesSize);
//ReplicateRunner * startReplicate(int replicate, MESolverFactory solverFactory, std::map<std::string,string> & simulationParameters, lm::io::ReactionModel * reactionModel, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize, ResourceAllocator & resourceAllocator) throw(Exception,PthreadException);
//ReplicateRunner * popNextFinishedReplicate(list<ReplicateRunner *> & runningReplicates, ResourceAllocator & resourceAllocator);

// Allocate the profile space.
PROF_ALLOC;

int main(int argc, char** argv)
{
    // Make sure we are using the correct protocol buffers library.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    PROF_INIT;
    try
    {
        // Initialize the MPI library.
        lm::MPI::init(argc, argv);

        // If this is the master process, catch any problems with the command line arguments.
        int startAllProcesses=0;
        if (lm::MPI::worldRank == lm::MPI::MASTER)
        {
            //Print the startup messages.
            printCopyright(argc, argv);
            lm::MPI::printCapabilities();

            // Parse the command line arguments.
            try
            {
                parseArguments(argc, argv);

                if (functionOption == "help")
                {
                    // Handle help on the master process.
                    printUsage(argc, argv);
                }
                else if (functionOption == "version")
                {
                    // Handle version on the master process.
                }
                else if (functionOption == "devices" || functionOption == "simulation")
                {
                	// Mark that we need to start all of the processes.
                	startAllProcesses = 1;
                }
                else
                {
                    throw lm::CommandLineArgumentException("unknown function.");
                }
            }
            catch (lm::CommandLineArgumentException e)
            {
                std::cerr << "Invalid command line argument: " << e.what() << std::endl << std::endl;
                printUsage(argc, argv);
            }
        }

        // Broadcast some startup info.
        MPI_EXCEPTION_CHECK(MPI_Bcast(&startAllProcesses,1,MPI_INT,lm::MPI::MASTER,MPI_COMM_WORLD));

        // See if we should start all of the processes.
        if (startAllProcesses)
        {
            // Parse the arguments again on all processes.
            parseArguments(argc, argv);

			// Get the hostname.
			char hostname[MPI_MAX_PROCESSOR_NAME+1];
			int hostnameLength;
			memset(hostname,0,sizeof(hostname));
			MPI_EXCEPTION_CHECK(MPI_Get_processor_name(hostname, &hostnameLength));

			// Create the resource list on the master.
			if (lm::MPI::worldRank == lm::MPI::MASTER)
			{
				// Receive all of the host names.
				char* hostnameTable = new char[sizeof(hostname)*lm::MPI::worldSize];
				MPI_EXCEPTION_CHECK(MPI_Gather(hostname,sizeof(hostname),MPI_CHAR,hostnameTable,sizeof(hostname),MPI_CHAR,lm::MPI::MASTER,MPI_COMM_WORLD));

				// Extract the hostnames.
				list<string> hostnames;
				for (int i=0; i<lm::MPI::worldSize; i++)
					hostnames.push_back(string(&hostnameTable[i*sizeof(hostname)]));

				MPINodeResourceMap resourceList(hostnames, numberCpuCores);

				// Send the resource map.
				MPI_EXCEPTION_CHECK(MPI_Scatter(resourceList.getCpuCoresTable(),1,MPI_INT,&numberCpuCores,1,MPI_INT,lm::MPI::MASTER,MPI_COMM_WORLD));
			}
			else
			{
				// Send all of the host names.
				MPI_EXCEPTION_CHECK(MPI_Gather(hostname,sizeof(hostname),MPI_CHAR,NULL,sizeof(hostname),MPI_CHAR,lm::MPI::MASTER,MPI_COMM_WORLD));

				// Receive the resource map.
				MPI_EXCEPTION_CHECK(MPI_Scatter(NULL,1,MPI_INT,&numberCpuCores,1,MPI_INT,lm::MPI::MASTER,MPI_COMM_WORLD));
			}

            // Perform the requested function.
            if (functionOption == "devices")
            {
                listDevicesMPI();
            }
            else if (functionOption == "simulation")
            {
                executeSimulationMPI();
            }
        }

        // Close the MPI library.
        Print::printf(Print::INFO, "Closing MPI library.");
        lm::MPI::finalize();

        Print::printf(Print::INFO, "Program execution finished.");
        PROF_WRITE;
        google::protobuf::ShutdownProtobufLibrary();
        return 0;
    }
    catch (lm::io::hdf5::HDF5Exception & e)
    {
        std::cerr << "HDF5 exception during execution: " << e.what() << std::endl;
    }
    catch (lm::MPIException & e)
    {
        std::cerr << "MPI exception during execution: " << e.what() << std::endl;
    }
    catch (lm::thread::PthreadException & e)
    {
        std::cerr << "PthreadException exception during execution: " << e.what() << std::endl;
    }
    catch (lm::Exception & e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (std::exception & e)
    {
        std::cerr << "Std exception during execution: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown Exception during execution." << std::endl;
    }
    MPI_Abort(MPI_COMM_WORLD,-1);
    lm::MPI::finalize();
    PROF_WRITE;
    google::protobuf::ShutdownProtobufLibrary();
    return -1;
}

void listDevicesMPI()
{
    // Get the MPI hostname.
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int hostnameLength;
    MPI_EXCEPTION_CHECK(MPI_Get_processor_name(hostname, &hostnameLength));

    // Print the capabilities message.
    printf("Process %d running on host %s with %d/%d processor(s)", lm::MPI::worldRank, hostname, numberCpuCores, getPhysicalCpuCores());
    #ifdef OPT_CUDA
    printf(" and %d/%d CUDA device(s)", (int)cudaDevices.size(), lm::CUDA::getNumberDevices());
    #endif
    printf(".\n");

    #ifdef OPT_CUDA
    if (shouldPrintCudaCapabilities)
    {
        for (int i=0; i<(int)cudaDevices.size(); i++)
        {
            printf("  %d-%s\n", lm::MPI::worldRank, lm::CUDA::getCapabilitiesString(cudaDevices[i]).c_str());
        }
    }
    #endif
}

void executeSimulationMPI()
{
    PROF_SET_THREAD(0);
    PROF_BEGIN(PROF_SIM_RUN);

    if (lm::MPI::worldRank == lm::MPI::MASTER)
        executeSimulationMPISingleMaster();
    else
        executeSimulationMPISingleSlave();

    PROF_END(PROF_SIM_RUN);
}

void executeSimulationMPISingleMaster()
{
    Print::printf(Print::DEBUG, "MPI master process %d started.", lm::MPI::worldRank);

    // Create the resource allocator, subtract one core for the data output thread on the master.
    #ifdef OPT_CUDA
    Print::printf(Print::INFO, "MPI process %d using %d core(s) and %d CUDA device(s).", lm::MPI::worldRank, numberCpuCores, (int)cudaDevices.size());
    Print::printf(Print::INFO, "Assigning %0.2f core(s) and %0.2f CUDA device(s) per replicate.", cpuCoresPerReplicate, cudaDevicesPerReplicate);
    ResourceAllocator resourceAllocator(lm::MPI::worldRank, numberCpuCores, cpuCoresPerReplicate, cudaDevices, cudaDevicesPerReplicate);
    #else
    Print::printf(Print::INFO, "MPI process %d using %d core(s).", lm::MPI::worldRank, numberCpuCores);
    Print::printf(Print::INFO, "Assigning %0.2f core(s) per replicate.", cpuCoresPerReplicate);
    ResourceAllocator resourceAllocator(lm::MPI::worldRank, numberCpuCores, cpuCoresPerReplicate);
    #endif

    // Reserve a core for the data output thread, unless we have a flag telling us not to.
    int reservedCpuCore = 0;
    if (shouldReserveOutputCore)
    {
    	reservedCpuCore=resourceAllocator.reserveCpuCore();
    	Print::printf(Print::INFO, "Reserved CPU core %d on process %d for data output.", reservedCpuCore, lm::MPI::worldRank);
    }

    // Create a worker to handle any signals.
    lm::main::SignalHandler * signalHandler = new lm::main::SignalHandler();
    signalHandler->setAffinity(reservedCpuCore);
    signalHandler->start();

    // Create the checkpoint signaler.
    lm::main::CheckpointSignaler * checkpointSignaler = new lm::main::CheckpointSignaler();
    checkpointSignaler->setAffinity(reservedCpuCore);
    checkpointSignaler->start();
    checkpointSignaler->startCheckpointing(checkpointInterval);

    // Open the file.
    lm::io::hdf5::Hdf5File * file = new lm::io::hdf5::Hdf5File(simulationFilename);

    // Start the data output thread.
    lm::main::LocalDataOutputWorker * dataOutputWorker = new lm::main::LocalDataOutputWorker(file);
    dataOutputWorker->setAffinity(reservedCpuCore);
    dataOutputWorker->start();

    // Set the data output handler to be the worker.
    lm::main::DataOutputQueue::setInstance(dataOutputWorker);

    //start the replicate manager thread on the master
    lm::main::ReplicateManager * replicateManager = new lm::main::ReplicateManager(resourceAllocator, solverFactory);
    replicateManager->setAffinity(reservedCpuCore);
    replicateManager->start();

    //start the local replicate worker thread
    lm::main::LocalReplicateSupervisor * localReplicateWorker = new lm::main::LocalReplicateSupervisor(file);
    localReplicateWorker->setAffinity(reservedCpuCore);
    localReplicateWorker->start();

    void * ret;
    PTHREAD_EXCEPTION_CHECK(pthread_join(localReplicateWorker->getId(), &ret));
    Print::printf(Print::INFO, "Master shutting down.");

    // Stop checkpointing.
    checkpointSignaler->stopCheckpointing();

    // Tell all of the slave processes to stop.
    for (int destProc=1; destProc<lm::MPI::worldSize; destProc++)
    {
        //int exitCode=0;
       //if (destProc != lm::MPI::MASTER)
        MPI_EXCEPTION_CHECK(MPI_Send(NULL, 0, MPI_INT, destProc, lm::MPI::MSG_EXIT, MPI_COMM_WORLD));
    }

    // Wait for all of the processes to exit.
    MPI_EXCEPTION_CHECK(MPI_Barrier(MPI_COMM_WORLD));

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
//    MPI_EXCEPTION_CHECK(MPI_Free_mem(staticDataBuffer));
//    delete[] assignedSimulationsTable;
//    delete[] maxSimulationsTable;
    delete checkpointSignaler;
    delete signalHandler;
    delete dataOutputWorker;
//    if (lattice != NULL) delete [] lattice; lattice = NULL;
//    if (latticeSites != NULL) delete [] latticeSites; latticeSites = NULL;

    Print::printf(Print::DEBUG, "MPI master process %d finished.", lm::MPI::worldRank);
}

void executeSimulationMPISingleSlave()
{
    Print::printf(Print::DEBUG, "MPI slave process %d started.", lm::MPI::worldRank);

    // Create the queue to handle data output.
    lm::main::MPIRemoteDataOutputQueue * dataOutputQueue = new lm::main::MPIRemoteDataOutputQueue();
    lm::main::DataOutputQueue::setInstance(dataOutputQueue);

    // Create the resource allocator.
    #ifdef OPT_CUDA
	Print::printf(Print::INFO, "MPI process %d using %d processor(s) and %d CUDA device(s).", lm::MPI::worldRank, numberCpuCores, (int)cudaDevices.size());
    ResourceAllocator resourceAllocator(lm::MPI::worldRank, numberCpuCores, cpuCoresPerReplicate, cudaDevices, cudaDevicesPerReplicate);
    #else
	Print::printf(Print::INFO, "MPI process %d using %d processor(s).", lm::MPI::worldRank, numberCpuCores);
    ResourceAllocator resourceAllocator(lm::MPI::worldRank, numberCpuCores, cpuCoresPerReplicate);
    #endif

    //start the replicate manager thread on the master
    lm::main::ReplicateManager * replicateManager = new lm::main::ReplicateManager(resourceAllocator, solverFactory);
    replicateManager->start();

    MPI_Status messageStatus;
    MPI_EXCEPTION_CHECK(MPI_Recv(NULL, 0, MPI_INT, lm::MPI::MASTER, lm::MPI::MSG_EXIT, MPI_COMM_WORLD, &messageStatus));

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

    // Destroy the data output queue.
    delete dataOutputQueue;
    dataOutputQueue = NULL;

    // Cleanup any resources.
//    MPI_EXCEPTION_CHECK(MPI_Free_mem(staticDataBuffer));
//    if (lattice != NULL) delete [] lattice; lattice = NULL;
//    if (latticeSites != NULL) delete [] latticeSites; latticeSites = NULL;

    Print::printf(Print::DEBUG, "MPI slave process %d finished.", lm::MPI::worldRank);

    // Wait for all of the processes to exit.
    MPI_EXCEPTION_CHECK(MPI_Barrier(MPI_COMM_WORLD));
}
