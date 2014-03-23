/*
 * ReplicateManager.cpp
 *
 *  Created on: Oct 4, 2013
 *      Author: tel
 */
#include <pthread.h>
#include "lm/io/hdf5/SimulationFile.h"
#include "DiffusionModel.pb.h"
#include "ReactionModel.pb.h"
#include "lm/io/SimulationParameters.h"
#include "lm/main/BruteRunner.h"
#include "lm/main/ForwardFluxRunner.h"
#include "lm/main/ReplicateDistributor.h"
#include "SimulationParameters.pb.h"
#include "lm/MPI.h"

namespace lm {
namespace main {

ReplicateDistributor::ReplicateDistributor(ResourceAllocator & resourceAllocator, MESolverFactory & solverFactory) throw(PthreadException):
resourceAllocator(resourceAllocator),
solverFactory(solverFactory),
distributorSlotAllocator(resourceAllocator),
staticDataBuffer(NULL),
shouldCheckpoint(false),
shouldAbort(false)
{
	MPI_EXCEPTION_CHECK(MPI_Alloc_mem(lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE, MPI_INFO_NULL, &staticDataBuffer));
}

ReplicateDistributor::~ReplicateDistributor() throw(PthreadException)
{
}

void ReplicateDistributor::wake() throw(PthreadException)
{
    MPI_EXCEPTION_CHECK(MPI_Send(NULL, 0, MPI_INT, lm::MPI::worldRank, lm::MPI::MSG_WAKE_REPLICATE_MANAGER, MPI_COMM_WORLD));
}

void ReplicateDistributor::abort() throw(PthreadException)
{
    if (running)
    {
        aborted = true;
        wake();
    }
}

void ReplicateDistributor::checkpoint() throw(PthreadException)
{
    bool success=false;

    if (running)
    {
        shouldCheckpoint = true;
        wake();
        success = true;
    }
}

int ReplicateDistributor::run()
{
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
				distributorSlotAllocator.update(work);
//				PROF_END(PROF_MASTER_READ_STATIC_MSG);update(result);
            }
            else if (messageStatus.MPI_SOURCE == lm::MPI::worldRank && messageStatus.MPI_TAG == lm::MPI::MSG_WAKE_REPLICATE_MANAGER)
            {
                MPI_EXCEPTION_CHECK(MPI_Recv(NULL, 0, MPI_INT, lm::MPI::worldRank, lm::MPI::MSG_WAKE_REPLICATE_MANAGER, MPI_COMM_WORLD, &messageStatus));
            }
        }
    }
    Print::printf(Print::INFO, "Replicate manager thread finished.");
    return 0;
}

void ReplicateDistributor::distributeWorkUnit(lm::work::Work & work)
{

}

template <int tag>
void ReplicateDistributor::receiveSizeThenBuffer(void * staticDataBuffer, int & msgSize)
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
void ReplicateDistributor::receiveThing(void * staticDataBuffer, t * thing)
{
    int msgSize;
    receiveSizeThenBuffer<tag>(staticDataBuffer, msgSize);
    thing->ParseFromArray(staticDataBuffer, msgSize);
}

void ReplicateDistributor::receiveLatticeModel(uint8_t ** lattice, size_t * latticeSize, uint8_t ** latticeSites, size_t * latticeSitesSize)
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

void ReplicateDistributor::startReplicate(int replicate, MESolverFactory solverFactory, std::map<std::string,string> & simulationParameters, lm::io::ReactionModel * reactionModel, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize, ResourceAllocator & resourceAllocator) throw(Exception,PthreadException)
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
    runningReplicates.push_back(runner);
}

ReplicateRunner * ReplicateDistributor::popNextFinishedReplicate(list<ReplicateRunner *> & runningReplicates, ResourceAllocator & resourceAllocator)
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
