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

#ifndef REPLICATESUPERVISOR_H_
#define REPLICATESUPERVISOR_H_

#include <list>
#include <map>
#include <deque>
#include <string>
#include <vector>
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/replicates/TrajectoryList.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/thread/Worker.h"

namespace lm {
namespace replicates {

using std::deque;
using std::list;
using std::map;
using std::string;
using std::vector;

class ReplicateSupervisor : public lm::main::SimulationSupervisor
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

public:
    ReplicateSupervisor();
    //ReplicateSupervisor(int * maxSlotsTable, lm::io::hdf5::Hdf5File * file) throw(PthreadException);
    virtual ~ReplicateSupervisor();

    //virtual void wake() throw(PthreadException);
    //virtual void abort() throw(PthreadException);
    //virtual void checkpoint() throw(PthreadException);
//    virtual int FindRep(int destProc);
//    virtual int RunRep(int destProc, int replicate);

    //virtual void distributeWorkUnits();
    //virtual void distributeWorkUnit(deque<Slot *>::iterator slot_it, map<int, TrajectoryAllocator::Trajectory>::iterator traj_it);
    //virtual void update(lm::work::Result & result);

//    virtual void MPI_MastBcastOut(void *buf, int count, MPI_Datatype datatype, int tag, MPI_Comm comm);
//
//    //receive from all nodes, one by one, including master. nodes should use MPI_Send plus the relevant tag to send
//    template <typename t>
//    void MPI_MastBcastIn(t * recvtable, int recvcount, MPI_Datatype recvtype, int recvtag, MPI_Comm comm);
//
//    template <typename t, int tag>
//    void bcastThing(void * staticDataBuffer, t * thing);
//
//    template <int tag>
//    void bcastSizeThenBuffer(void * staticDataBuffer, int msgSize);

    //map<int,int> simulationStatusTable;
protected:
    virtual void startSimulation();
    virtual void workUnitStarted(const lm::message::StartedWorkUnit& msg);
    virtual void workUnitFinished(const lm::message::FinishedWorkUnit& msg);

protected:
    lm::io::TrajectoryLimits limits;
    TrajectoryList* trajectories;
    long long workUnitCount;

    /*
    void * staticDataBuffer;
    //variables relating to Worker behavior
    bool shouldCheckpoint;
    bool shouldAbort;

    // Create a table for the simulation status.
    // key is replicate number, val is status: 0=waiting to run, 2=finished, other values=(?)(indicate at least not finished)
    lm::io::hdf5::Hdf5File * file;

    //the objects that manage the slots and the trajectories
    TrajectoryAllocator trajectoryAllocator;
    SupervisorSlotAllocator slotAllocator;

    map<int,struct timespec> simulationStartTimeTable; //TODO: figure out what header timespec is in and put it in this header
*/
};

}
}

#endif /* REPLICATESUPERVISOR_H_ */
