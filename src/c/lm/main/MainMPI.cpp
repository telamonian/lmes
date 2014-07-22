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

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <map>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <google/protobuf/stubs/common.h>
#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/Math.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/Types.h"
#ifdef OPT_CUDA
#include "lm/Cuda.h"
#endif
#include "lm/main/CheckpointSignaler.h"
#include "lm/main/Main.h"
#include "lm/main/ResourceController.h"
#include "lm/main/SignalHandler.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/resource/ResourceMap.h"
#include "lm/thread/Thread.h"
#include "lm/thread/WorkerManager.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"
#include "hrtime.h"

using std::map;
using std::list;
using lm::Print;
using lm::Exception;
using lm::resource::ResourceMap;
using lm::thread::PthreadException;

void listDevicesMPI();
void executeSimulationMPI();
void executeSimulationMPISingleMaster(ResourceMap* resourceMap);
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
	// Start the global execution timer
	globalTimer = getHrTime();

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
    printf("Process %d running on host %s with %d processor(s)", lm::MPI::worldRank, hostname, (int)lm::main::ResourceController::getPhysicalCPUCores().size());
    #ifdef OPT_CUDA
    printf(" and %d CUDA device(s)", (int)lm::main::ResourceController::getPhysicalGPUs().size());
    #endif
    printf(".\n");

    #ifdef OPT_CUDA
    if (shouldPrintGPUCapabilities)
    {
        for (int i=0; i<lm::CUDA::getNumberDevices(); i++)
        {
            printf("  %d-%s\n", lm::MPI::worldRank, lm::CUDA::getCapabilitiesString(i).c_str());
        }
    }
    #endif
}

void executeSimulationMPI()
{
    PROF_SET_THREAD(0);
    PROF_BEGIN(PROF_SIM_RUN);

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

        // Create the resource map.
        ResourceMap resourceMap(hostnames, cpuCores, gpuDevices, resourceFilename);
        executeSimulationMPISingleMaster(&resourceMap);
    }
    else
    {
        // Send all of the host names.
        MPI_EXCEPTION_CHECK(MPI_Gather(hostname,sizeof(hostname),MPI_CHAR,NULL,sizeof(hostname),MPI_CHAR,lm::MPI::MASTER,MPI_COMM_WORLD));
        executeSimulationMPISingleSlave();
    }

    PROF_END(PROF_SIM_RUN);
}

