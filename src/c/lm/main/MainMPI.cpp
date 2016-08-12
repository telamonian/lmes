/*
  * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
 * 
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
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
#include "lm/input/DiffusionModel.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/OrderParameters.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/Tilings.pb.h"
#include "lm/main/CheckpointSignaler.h"
#include "lm/main/Main.h"
#include "lm/main/ResourceController.h"
#include "lm/main/SignalHandler.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/me/PropensityFunction.h"
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

void ioTest();
void listDevicesMPI();
void executeSimulationMPI();
void executeSimulationMPISingleMaster(ResourceMap* resourceMap);
void executeSimulationMPISingleSlave();

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
                parseArguments(argc, argv, true);

                if (functionOption == "help")
                {
                    // Handle help on the master process.
                    printUsage(argc, argv);
                }
                else if (functionOption == "version")
                {
                    // Handle version on the master process.
                }
                else if (functionOption == "iotest")
                {
                    ioTest();
                }
                else if (functionOption == "devices" || functionOption == "simulation")
                {
                    // Mark that we need to start all of the processes.
                    startAllProcesses = 1;
                }
                else if (functionOption == "debug")
                {
                    mainDebug(argc, argv);
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

        PROF_WRITE;

        // Close the MPI library.
        Print::printf(Print::INFO, "Closing MPI library.");
        lm::MPI::finalize();

        Print::printf(Print::INFO, "Program execution finished.");
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
    PROF_WRITE;
    MPI_Abort(MPI_COMM_WORLD,-1);
    lm::MPI::finalize();
    google::protobuf::ShutdownProtobufLibrary();
    return -1;
}

void ioTest()
{
    lm::input::DiffusionModel diffusionModel;
    lm::input::OrderParameters orderParameters;
    lm::input::ReactionModel reactionModel;
    lm::input::Tilings tilings;

    // Open the simulation file.
    lm::io::hdf5::Hdf5File * file = new lm::io::hdf5::Hdf5File(simulationInputFilename);

    // Read in, and then write out, any extant sections of the simulation file
    if (file->hasDiffusionModel())
    {
        file->getDiffusionModel(&diffusionModel);
        file->setDiffusionModel(&diffusionModel);
    }
    if (file->hasOrderParameters())
    {
        file->getOrderParameters(&orderParameters);
        file->setOrderParameters(&orderParameters);
    }
    if (file->hasReactionModel())
    {
        file->getReactionModel(&reactionModel);
        file->setReactionModel(&reactionModel);
    }
    if (file->hasTilings())
    {
        file->getTilings(&tilings);
        file->setTilings(&tilings);
    }
    file->close();
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

    // Print a list of the registered propensity fucntinos.
    lm::me::PropensityFunctionFactory fs;
    fs.printRegisteredFunctions();

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
    supervisor->init();

    // Start the supervisor.
    supervisor->start();

    // Wait for the resource controller to stop.
    resourceController.wait();

    // Wait for the supervisor to stop.
    supervisor->wait();
    delete supervisor; supervisor = NULL;

    // Wait for all of the processes to exit.
    MPI_EXCEPTION_CHECK(MPI_Barrier(MPI_COMM_WORLD));

    Print::printf(Print::DEBUG, "MPI master process %d finished.", lm::MPI::worldRank);
}

void executeSimulationMPISingleSlave()
{
    Print::printf(Print::DEBUG, "MPI slave process %d started.", lm::MPI::worldRank);

    // Start the resource controller for this process.
    lm::main::ResourceController resourceController;
    resourceController.start();

    // Wait for the resource controller to stop.
    resourceController.wait();

    // Wait for all of the processes to exit.
    MPI_EXCEPTION_CHECK(MPI_Barrier(MPI_COMM_WORLD));

    Print::printf(Print::DEBUG, "MPI slave process %d finished.", lm::MPI::worldRank);
}
