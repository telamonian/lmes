/*
 * LocalReplicateWorker.cpp
 *
 *  Created on: Oct 4, 2013
 *      Author: tel
 */

#include <ctime>
#include <mpi.h>
#if defined(MACOSX)
#include <sys/time.h>
#endif
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.h"
#include "lm/main/Main.h"
#include "lm/main/LocalReplicateWorker.h"
#include "lm/message/SimulationParameters.pb.h"
#include "lm/MPI.h"
#include "lm/Print.h"

namespace lm {
namespace main {

LocalReplicateWorker::LocalReplicateWorker(lm::io::hdf5::SimulationFile * file) throw(PthreadException):
file(file),
shouldCheckpoint(false),
shouldAbort(false)
{}

LocalReplicateWorker::~LocalReplicateWorker() throw(PthreadException)
{
}

void LocalReplicateWorker::wake() throw(PthreadException)
{
    MPI_EXCEPTION_CHECK(MPI_Send(NULL, 0, MPI_INT, lm::MPI::worldRank, lm::MPI::MSG_WAKE_LOCAL_REPLICATE_WORKER, MPI_COMM_WORLD));
}

void LocalReplicateWorker::abort() throw(PthreadException)
{
    if (running)
    {
        aborted = true;
        wake();
    }
}

void LocalReplicateWorker::checkpoint() throw(PthreadException)
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
//    int maxSimulations = resourceAllocator.getMaxSimultaneousReplicates();
    Print::printf(Print::DEBUG, "before 1st worker gather.");
    MPI_MastBcastIn<int>(maxSimulationsTable, 1, MPI_INT, lm::MPI::MSG_SIMULTANEOUS_REPLICATES, MPI_COMM_WORLD); //may need to change 1st NULL back to &maxSimulations
    Print::printf(Print::DEBUG, "after 1st worker gather.");
    int simultaneousReplicates=0;
    for (int i=0; i<lm::MPI::worldSize; i++) simultaneousReplicates += maxSimulationsTable[i];
    Print::printf(Print::INFO, "Number of simultaneous replicates is %d", simultaneousReplicates);
    if (simultaneousReplicates == 0) throw Exception("Invalid configuration, no replicates can be processed.");

    // Initialize simulation status and simulation timing table.
    for (vector<int>::iterator it=replicates.begin(); it<replicates.end(); it++)
    {
        Print::printf(Print::DEBUG, "replicate %d seen.", *it);
        simulationStatusTable[*it] = 0;
        simulationStartTimeTable[*it].tv_sec = 0;
        simulationStartTimeTable[*it].tv_nsec = 0;
    }

    // Get the simulation parameters and distribute them to the slaves.
    //std::map<std::string,string> simulationParameters = file->getParameters();
    lm::io::SimulationParameters simulationParameters(file->getParameters());
    bcastThing<lm::io::SimulationParameters, lm::MPI::MSG_SIMULATION_PARAMETERS>(staticDataBuffer, &simulationParameters);

    Print::printf(Print::DEBUG, "before reaction model.");

    // Get the reaction model and distribute it to the slaves.
    lm::io::ReactionModel reactionModel;

    if (solverFactory.needsReactionModel())
    {
        file->getReactionModel(&reactionModel);
        bcastThing<lm::io::ReactionModel, lm::MPI::MSG_REACTION_MODEL>(staticDataBuffer, &reactionModel);
    }

    Print::printf(Print::DEBUG, "after reaction model.");

    // Get the diffusion model and distribute it to the slaves.
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
        bcastThing<lm::io::DiffusionModel, lm::MPI::MSG_DIFFUSION_MODEL>(staticDataBuffer, &diffusionModel);
        bcastSizeThenBuffer<lm::MPI::MSG_LATTICE>(lattice, latticeSize);
        bcastSizeThenBuffer<lm::MPI::MSG_LATTICE_SITES>(latticeSites, latticeSitesSize);
        //broadcastDiffusionModel(staticDataBuffer, &diffusionModel, lattice, latticeSize, latticeSites, latticeSitesSize);
    }

    Print::printf(Print::DEBUG, "setup done.");
    //initialize replicate runners on all nodes via messages
    int replicate;
    for (int i=0; i<lm::MPI::worldSize; ++i)
    {
        for (int j=0; j<maxSimulationsTable[i]; ++j)
        {
            Print::printf(Print::DEBUG, "finding space for replicate %d out of node max %d", j, maxSimulationsTable[i]);
            // find a replicate to run and send the message to a replicate manager to run it
            replicate = FindRunRep(i);
            simulationStatusTable[replicate] = 1;
            // If all of the simulations have been assigned, stop
            if (replicate==-1)
            {
                j=maxSimulationsTable[i]; i=lm::MPI::worldSize;
            }
        }
    }

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

