/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
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
 * Author(s): Elijah Roberts, Max Klein
 */

#if defined(MACOSX)
#include <sys/sysctl.h>
#elif defined(LINUX)
#include <sys/sysinfo.h>
#endif

#include "lm/Exceptions.h"
#ifdef OPT_CUDA
#include "lm/Cuda.h"
#endif
#include "lm/MPI.h"
#include "lm/main/ResourceController.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Communicator.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"

namespace lm {
namespace main {

ResourceController::ResourceController()
    :communicator(lm::MPI::worldRank, threadNumber)
{
}

ResourceController::~ResourceController()
{
}

void ResourceController::wake() throw(PthreadException)
{
//    MPI_EXCEPTION_CHECK(MPI_Send(NULL, 0, MPI_INT, lm::MPI::worldRank, lm::MPI::MSG_WAKE_REPLICATE_DISTRIBUTOR, MPI_COMM_WORLD));
}

/**
 * Gets the number of physical cpu cores on the system.
 */
std::vector<int> ResourceController::getPhysicalCPUCores()
{
    std::vector<int> cpus;

    // Get the number of processors.
    int numberCPUs=0;
    #if defined(MACOSX)
    uint physicalCpuCores;
    size_t  physicalCpuCoresSize=sizeof(physicalCpuCores);
    sysctlbyname("hw.activecpu",&physicalCpuCores,&physicalCpuCoresSize,NULL,0);
    numberCPUs=(int)physicalCpuCores;
    #elif defined(LINUX)
    numberCPUs=get_nprocs();
    #else
    #error "Unsupported architecture."
    #endif

    // Create a pid entry for each cpu.
    for (int i=0; i<numberCPUs; i++)
        cpus.push_back(i);

    return cpus;
}

std::vector<int> ResourceController::getPhysicalGPUs()
{
    std::vector<int> gpus;

    #ifdef OPT_CUDA
    for (int i=0; i<lm::CUDA::getNumberDevices(); i++)
        gpus.push_back(i);
    #endif

    return gpus;
}


int ResourceController::run()
{
    try
    {
        Print::printf(Print::INFO, "Resource controller %d:%d started.", lm::MPI::worldRank, threadNumber);

        // Register our info with the supervisor.
        lm::message::Message msg;
        msg.mutable_resources_available()->set_hostname(communicator.getHostname());
        msg.mutable_resources_available()->set_controller_process(lm::MPI::worldRank);
        msg.mutable_resources_available()->set_controller_thread(threadNumber);
        std::vector<int> cpus=getPhysicalCPUCores();
        for (std::vector<int>::iterator it = cpus.begin() ; it != cpus.end(); ++it)
            msg.mutable_resources_available()->add_cpu(*it);
        std::vector<int> gpus=getPhysicalGPUs();
        for (std::vector<int>::iterator it = gpus.begin() ; it != gpus.end(); ++it)
            msg.mutable_resources_available()->add_gpu(*it);
        communicator.sendMessage(lm::MPI::MASTER, lm::main::SimulationSupervisor::THREAD_ID, &msg);

        // Loop reading messages.
        lm::message::Message message;
        while (true)
        {
            // Read the next message.
            communicator.receiveMessage(&message);

            // Do something with the message.
            if (message.start_work_unit_runner_size() > 0)
            {
                for (int i=0; i<message.start_work_unit_runner_size(); i++)
                    startWorkUnitRunner(message.start_work_unit_runner(i));
            }
            else
            {
                Print::printf(Print::ERROR, "Resource controller received an unknown message: {\n%s}",message.DebugString().c_str());
            }

            // Clear the message object so it can be used again.
            message.Clear();
        }

        /*
        // MPI message variables.
        int messageSize;
        int messageWaiting;
        MPI_Status messageStatus;

        // Report the max simultaneous simulations to the master
        int maxSimulations = resourceAllocator.getMaxSlots();
        MPI_EXCEPTION_CHECK(MPI_Send(&maxSimulations, 1, MPI_INT, lm::MPI::MASTER, lm::MPI::MSG_SIMULTANEOUS_REPLICATES, MPI_COMM_WORLD));

    //    //Read the simulation parameters
    //    lm::io::SimulationParameters parameters;
    //    receiveThing<lm::io::SimulationParameters, lm::MPI::MSG_SIMULATION_PARAMETERS>(staticDataBuffer, &parameters);
    //    map<string,string> simulationParameters(parameters.getParameters());
    //    Print::printf(Print::DEBUG, "Process %d received simulation parameters: %d parameters", lm::MPI::worldRank, simulationParameters.size());
    //
    //    // Read the reaction model.
    //    lm::io::ReactionModel reactionModel;
    //    if (solverFactory.needsReactionModel())
    //    {
    //        receiveThing<lm::io::ReactionModel, lm::MPI::MSG_REACTION_MODEL>(staticDataBuffer, &reactionModel);
    //        Print::printf(Print::DEBUG, "Process %d received reaction model: %d bytes", lm::MPI::worldRank, reactionModel.ByteSize());
    //    }
    //
    //    // Read the diffusion model.
    //    lm::io::DiffusionModel diffusionModel;
    //    uint8_t * lattice=NULL, * latticeSites=NULL;
    //    size_t latticeSize=0, latticeSitesSize=0;
    //    if (solverFactory.needsDiffusionModel())
    //    {
    //        receiveThing<lm::io::DiffusionModel, lm::MPI::MSG_DIFFUSION_MODEL>(staticDataBuffer, &diffusionModel);
    //        Print::printf(Print::DEBUG, "Process %d received diffusion model: %d bytes", lm::MPI::worldRank, diffusionModel.ByteSize());
    //        receiveLatticeModel(&lattice, &latticeSize, &latticeSites, &latticeSitesSize);
    //    }

        // simulation execution loop
        while (true)
        {
            //Print::printf(Print::VERBOSE_DEBUG, "Data output thread looping: %d data sets to write.", dataQueue.size());

            // If we need to abort, do that with the highest priority. TODO: does abort need to do any clean-up here?
            if (aborted)
            {
                break;
            }

            // If we are not running, we are done.
            else if (!running)
            {
                break;
            }

            // If we need to write a checkpoint, do so. TODO: make checkpointing actually do something for Replicate
            else if (shouldCheckpoint)
            {
                shouldCheckpoint = false;
                //doCheckpoint = true;
            }

            // Otherwise, wait for a run simulation message or a wake message
            else
            {
                //this loop keeps the thread in this part of the loop in the case of an irrelevant message
                while (true)
                {
                    //Print::printf(Print::DEBUG, "two.");
                    MPI_EXCEPTION_CHECK(MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &messageStatus));
                    if ((messageStatus.MPI_SOURCE == lm::MPI::MASTER && messageStatus.MPI_TAG == lm::MPI::MSG_RUN_SIMULATION) || (messageStatus.MPI_SOURCE == lm::MPI::worldRank && messageStatus.MPI_TAG == lm::MPI::MSG_WAKE_REPLICATE_DISTRIBUTOR)) break;
                }
                if (messageStatus.MPI_SOURCE == lm::MPI::MASTER && messageStatus.MPI_TAG == lm::MPI::MSG_RUN_SIMULATION)
                {
    //            	PROF_BEGIN(PROF_MASTER_READ_STATIC_MSG);
                    // Read the message into the buffer.
                    MPI_EXCEPTION_CHECK(MPI_Recv(staticDataBuffer, lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE, MPI_BYTE, lm::MPI::MASTER, lm::MPI::MSG_RESULT_UNIT, MPI_COMM_WORLD, &messageStatus));

                    // Get the size of the message.
                    MPI_EXCEPTION_CHECK(MPI_Get_count(&messageStatus, MPI_BYTE, &messageSize));
                    Print::printf(Print::VERBOSE_DEBUG, "Received output data set of size %d from process %d.", messageSize, messageStatus.MPI_SOURCE);
                    //parse the received byte array into a work message
                    work.ParseFromArray(staticDataBuffer, messageSize);
                    distributorSlotAllocator.alloc(work);
    //				PROF_END(PROF_MASTER_READ_STATIC_MSG);update(result);
                }
                else if (messageStatus.MPI_SOURCE == lm::MPI::worldRank && messageStatus.MPI_TAG == lm::MPI::MSG_WAKE_REPLICATE_DISTRIBUTOR)
                {
                    MPI_EXCEPTION_CHECK(MPI_Recv(NULL, 0, MPI_INT, lm::MPI::worldRank, lm::MPI::MSG_WAKE_REPLICATE_DISTRIBUTOR, MPI_COMM_WORLD, &messageStatus));
                }
            }
        }
        */

        Print::printf(Print::INFO, "Resource controller %d:%d finished.", lm::MPI::worldRank, threadNumber);
        return 0;
    }
    catch (lm::Exception e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (std::exception& e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (...)
    {
        Print::printf(Print::FATAL, "Unknown Exception during execution (%s:%d)", __FILE__, __LINE__);
    }
    return -1;
}

void ResourceController::startWorkUnitRunner(const lm::message::StartWorkUnitRunner& properties)
{
    // Start the work unit runner.
    WorkUnitRunner* runner = new WorkUnitRunner(properties);
    runners.push_back(runner);
    runner->start();
}

/*
template <int tag>
void ResourceController::receiveSizeThenBuffer(void * staticDataBuffer, int & msgSize)
{
    MPI_Status msgStat;
    Print::printf(Print::DEBUG, "receiving msg with tag %d.", tag);
    MPI_Status messageStatus;
//    while(true){
//    MPI_EXCEPTION_CHECK(MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &msgStat));
//    Print::printf(Print::DEBUG, "probed message with tag %d", msgStat.MPI_TAG);
//    }
    MPI_EXCEPTION_CHECK(MPI_Recv(&msgSize, 1, MPI_INT, lm::MPI::MASTER, lm::MPI::MSG_MSG_SIZE, MPI_COMM_WORLD, &messageStatus));
    Print::printf(Print::DEBUG, "msg with size tag %d received. it said %d", lm::MPI::MSG_MSG_SIZE, msgSize);
    MPI_EXCEPTION_CHECK(MPI_Recv(staticDataBuffer, msgSize, MPI_BYTE, lm::MPI::MASTER, tag, MPI_COMM_WORLD, &messageStatus));
    Print::printf(Print::DEBUG, "messge with tag %d received.", tag);
}

template <typename t, int tag>
void ResourceController::receiveThing(void * staticDataBuffer, t * thing)
{
    int msgSize;
    receiveSizeThenBuffer<tag>(staticDataBuffer, msgSize);
    thing->ParseFromArray(staticDataBuffer, msgSize);
}

void ResourceController::receiveLatticeModel(uint8_t ** lattice, size_t * latticeSize, uint8_t ** latticeSites, size_t * latticeSitesSize)
{
    MPI_Status messageStatus;
    MPI_EXCEPTION_CHECK(MPI_Recv(latticeSize, 1, MPI_INT, lm::MPI::MASTER, lm::MPI::MSG_MSG_SIZE, MPI_COMM_WORLD, &messageStatus));
    *lattice = new uint8_t[*latticeSize];
    MPI_EXCEPTION_CHECK(MPI_Recv(*lattice, *latticeSize, MPI_BYTE, lm::MPI::MASTER, lm::MPI::MSG_LATTICE, MPI_COMM_WORLD, &messageStatus));
    MPI_EXCEPTION_CHECK(MPI_Recv(latticeSitesSize, 1, MPI_INT, lm::MPI::MASTER, lm::MPI::MSG_MSG_SIZE, MPI_COMM_WORLD, &messageStatus));
    *latticeSites = new uint8_t[*latticeSitesSize];
    MPI_EXCEPTION_CHECK(MPI_Recv(*latticeSites, *latticeSitesSize, MPI_BYTE, lm::MPI::MASTER, lm::MPI::MSG_LATTICE_SITES, MPI_COMM_WORLD, &messageStatus));
//    MPI_EXCEPTION_CHECK(MPI_Bcast(*latticeSites, *latticeSitesSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
    Print::printf(Print::DEBUG, "Process %d received diffusion model lattice: %d and %d bytes", lm::MPI::worldRank, *latticeSize, *latticeSitesSize);
}

//void ResourceController::startReplicate(int replicate, MESolverFactory solverFactory, std::map<std::string,string> & simulationParameters, lm::io::ReactionModel * reactionModel, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize, ResourceAllocator & resourceAllocator) throw(Exception,PthreadException)
//{
//    // Allocate resources for the replicate.
//    ResourceAllocator::ComputeResources resources = resourceAllocator.assignReplicate(replicate);
//    ReplicateRunner * runner = NULL;
//    // Start a new thread for the replicate.
//    Print::printf(Print::DEBUG, "Starting replicate %d in process %d (%s).", replicate, lm::MPI::worldRank, resources.toString().c_str());
//    if (useForwardFluxRunner)
//    {
//        runner = new ForwardFluxRunner(replicate, solverFactory, &simulationParameters, reactionModel, diffusionModel, lattice, latticeSize, latticeSites, latticeSitesSize, resources);
//    }
//    else
//    {
//        runner = new BruteRunner(replicate, solverFactory, &simulationParameters, reactionModel, diffusionModel, lattice, latticeSize, latticeSites, latticeSitesSize, resources);
//    }
//    runner->start();
//    runningReplicates.push_back(runner);
//}
//
//ReplicateRunner * ResourceController::popNextFinishedReplicate(list<ReplicateRunner *> & runningReplicates, ResourceAllocator & resourceAllocator)
//{
//    for (list<ReplicateRunner *>::iterator it=runningReplicates.begin(); it != runningReplicates.end(); it++)
//    {
//        ReplicateRunner * runner = *it;
//        if (runner->hasReplicateFinished())
//        {
//            runningReplicates.erase(it);
//            resourceAllocator.removeReplicate(runner->getReplicate());
//            return runner;
//        }
//    }
//    return NULL;
//}
*/
}
}
