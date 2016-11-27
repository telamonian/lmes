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
#include "lm/Print.h"
#include "lm/Types.h"
#ifdef OPT_CUDA
#include "lm/Cuda.h"
#endif
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/Tilings.pb.h"
#include "lm/main/CheckpointSignaler.h"
#include "lm/main/Globals.h"
#include "lm/main/MainArgs.h"
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
using lm::message::Communicator;

void listDevices();
void executeSimulationMaster();
void executeSimulationSlave();
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

    // Initialize the profiling library.
    PROF_INIT;

    try
    {
        // Initialize the communicator class and see if we are the master process.
        Print::printf(Print::INFO, "Initializing communications library.");
        bool master = Communicator::initializeDefaultSubclass();

        //Print the startup messages.
        printCopyright(argc, argv);

        // Parse the command line arguments.
        parseArguments(argc, argv);

        if (functionOption == "help")
        {
            printUsage(argc, argv);
        }
        else if (functionOption == "version")
        {
            // Nothing to do.
        }
        else if (functionOption == "devices")
        {
            listDevices();
        }
        else if (functionOption == "simulation")
        {
            if (master)
                executeSimulationMaster();
            else
                executeSimulationSlave();
        }
        else if (functionOption == "debug")
        {
            mainDebug(argc, argv);
        }
        else
        {
            throw lm::CommandLineArgumentException("unknown function.");
        }

        PROF_WRITE;

        // Close the communications library.
        Print::printf(Print::INFO, "Finalizing communications library.");
        Communicator::finalizeDefaultSubclass();

        Print::printf(Print::INFO, "Program execution finished.");
        google::protobuf::ShutdownProtobufLibrary();
        return 0;
    }
    catch (lm::CommandLineArgumentException e)
    {
        std::cerr << "Invalid command line argument: " << e.what() << std::endl << std::endl;
        printUsage(argc, argv);
    }
    catch (lm::thread::PthreadException& e)
    {
        std::cerr << "PthreadException exception during execution: " << e.what() << std::endl;
    }
    catch (lm::Exception& e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (std::exception& e)
    {
        std::cerr << "Std exception during execution: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown Exception during execution." << std::endl;
    }
    PROF_WRITE;
    Communicator::finalizeDefaultSubclass(true);
    google::protobuf::ShutdownProtobufLibrary();
    return -1;
}

void listDevices()
{
    // Create a communicator.
    Communicator* c = Communicator::createObjectOfDefaultSubclass(false);

    // Print the capabilities message.
    printf("Running on host %s with %d processor(s)", c->getHostname().c_str(), (int)lm::main::ResourceController::getPhysicalCPUCores().size());
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

void executeSimulationMaster()
{
    PROF_SET_THREAD(0);
    PROF_BEGIN(PROF_SIM_RUN);

    // Get the hostname.
    char hostnameBuffer[_POSIX_HOST_NAME_MAX+1];
    memset(hostnameBuffer,0,sizeof(hostnameBuffer));
    if (gethostname(hostnameBuffer, sizeof(hostnameBuffer)) != 0)
        throw lm::Exception("unable to get hostname");
    string hostname(hostnameBuffer);
    Print::printf(Print::DEBUG, "Master process started on host %s.", hostname.c_str());

    ResourceMap* resourceMap=NULL;

    // If we ahve a resource file, use it.
    if (resourceFilename != "")
    {
    }

    // Othwerise, if we are running under a PBS queue manager, query it.

    // Otherwise, just use the local hostname.
    else
    {
        list<string> hostnames;
        hostnames.push_back(string(hostname));
        resourceMap = new ResourceMap(hostnames, cpuCores, gpuDevices);
    }

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
    supervisor->setResourceMap(*resourceMap);
    supervisor->init();

    // Start the supervisor.
    supervisor->start();

    // Wait for the resource controller to stop.
    resourceController.wait();

    // Wait for the supervisor to stop.
    supervisor->wait();
    delete supervisor;
    supervisor = NULL;

    Print::printf(Print::DEBUG, "Master process finished on host %s.", hostname.c_str());
    PROF_END(PROF_SIM_RUN);
}

void executeSimulationSlave()
{
    PROF_SET_THREAD(0);
    PROF_BEGIN(PROF_SIM_RUN);

    // Get the hostname.
    char hostnameBuffer[_POSIX_HOST_NAME_MAX+1];
    memset(hostnameBuffer,0,sizeof(hostnameBuffer));
    if (gethostname(hostnameBuffer, sizeof(hostnameBuffer)) != 0)
        throw lm::Exception("unable to get hostname");
    string hostname(hostnameBuffer);
    Print::printf(Print::DEBUG, "Slave process started on host %s.", hostname.c_str());

    // Start the resource controller for this process.
    lm::main::ResourceController resourceController;
    resourceController.start();

    // Wait for the resource controller to stop.
    resourceController.wait();

    Print::printf(Print::DEBUG, "Slave process finished on host %s.", hostname.c_str());
    PROF_END(PROF_SIM_RUN);
}
