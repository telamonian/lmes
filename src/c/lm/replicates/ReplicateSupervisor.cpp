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

#include <map>
#include <string>

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/io/OutputWriter.h"
#include "lm/main/Main.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Message.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/replicates/ReplicateSupervisor.h"
#include "lm/replicates/TrajectoryList.h"
#include "lm/resource/ResourceMap.h"

using std::map;
using std::string;
using lm::resource::ResourceMap;

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

ReplicateSupervisor::ReplicateSupervisor()
:trajectories(NULL), workUnitCount(0)
{

}

ReplicateSupervisor::~ReplicateSupervisor()
{
    if (trajectories != NULL) delete trajectories; trajectories = NULL;
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

void ReplicateSupervisor::allResourcesRegistered()
{
    // Reserve a core for the output writer.
    ResourceMap::ComputeResources resources = resourceMap->reserveCPUCores(communicator.getSourceProcess(),1);

    // Start the output writer.
    Print::printf(Print::INFO, "Reserved core %d on %d:%d for the output writer.", resources.cpuCores[0], resources.controller_process, resources.controller_thread);

    // Call the base class method.
    SimulationSupervisor::allResourcesRegistered();
}

void ReplicateSupervisor::startSimulation()
{
    Print::printf(Print::INFO, "Replicate supervisor starting simulation.");

    // Create the new trajectory list.
    trajectories = new TrajectoryList(::replicates.front(), ::replicates.back());

    // See if we have a max time limit.
    if (simulationParameterMap.count("maxTime"))
        limits.set_max_time(atof(simulationParameterMap["maxTime"].c_str()));

    // Set the species lower limits from the parameters.
    if (simulationParameterMap.count("speciesLowerLimitList"))
    {
        for (int i=0; i<(int)reactionModel.number_species(); i++)
            limits.add_min_species_count(-1);

        string listString = simulationParameterMap["speciesLowerLimitList"];
        size_t start=0, end=0;
        while (end != string::npos)
        {
            end = listString.find(',', start);
            string speciesLowerLimit = listString.substr(start, (end == string::npos) ? string::npos : end - start);

            size_t equalsPos=0;
            equalsPos = speciesLowerLimit.find(':', 0);
            if (equalsPos > 0 && equalsPos < speciesLowerLimit.length()-1)
            {
                int parsedSpecies = atoi(speciesLowerLimit.substr(0, equalsPos).c_str());
                int parsedLimit = atoi(speciesLowerLimit.substr(equalsPos+1, string::npos).c_str());
                limits.set_min_species_count(parsedSpecies, parsedLimit);
                Print::printf(Print::DEBUG, "Parsed lower limit %s to: %d => %d", speciesLowerLimit.c_str(), parsedSpecies, parsedLimit);
            }
            start = end+1;
        }
    }

    // Set the species upper limits from the parameters.
    if (simulationParameterMap.count("speciesUpperLimitList"))
    {
        for (int i=0; i<(int)reactionModel.number_species(); i++)
            limits.add_max_species_count(-1);

        string listString = simulationParameterMap["speciesUpperLimitList"];
        size_t start=0, end=0;
        while (end != string::npos)
        {
            end = listString.find(',', start);
            string speciesUpperLimit = listString.substr(start, (end == string::npos) ? string::npos : end - start);

            size_t equalsPos=0;
            equalsPos = speciesUpperLimit.find(':', 0);
            if (equalsPos > 0 && equalsPos < speciesUpperLimit.length()-1)
            {
                uint parsedSpecies = atoi(speciesUpperLimit.substr(0, equalsPos).c_str());
                uint parsedLimit = atoi(speciesUpperLimit.substr(equalsPos+1, string::npos).c_str());
                limits.set_max_species_count(parsedSpecies, parsedLimit);
                Print::printf(Print::DEBUG, "Parsed upper limit %s to: %d <= %d", speciesUpperLimit.c_str(), parsedSpecies, parsedLimit);
            }
            start = end+1;
        }
    }

    // Go through the replicates to run and start the initial work units.
    // TODO move RunWorkUnit message creation and sending to a slot method
    while (true)
    {
        // Get the next trajectory to run, if there is one.
        int nextTrajectory = trajectories->nextTrajectoryToRun();
        if (nextTrajectory < 0) break;

        // Allocate the next free slot, if there is one.
        lm::resource::Slot * workSlot = slotList.alloc();
        if (workSlot==NULL) break;

        // Send the start work unit message.
        lm::message::Message msg;
        lm::message::RunWorkUnit& run = *msg.mutable_run_work_unit();
        run.set_work_unit_id(workUnitCount++);
        run.set_supervisor_process(communicator.getSourceProcess());
        run.set_supervisor_thread(communicator.getSourceThread());
        run.set_output_process(communicator.getSourceProcess()); // TODO change to output process
        run.set_output_thread(communicator.getSourceThread()); // TODO change to output thread
        run.set_max_steps(100);
        *run.mutable_initial_state() = trajectories->getTrajectoryState(nextTrajectory);
        *run.mutable_limits() = limits;
        Print::printf(Print::INFO, "Sending message to start work unit %d with trajectory %d on slot %d:%d.", run.work_unit_id(), nextTrajectory, workSlot->getSlotKey()[0], workSlot->getSlotKey()[1]);
        communicator.sendMessage(workSlot->getSlotKey()[0], workSlot->getSlotKey()[1], &msg);
        trajectories->updateTrajectoryStatus(nextTrajectory, TrajectoryList::RUNNING);
        //break;
    }
}

void ReplicateSupervisor::workUnitStarted(const lm::message::StartedWorkUnit& msg)
{
    Print::printf(Print::INFO, "Work unit %d started.",msg.work_unit_id());
}

void ReplicateSupervisor::workUnitFinished(const lm::message::FinishedWorkUnit& msg)
{
    Print::printf(Print::INFO, "Work unit %d finished in %0.3f s.",msg.work_unit_id(),msg.run_time());
    Print::printf(Print::VERBOSE_DEBUG, "Message: {\n%s}",msg.DebugString().c_str());

    if (msg.status() == lm::message::FinishedWorkUnit::LIMIT_REACHED)
    {
        trajectories->updateTrajectoryStatus(msg.final_state().trajectory_id(), TrajectoryList::FINISHED);
        trajectories->updateTrajectoryState(msg.final_state().trajectory_id(), msg.final_state());
    }
    else
    {
        trajectories->updateTrajectoryStatus(msg.final_state().trajectory_id(), TrajectoryList::WAITING);
        trajectories->updateTrajectoryState(msg.final_state().trajectory_id(), msg.final_state());
    }
    // Free the slot that the returning work unit just ran on
    slotList.free(msg.process(), msg.thread());

    // Get next available slot. If there are more trajectories than slots, this is guaranteed to be the slot we just freed. Otherwise it will be the "coldest" (longest unoccupied) slot
    lm::resource::Slot * workSlot = slotList.alloc();
    if (workSlot==NULL) Print::printf(Print::ERROR, "Slot allocation error (there was no free slot even though a slot should have been freed immediately prior)");

    // Get the next trajectory to run, if there is one.
    int nextTrajectory = trajectories->nextTrajectoryToRun();
    if (nextTrajectory >= 0)
    {
        // Send the start work unit message.
        lm::message::Message msg;
        lm::message::RunWorkUnit& run = *msg.mutable_run_work_unit();
        run.set_work_unit_id(workUnitCount++);
        run.set_supervisor_process(communicator.getSourceProcess());
        run.set_supervisor_thread(communicator.getSourceThread());
        run.set_output_process(communicator.getSourceProcess()); // TODO change to output process
        run.set_output_thread(communicator.getSourceThread()); // TODO change to output thread
        run.set_max_steps(100);
        *run.mutable_initial_state() = trajectories->getTrajectoryState(nextTrajectory);
        *run.mutable_limits() = limits;
        Print::printf(Print::INFO, "Sending message to start work unit %d with trajectory %d on slot %d:%d.", run.work_unit_id(), nextTrajectory, workSlot->getSlotKey()[0], workSlot->getSlotKey()[1]);
        communicator.sendMessage(workSlot->getSlotKey()[0], workSlot->getSlotKey()[1], &msg);
        trajectories->updateTrajectoryStatus(nextTrajectory, TrajectoryList::RUNNING);
    }

}



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
	vector<int> slotIds;
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
				distributeWorkUnit(slotAllocator.freeSlots.end() - 1, traj_it);
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