        // Otherwise, wait for a simulation finished message or a wake message
        else
        {
            //this loop keeps the thread in this part of the loop in the case of an irrelevant message
            while (true)
            {
                MPI_EXCEPTION_CHECK(MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &messageStatus));
                if ((messageStatus.MPI_TAG == lm::MPI::MSG_SIMULATION_FINISHED) || (messageStatus.MPI_SOURCE == lm::MPI::MASTER && messageStatus.MPI_TAG == lm::MPI::MSG_WAKE_LOCAL_REPLICATE_WORKER)) break;
            }
            if (messageStatus.MPI_TAG == lm::MPI::MSG_SIMULATION_FINISHED)
            {
                struct timespec now;
                #if defined(LINUX)
                clock_gettime(CLOCK_REALTIME, &now);
                #elif defined(MACOSX)
                struct timeval now2;
                gettimeofday(&now2, NULL);
                now.tv_sec = now2.tv_sec;
                now.tv_nsec = now2.tv_usec*1000;
                #endif

                MPI_EXCEPTION_CHECK(MPI_Recv(&finishedMessage, 2, MPI_INT, MPI_ANY_SOURCE, lm::MPI::MSG_SIMULATION_FINISHED, MPI_COMM_WORLD, &messageStatus));
                Print::printf(Print::INFO, "Replicate %d completed by process %d with exit code %d in %0.2f seconds.", finishedMessage[0], messageStatus.MPI_SOURCE, finishedMessage[1], ((double)(now.tv_sec-simulationStartTimeTable[finishedMessage[0]].tv_sec))+1e-9*((double)now.tv_nsec-simulationStartTimeTable[finishedMessage[0]].tv_nsec));
                simulationStatusTable[finishedMessage[0]] = 2;
                replicate = FindRunRep(messageStatus.MPI_SOURCE);
                if (replicate==-2) break;
            }
            else if (messageStatus.MPI_SOURCE == lm::MPI::MASTER && messageStatus.MPI_TAG == lm::MPI::MSG_WAKE_LOCAL_REPLICATE_WORKER)
            {
                MPI_EXCEPTION_CHECK(MPI_Recv(NULL, 0, MPI_INT, lm::MPI::MASTER, lm::MPI::MSG_WAKE_LOCAL_REPLICATE_WORKER, MPI_COMM_WORLD, &messageStatus));
            }
        }
    }
    MPI_EXCEPTION_CHECK(MPI_Free_mem(staticDataBuffer));
    delete[] assignedSimulationsTable;
    delete[] maxSimulationsTable;
    if (lattice != NULL) delete [] lattice; lattice = NULL;
    if (latticeSites != NULL) delete [] latticeSites; latticeSites = NULL;
    running = false;
    Print::printf(Print::INFO, "Local replicate worker thread finished.");
    return 0;
}

//void LocalReplicateWorker::broadcastSimulationParameters(void * staticDataBuffer, map<string,string> & simulationParameters)
//{
//    // Send the simulation parameters to the nodes.
//    lm::message::SimulationParameters msg;
//    lm::io::SimulationParameters::intoMessage(msg, simulationParameters);
//    int msgSize = msg.ByteSize();
//    if (msgSize > lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE) throw Exception("Simulation parameter message exceeded buffer size");
//    msg.SerializeToArray(staticDataBuffer, msgSize);
//    MPI_MastBcastOut(&msgSize, 1, MPI_INT, lm::MPI::MASTER, lm::MPI::MSG_MSG_SIZE, MPI_COMM_WORLD);
//    MPI_MastBcastOut(staticDataBuffer, msgSize, MPI_BYTE, lm::MPI::MASTER, lm::MPI::MSG_REACTION_MODEL, MPI_COMM_WORLD);
//}
//
//void LocalReplicateWorker::broadcastReactionModel(void * staticDataBuffer, lm::io::ReactionModel * reactionModel)
//{
//    // Send the reaction model to the nodes.
//    int msgSize = reactionModel->ByteSize();
//    if (msgSize > lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE) throw Exception("Reaction model message exceeded buffer size");
//    reactionModel->SerializeToArray(staticDataBuffer, msgSize);
//    MPI_EXCEPTION_CHECK(MPI_Bcast(&msgSize, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
//    MPI_EXCEPTION_CHECK(MPI_Bcast(staticDataBuffer, msgSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
//}
//
//void LocalReplicateWorker::broadcastDiffusionModel(void * staticDataBuffer, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize)
//{
//    // Send the diffusion model to the nodes.
//    int msgSize = diffusionModel->ByteSize();
//    if (msgSize > lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE) throw Exception("Diffusion model message exceeded buffer size");
//    diffusionModel->SerializeToArray(staticDataBuffer, msgSize);
//    MPI_EXCEPTION_CHECK(MPI_Bcast(&msgSize, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
//    MPI_EXCEPTION_CHECK(MPI_Bcast(staticDataBuffer, msgSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
//    MPI_EXCEPTION_CHECK(MPI_Bcast(&latticeSize, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
//    MPI_EXCEPTION_CHECK(MPI_Bcast(lattice, latticeSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
//    MPI_EXCEPTION_CHECK(MPI_Bcast(&latticeSitesSize, 1, MPI_INT, lm::MPI::MASTER, MPI_COMM_WORLD));
//    MPI_EXCEPTION_CHECK(MPI_Bcast(latticeSites, latticeSitesSize, MPI_BYTE, lm::MPI::MASTER, MPI_COMM_WORLD));
//}

//send message, one by one, to all nodes including master. nodes should use MPI_Recv plus the relevant tag to receive
void LocalReplicateWorker::MPI_MastBcastOut(void * buf, int count, MPI_Datatype datatype, int tag, MPI_Comm comm)
{
    Print::printf(Print::DEBUG, "in mastbcastout, lm::MPI::worldSize is %d and lm::MPI::MASTER is %d.", lm::MPI::worldSize, lm::MPI::MASTER);
    for(int destProc=0; destProc < lm::MPI::worldSize; ++destProc)
    {
        MPI_EXCEPTION_CHECK(MPI_Send(buf, count, datatype, destProc, tag, comm));
    }
}

int LocalReplicateWorker::FindRunRep(int destProc)
{
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
    // if all replicates have finished, signal we are finished.
    if (allFinished) return -2;
    // If we are out of new simulations to run, signal we are done.
    if (replicate==-1) return replicate;



    //send the message to start replicate to node with rank of destProc
    MPI_EXCEPTION_CHECK(MPI_Send(&replicate, 1, MPI_INT, destProc, lm::MPI::MSG_RUN_SIMULATION, MPI_COMM_WORLD));
    Print::printf(Print::DEBUG, "signal to start replicate %d sent", replicate);
    return replicate;
}

}
}
