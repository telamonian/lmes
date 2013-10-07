/*
 * ReplicateManager.cpp
 *
 *  Created on: Oct 4, 2013
 *      Author: tel
 */

#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.h"
#include "lm/main/ReplicateManager.h"
#include "lm/MPI.h"

namespace lm {
namespace main {

ReplicateManager::ReplicateManager(ResourceAllocator & resourceAllocator, MESolverFactory & solverFactory):
resourceAllocator(resourceAllocator),
solverFactory(solverFactory),
shouldCheckpoint(false),
shouldAbort(false)
{}

void ReplicateManager::wake()
{
    MPI_EXCEPTION_CHECK(MPI_Send(NULL, 0, MPI_INT, lm::MPI::worldRank, lm::MPI::MSG_WAKE_REPLICATE_MANAGER, MPI_COMM_WORLD));
}

void ReplicateManager::abort()
{
    if (running)
    {
        aborted = true;
        wake();
    }
}

void ReplicateManager::checkpoint()
{
    bool success=false;

    if (running)
    {
        shouldCheckpoint = true;
        wake();
        success = true;
    }
}

int ReplicateManager::run()
{
    // MPI message variables.
    int messageWaiting;
    MPI_Status messageStatus;
    void * staticDataBuffer = NULL;
    MPI_EXCEPTION_CHECK(MPI_Alloc_mem(lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE, MPI_INFO_NULL, &staticDataBuffer));
    int finishedMessage[2];

    // Report the max simultaneous simulations to the master
    int maxSimulations = resourceAllocator.getMaxSimultaneousReplicates();
    MPI_EXCEPTION_CHECK(MPI_Gather(&maxSimulations, 1, MPI_INT, NULL, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));

    //Read the simulation parameters
    map<string,string> simulationParameters = receiveSimulationParameters(staticDataBuffer);

    // Read the reaction model.
    lm::io::ReactionModel reactionModel;
    if (solverFactory.needsReactionModel())
    {
        receiveReactionModel(staticDataBuffer, &reactionModel);
    }

    // Read the diffusion model.
    lm::io::DiffusionModel diffusionModel;
    uint8_t * lattice=NULL, * latticeSites=NULL;
    size_t latticeSize=0, latticeSitesSize=0;
    if (solverFactory.needsDiffusionModel())
    {
        receiveDiffusionModel(staticDataBuffer, &diffusionModel, &lattice, &latticeSize, &latticeSites, &latticeSitesSize);
    }

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

        // If we need to write a checkpoint, do so. TODO: make checkpointing actually do something for ReplicateManager
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
                MPI_EXCEPTION_CHECK(MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &messageStatus));
                if ((messageStatus.MPI_SOURCE == lm::MPI::MASTER && messageStatus.MPI_TAG == lm::MPI::MSG_RUN_SIMULATION) || (messageStatus.MPI_SOURCE == lm::MPI::worldRank && messageStatus.MPI_TAG == lm::MPI::MSG_WAKE_REPLICATE_MANAGER)) break;
            }
            if (messageStatus.MPI_SOURCE == lm::MPI::MASTER && messageStatus.MPI_TAG == lm::MPI::MSG_RUN_SIMULATION)
            {
                int replicate;
                MPI_EXCEPTION_CHECK(MPI_Recv(&replicate, 1, MPI_INT, lm::MPI::MASTER, lm::MPI::MSG_RUN_SIMULATION, MPI_COMM_WORLD, &messageStatus));
                startReplicate(replicate, solverFactory, simulationParameters, &reactionModel, &diffusionModel, lattice, latticeSize, latticeSites, latticeSitesSize, resourceAllocator);
            }
            else if (messageStatus.MPI_SOURCE == lm::MPI::worldRank && messageStatus.MPI_TAG == lm::MPI::MSG_WAKE_REPLICATE_MANAGER)
            {
                MPI_EXCEPTION_CHECK(MPI_Recv(NULL, 0, MPI_INT, lm::MPI::worldRank, lm::MPI::MSG_WAKE_REPLICATE_MANAGER, MPI_COMM_WORLD, &messageStatus));
            }
        }
    }
    MPI_EXCEPTION_CHECK(MPI_Free_mem(staticDataBuffer));
    Print::printf(Print::INFO, "Replicate manager thread finished.");
    return 0;
}