void executeSimulationMPISingleMaster(ResourceMap* resourceMap)
{
    Print::printf(Print::DEBUG, "MPI master process %d started.", lm::MPI::worldRank);

    // Print a list of the registered classes.
    lm::ClassFactory::getInstance().printRegisteredClasses();

    // Start the resource controller for this process.
    lm::main::ResourceController resourceController;
    resourceController.start();

    // Create the supervisor.
    lm::main::SimulationSupervisor* supervisor = static_cast<lm::main::SimulationSupervisor*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::main::SimulationSupervisor",supervisorClassName));
    supervisor->setUseCPUAffinity(useCPUAffinity);
    supervisor->setSimulationFilename(simulationInputFilename, simulationOutputFilename);
    supervisor->setOutputWriterClassName(outputWriterClassName);
    supervisor->setSolverClassName(solverClassName);
    supervisor->setResourceMap(resourceMap);
    supervisor->initialize();

    // Start the supervisor.
    supervisor->start();

    /*
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

    // Get the maximum number of simulations that can be started on each process.
    int maxSlots = 0;	//need maxSlots here due to the fact that MPI_Gather complains if send_buf is NULL even on the root process
    int * maxSlotsTable = new int[lm::MPI::worldSize];
    MPI_EXCEPTION_CHECK(MPI_Gather(&maxSlots, 1, MPI_INT, maxSlotsTable, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));

    //calculate the total number of slots that can be simultaneously used for running work units across the comm
    int maxSlotsTotal=0;
    for (int i=0; i<lm::MPI::worldSize; ++i) maxSlotsTotal += maxSlotsTable[i];
    Print::printf(Print::INFO, "Number of work unit slots is %d", maxSlotsTotal);
    if (maxSlotsTotal == 0) throw Exception("Invalid configuration, no work units can be processed.");

    // Create a worker to handle any signals.
    lm::main::SignalHandler * signalHandler = new lm::main::SignalHandler();
    signalHandler->setAffinity(reservedCpuCore);
    signalHandler->start();

    // Create the checkpoint signaler.
    lm::main::CheckpointSignaler * checkpointSignaler = new lm::main::CheckpointSignaler();
    checkpointSignaler->setAffinity(reservedCpuCore);
    checkpointSignaler->start();
    checkpointSignaler->startCheckpointing(checkpointInterval);

    */

    /*
    // Start the data output thread.
    lm::main::LocalDataOutputWorker * dataOutputWorker = new lm::main::LocalDataOutputWorker(file);
    dataOutputWorker->setAffinity(reservedCpuCore);
    dataOutputWorker->start();

    // Set the data output handler to be the worker.
    lm::main::DataOutputQueue::setInstance(dataOutputWorker);

    //start the replicate distributor thread on the master
//    lm::main::ReplicateDistributor * replicateDistributor = new lm::main::ReplicateDistributor(resourceAllocator, solverFactory);
//    replicateDistributor->setAffinity(reservedCpuCore);
//    replicateDistributor->start();

    //start the replicate supervisor thread
    lm::main::ReplicateSupervisor * replicateSupervisor = new lm::main::ReplicateSupervisor(maxSlotsTable, file);
    replicateSupervisor->setAffinity(reservedCpuCore);
    replicateSupervisor->start();
*/

    // Wait for the resource controller to stop.
    resourceController.wait();

    // Stop the supervisor.
    supervisor->stop();
    delete supervisor;
    supervisor = NULL;

    /*
    // Stop checkpointing.
    checkpointSignaler->stopCheckpointing();

    // Tell all of the slave processes to stop.
    int exitCode = globalAbort ? 1 : 0;
    for (int destProc=1; destProc<lm::MPI::worldSize; destProc++)
    {
        //if (destProc != lm::MPI::MASTER)
        MPI_EXCEPTION_CHECK(MPI_Send(&exitCode, 1, MPI_INT, destProc, lm::MPI::MSG_EXIT, MPI_COMM_WORLD));
    }

*/

    // Wait for all of the processes to exit.
    MPI_EXCEPTION_CHECK(MPI_Barrier(MPI_COMM_WORLD));

    // If this was a global abort, stop the workers quickly.
    /*if (globalAbort)
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
    delete[] maxSlotsTable;
//    delete checkpointSignaler;
//    delete signalHandler;
//    delete dataOutputWorker;
//    if (lattice != NULL) delete [] lattice; lattice = NULL;
//    if (latticeSites != NULL) delete [] latticeSites; latticeSites = NULL;

*/

    Print::printf(Print::DEBUG, "MPI master process %d finished.", lm::MPI::worldRank);
}

void executeSimulationMPISingleSlave()
{
    Print::printf(Print::DEBUG, "MPI slave process %d started.", lm::MPI::worldRank);

    // Start the resource controller for this process.
    lm::main::ResourceController resourceController;
    resourceController.start();

    /*
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

    // Report the max simultaneous simulations to the master
    int maxSlots = resourceAllocator.getMaxSlots();
    MPI_EXCEPTION_CHECK(MPI_Gather(&maxSlots, 1, MPI_INT, NULL, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
    //start the replicate distributor thread on the slave
    lm::main::ReplicateDistributor * replicateDistributor = new lm::main::ReplicateDistributor(resourceAllocator);
    replicateDistributor->start();

    // join the replicate distributor thread. the termination of this thread should be promptly followed by the termination of this process
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
*/

    // Wait for the resource controller to stop.
    resourceController.wait();

    // Wait for all of the processes to exit.
    MPI_EXCEPTION_CHECK(MPI_Barrier(MPI_COMM_WORLD));

    Print::printf(Print::DEBUG, "MPI slave process %d finished.", lm::MPI::worldRank);
}
