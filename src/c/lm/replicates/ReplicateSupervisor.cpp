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

#include <ctime>
#include <deque>
#include <mpi.h>
#include <vector>
#if defined(MACOSX)
#include <sys/time.h>
#endif
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/ClassFactory.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/replicates/ReplicateSupervisor.h"
#include "lm/resource/SupervisorSlotAllocator.h"

namespace lm {
namespace replicates {

bool ReplicateSupervisor::registered=ReplicateSupervisor::registerClass();

bool ReplicateSupervisor::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::main::SimulationSupervisor","lm::replicates::ReplicateSupervisor",&ReplicateSupervisor::allocateObject);
    return true;
}

void* ReplicateSupervisor::allocateObject()
{
    return new ReplicateSupervisor();
}

using lm::resource::SupervisorSlotAllocator;
using std::deque;
using std::vector;

ReplicateSupervisor::ReplicateSupervisor()
{

}

/*ReplicateSupervisor::ReplicateSupervisor(int * maxSlotsTable, lm::io::hdf5::Hdf5File * file) throw(PthreadException):
slotAllocator(maxSlotsTable),
trajectoryAllocator(file, true, true),
file(file),
staticDataBuffer(NULL),
shouldCheckpoint(false),
shouldAbort(false)
{
MPI_EXCEPTION_CHECK(MPI_Alloc_mem(lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE, MPI_INFO_NULL, &staticDataBuffer));
int maxSlotsTotal=0;
for (int i=0; i<lm::MPI::worldSize; ++i) maxSlotsTotal += maxSlotsTable[i];
trajectoryAllocator.initTrajectories(maxSlotsTotal);
}
*/
ReplicateSupervisor::~ReplicateSupervisor()
{
}

/*
void ReplicateSupervisor::wake() throw(PthreadException)
{
    MPI_EXCEPTION_CHECK(MPI_Send(NULL, 0, MPI_INT, lm::MPI::worldRank, lm::MPI::MSG_WAKE_REPLICATE_SUPERVISOR, MPI_COMM_WORLD));
}

void ReplicateSupervisor::abort() throw(PthreadException)
{
    if (running)
    {
        aborted = true;
        wake();
    }
}

void ReplicateSupervisor::checkpoint() throw(PthreadException)
{
    bool success=false;

    if (running)
    {
        shouldCheckpoint = true;
        wake();
        success = true;
    }
}
*/
void ReplicateSupervisor::startSimulation()
{
    Print::printf(Print::INFO, "Replicate supervisor starting simulation.");
    /*
    // MPI message variables.
	int messageSize;
    int messageWaiting;
    MPI_Status messageStatus;
    lm::work::Result result;

    // distribute first round of work units to slave distributors. In theory, # work units = # trajectories = # slots
    distributeWorkUnits();

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
                if ((messageStatus.MPI_TAG == lm::MPI::MSG_RESULT_UNIT) || (messageStatus.MPI_SOURCE == lm::MPI::MASTER && messageStatus.MPI_TAG == lm::MPI::MSG_WAKE_REPLICATE_SUPERVISOR)) break;
            }
            if (messageStatus.MPI_TAG == lm::MPI::MSG_RESULT_UNIT)
            {
//                struct timespec now;
//                #if defined(LINUX)
//                clock_gettime(CLOCK_REALTIME, &now);
//                #elif defined(MACOSX)
//                struct timeval now2;
//                gettimeofday(&now2, NULL);
//                now.tv_sec = now2.tv_sec;
//                now.tv_nsec = now2.tv_usec*1000;
//                #endif

//                Print::printf(Print::INFO, "Replicate %d completed by process %d with exit code %d in %0.2f seconds.", finishedMessage[0], messageStatus.MPI_SOURCE, finishedMessage[1], ((double)(now.tv_sec-simulationStartTimeTable[finishedMessage[0]].tv_sec))+1e-9*((double)now.tv_nsec-simulationStartTimeTable[finishedMessage[0]].tv_nsec));

//            	PROF_BEGIN(PROF_MASTER_READ_STATIC_MSG);
				// Read the message into the buffer.
				MPI_EXCEPTION_CHECK(MPI_Recv(staticDataBuffer, lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE, MPI_BYTE, messageStatus.MPI_SOURCE, lm::MPI::MSG_RESULT_UNIT, MPI_COMM_WORLD, &messageStatus));

				// Get the size of the message.
				MPI_EXCEPTION_CHECK(MPI_Get_count(&messageStatus, MPI_BYTE, &messageSize));
				Print::printf(Print::VERBOSE_DEBUG, "Received output data set of size %d from process %d.", messageSize, messageStatus.MPI_SOURCE);
				//parse the received byte array into a result message
				result.ParseFromArray(staticDataBuffer, messageSize);
//				PROF_END(PROF_MASTER_READ_STATIC_MSG);

//				lm::main::DataOutputQueue::getInstance()->writeResult(result);	//TODO: make this work. the eventual writeResult signature should be written sans reference, so as to eliminate races with this current loop over rewriting result
//            	update(result);
//            	distributeWorkUnits();
            }
            else if (messageStatus.MPI_SOURCE == lm::MPI::MASTER && messageStatus.MPI_TAG == lm::MPI::MSG_WAKE_REPLICATE_SUPERVISOR)
            {
                MPI_EXCEPTION_CHECK(MPI_Recv(NULL, 0, MPI_INT, lm::MPI::MASTER, lm::MPI::MSG_WAKE_REPLICATE_SUPERVISOR, MPI_COMM_WORLD, &messageStatus));
            }
        }
    }
//    if (lattice != NULL) delete [] lattice; lattice = NULL;
//    if (latticeSites != NULL) delete [] latticeSites; latticeSites = NULL;
    running = false;
    */
}

//update the state of the slotAllocator and the trajectoryAllocator based on a result that has just been received
/*
 *void ReplicateSupervisor::update(lm::work::Result & result)
{
	slotAllocator.update(result);
	trajectoryAllocator.update(result);
}

//does the following:
//marks slot as BUSY
//sets slot related properties (pid, sid) of trajectory
//sends work unit to appropriate process using an MPI message
void ReplicateSupervisor::distributeWorkUnit(deque<Slot *>::iterator slot_it, map<int, TrajectoryAllocator::Trajectory>::iterator traj_it)
{
	vector<int> slotIds(2);
	slotIds.push_back((*slot_it)->pid);
	slotIds.push_back((*slot_it)->sid);
	(*slot_it)->alloc(traj_it->second.getWork(slotIds));	//slot_it->second.alloc() allocates the slot the iterator points to and return a vector of [pid, sid]
}

//function that can be run at any time to distribute unfinished trajectories to free slots
void ReplicateSupervisor::distributeWorkUnits()
{
	//deque<map<vector<int>, SupervisorSlot>::iterator> slots(slotAllocator.freeSlots);
	//deque<SupervisorSlot *>::iterator> slots(slotAllocator.freeSlots);
	bool modifiedTrajectories = false;
	do
	{
		map<int, TrajectoryAllocator::Trajectory>::iterator traj_it(trajectoryAllocator.getBegin());
		map<int, TrajectoryAllocator::Trajectory>::iterator end(trajectoryAllocator.getEnd());
		while (traj_it!=end)
		{
			modifiedTrajectories = false;
			if (slotAllocator.freeSlots.size()==0)
			{
				return;
			}
			if (traj_it->second.status==TrajectoryAllocator::CONTINUE && traj_it->second.getPid()==-1 && traj_it->second.getSid()==-1)
			{
				distributeWorkUnit(slotAllocator.freeSlots.end(), traj_it);
				slotAllocator.freeSlots.pop_back();
				++traj_it;
			}
			else if (traj_it->second.status==TrajectoryAllocator::FINISHED)
			{
				trajectoryAllocator.eraseTrajectory(traj_it++);
				trajectoryAllocator.initTrajectory();
				modifiedTrajectories=true;
			}
		}
	} while (modifiedTrajectories==true);
}
*/

////send message, one by one, to all nodes including master. nodes should use MPI_Recv plus the relevant tag to receive
//void ReplicateSupervisor::MPI_MastBcastOut(void * buf, int count, MPI_Datatype datatype, int tag, MPI_Comm comm)
//{
//    Print::printf(Print::DEBUG, "in mastbcastout, lm::MPI::worldSize is %d and lm::MPI::MASTER is %d.", lm::MPI::worldSize, lm::MPI::MASTER);
//    for(int destProc=0; destProc < lm::MPI::worldSize; ++destProc)
//    {
//        MPI_EXCEPTION_CHECK(MPI_Send(buf, count, datatype, destProc, tag, comm));
//    }
//}
//
//template <typename t>
//void ReplicateSupervisor::MPI_MastBcastIn(t * recvtable, int recvcount, MPI_Datatype recvtype, int recvtag, MPI_Comm comm)
//{
//    MPI_Status messageStatus;
//    for(int sendProc; sendProc < lm::MPI::worldSize; ++sendProc)
//    {
//        MPI_EXCEPTION_CHECK(MPI_Recv(recvtable + sendProc, recvcount, recvtype, sendProc, recvtag, comm, &messageStatus));
//    }
//}
//
//template <typename t, int tag>
//void ReplicateSupervisor::bcastThing(void * staticDataBuffer, t * thing)
//{
//    int msgSize = thing->ByteSize();
//    if (msgSize > lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE) throw Exception("Message exceeded buffer size. Message tag:", tag);
//    thing->SerializeToArray(staticDataBuffer, msgSize);
//    bcastSizeThenBuffer<tag>(staticDataBuffer, msgSize);
//}
//
//template <int tag>
//void ReplicateSupervisor::bcastSizeThenBuffer(void * staticDataBuffer, int msgSize)
//{
//    Print::printf(Print::DEBUG, "sending msg with tag %d.", tag);
//    MPI_MastBcastOut(&msgSize, 1, MPI_INT, lm::MPI::MSG_MSG_SIZE, MPI_COMM_WORLD);
//
//    MPI_MastBcastOut(staticDataBuffer, msgSize, MPI_BYTE, tag, MPI_COMM_WORLD);
//    Print::printf(Print::DEBUG, "messge with tag %d sent.", tag);
//}
//
//int ReplicateSupervisor::FindRep(int destProc)
//{
//    int replicate = -1;
//    bool allFinished=true;
//    for (vector<int>::iterator it=replicates.begin(); it<replicates.end(); it++)
//    {
//        if (simulationStatusTable[*it] == 0)
//        {
//            replicate = *it;
//            allFinished = false;
//            break;
//        }
//        if (simulationStatusTable[*it] != 2) allFinished = false;
//    }
//    // if all replicates have finished, signal that the whole program is done
//    if (allFinished) return -2;
//    // If we are out of new simulations to run, signal that all replicates have been assigned
//    if (replicate==-1) return replicate;
//    return replicate;
//}
//
//int ReplicateSupervisor::RunRep(int destProc, int replicate)
//{
//    //send the message to start replicate to node with rank of destProc
//    MPI_EXCEPTION_CHECK(MPI_Send(&replicate, 1, MPI_INT, destProc, lm::MPI::MSG_RUN_SIMULATION, MPI_COMM_WORLD));
//    Print::printf(Print::DEBUG, "signal to start replicate %d sent", replicate);
//    simulationStatusTable[replicate] = 1;
//    return replicate;
//}

}
}
