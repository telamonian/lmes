/*
 * LocalReplicateWorker.cpp
 *
 *  Created on: Oct 4, 2013
 *      Author: tel
 */

#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.h"
#include "lm/main/LocalReplicateWorker.h"
#include "lm/MPI.h"

namespace lm {
namespace main {

LocalReplicateWorker::LocalReplicateWorker(lm::io::hdf5::SimulationFile * file):
file(file),
shouldCheckpoint(false),
shouldAbort(false)
{}

void LocalReplicateWorker::wake()
{
    MPI_EXCEPTION_CHECK(MPI_Send(NULL, 0, MPI_INT, lm::MPI::worldRank, lm::MPI::MSG_WAKE_REPLICATE_MANAGER, MPI_COMM_WORLD));
}

void LocalReplicateWorker::abort()
{
    if (running)
    {
        aborted = true;
        wake();
    }
}

void LocalReplicateWorker::checkpoint()
{
    bool success=false;

    if (running)
    {
        shouldCheckpoint = true;
        wake();
        success = true;
    }
}

int LocalReplicateWorker::run()
{
    // MPI message variables.
    int messageWaiting;
    MPI_Status messageStatus;
    void * staticDataBuffer = NULL;
    MPI_EXCEPTION_CHECK(MPI_Alloc_mem(lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE, MPI_INFO_NULL, &staticDataBuffer));
    int finishedMessage[2];

    // Create a table for tracking the replicate assignments.
    int* assignedSimulationsTable = new int[lm::MPI::worldSize];
    for (int i=0; i<lm::MPI::worldSize; i++)
    {
        assignedSimulationsTable[i] = 0;
    }

    // Get the maximum number of simulations that can be started on each process.
    int* maxSimulationsTable = new int[lm::MPI::worldSize];
    //int maxSimulations = resourceAllocator.getMaxSimultaneousReplicates();
    MPI_EXCEPTION_CHECK(MPI_Gather(NULL, 1, MPI_INT, maxSimulationsTable, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD)); //may need to change 1st NULL back to &maxSimulations
    int simultaneousReplicates=0;
    for (int i=0; i<lm::MPI::worldSize; i++) simultaneousReplicates += maxSimulationsTable[i];
    Print::printf(Print::INFO, "Number of simultaneous replicates is %d", simultaneousReplicates);
    if (simultaneousReplicates == 0) throw Exception("Invalid configuration, no replicates can be processed.");

    // Create a table for the simulation status.
    // key is replicate number, val is status: 0=waiting to run, 2=finished, other values=(?)(indicate at least not finished)
    map<int,int> simulationStatusTable;
    map<int,struct timespec> simulationStartTimeTable;
    for (vector<int>::iterator it=replicates.begin(); it<replicates.end(); it++)
    {
        simulationStatusTable[*it] = 0;
        simulationStartTimeTable[*it].tv_sec = 0;
        simulationStartTimeTable[*it].tv_nsec = 0;
    }

    // Get the simulation parameters and distribute them to the slaves.
    std::map<std::string,string> simulationParameters = file->getParameters();
    broadcastSimulationParameters(staticDataBuffer, simulationParameters);


    // simulation control loop
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

        // If we need to write a checkpoint, do so. TODO: make checkpointing actually do something for LocalReplicateWorker
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

void broadcastSimulationParameters(void * staticDataBuffer, map<string,string> & simulationParameters)
{
    // Send the simulation parameters to the nodes.
    lm::message::SimulationParameters msg;
    lm::io::SimulationParameters::intoMessage(msg, simulationParameters);
    int msgSize = msg.ByteSize();
    if (msgSize > lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE) throw Exception("Simulation parameter message exceeded buffer size");
    msg.SerializeToArray(staticDataBuffer, msgSize);
    MPI_EXCEPTION_CHECK(MPI_Bcast(&msgSize, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
    MPI_EXCEPTION_CHECK(MPI_Bcast(staticDataBuffer, msgSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
}


}
}