map<string,string> ReplicateManager::receiveSimulationParameters(void * staticDataBuffer)
{
    int msgSize;
    MPI_EXCEPTION_CHECK(MPI_Bcast(&msgSize, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
    MPI_EXCEPTION_CHECK(MPI_Bcast(staticDataBuffer, msgSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
    lm::message::SimulationParameters msg;
    msg.ParseFromArray(staticDataBuffer, msgSize);
    std::map<std::string,string> simulationParameters = lm::io::SimulationParameters::fromMessage(msg);
    Print::printf(Print::DEBUG, "Process %d received simulation parameters: %d parameters", lm::MPI::worldRank, simulationParameters.size());
    return simulationParameters;
}

void ReplicateManager::receiveReactionModel(void * staticDataBuffer, lm::io::ReactionModel * reactionModel)
{
    int msgSize;
    MPI_EXCEPTION_CHECK(MPI_Bcast(&msgSize, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
    MPI_EXCEPTION_CHECK(MPI_Bcast(staticDataBuffer, msgSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
    reactionModel->ParseFromArray(staticDataBuffer, msgSize);
    Print::printf(Print::DEBUG, "Process %d received reaction model: %d bytes", lm::MPI::worldRank, msgSize);
}

void ReplicateManager::receiveDiffusionModel(void * staticDataBuffer, lm::io::DiffusionModel * diffusionModel, uint8_t ** lattice, size_t * latticeSize, uint8_t ** latticeSites, size_t * latticeSitesSize)
{
    int msgSize;
    MPI_EXCEPTION_CHECK(MPI_Bcast(&msgSize, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
    MPI_EXCEPTION_CHECK(MPI_Bcast(staticDataBuffer, msgSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
    diffusionModel->ParseFromArray(staticDataBuffer, msgSize);
    Print::printf(Print::DEBUG, "Process %d received diffusion model: %d bytes", lm::MPI::worldRank, msgSize);

    MPI_EXCEPTION_CHECK(MPI_Bcast(latticeSize, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
    *lattice = new uint8_t[*latticeSize];
    MPI_EXCEPTION_CHECK(MPI_Bcast(*lattice, *latticeSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
    MPI_EXCEPTION_CHECK(MPI_Bcast(latticeSitesSize, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
    *latticeSites = new uint8_t[*latticeSitesSize];
    MPI_EXCEPTION_CHECK(MPI_Bcast(*latticeSites, *latticeSitesSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
    Print::printf(Print::DEBUG, "Process %d received diffusion model lattice: %d and %d bytes", lm::MPI::worldRank, *latticeSize, *latticeSitesSize);
}

ReplicateRunner * ReplicateManager::startReplicate(int replicate, MESolverFactory solverFactory, std::map<std::string,string> & simulationParameters, lm::io::ReactionModel * reactionModel, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize, ResourceAllocator & resourceAllocator) throw(Exception,PthreadException)
{
    // Allocate resources for the replicate.
    ResourceAllocator::ComputeResources resources = resourceAllocator.assignReplicate(replicate);
    ReplicateRunner * runner = NULL;
    // Start a new thread for the replicate.
    Print::printf(Print::DEBUG, "Starting replicate %d in process %d (%s).", replicate, lm::MPI::worldRank, resources.toString().c_str());
    if (useForwardFluxRunner)
    {
        runner = new ForwardFluxRunner(replicate, solverFactory, &simulationParameters, reactionModel, diffusionModel, lattice, latticeSize, latticeSites, latticeSitesSize, resources);
    }
    else
    {
        runner = new BruteRunner(replicate, solverFactory, &simulationParameters, reactionModel, diffusionModel, lattice, latticeSize, latticeSites, latticeSitesSize, resources);
    }
    runner->start();
    return runner;
}

ReplicateRunner * ReplicateManager::popNextFinishedReplicate(list<ReplicateRunner *> & runningReplicates, ResourceAllocator & resourceAllocator)
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


}
}
