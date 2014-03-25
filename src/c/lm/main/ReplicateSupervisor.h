/*
 * ReplicateSupervisor.h
 *
 *  Created on: Oct 4, 2013
 *      Author: tel
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
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/resource/SupervisorSlot.h"
#include "lm/resource/SupervisorSlotAllocator.h"
#include "lm/resource/TrajectoryAllocator.h"
#include "lm/main/ReplicateRunner.h"
#include "lm/me/MESolverFactory.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/thread/Worker.h"
#include "lm/work/Result.pb.h"

namespace lm {
namespace main {

using lm::me::MESolverFactory;
using lm::main::ReplicateRunner;
using lm::resource::TrajectoryAllocator;
using lm::resource::SupervisorSlot;
using lm::resource::SupervisorSlotAllocator;
using std::deque;
using std::list;
using std::map;
using std::string;
using std::vector;

class ReplicateSupervisor : public lm::thread::Worker
{

public:
    ReplicateSupervisor(int * maxSlotsTable, lm::io::hdf5::Hdf5File * file) throw(PthreadException);
    virtual ~ReplicateSupervisor() throw(PthreadException);

    virtual void wake() throw(PthreadException);
    virtual void abort() throw(PthreadException);
    virtual void checkpoint() throw(PthreadException);
    virtual int FindRep(int destProc);
    virtual int RunRep(int destProc, int replicate);

    virtual void distributeWorkUnits();
    virtual void distributeWorkUnit(deque<Slot *>::iterator slot_it, map<int, TrajectoryAllocator::Trajectory>::iterator traj_it);
    virtual void update(lm::work::Result & result);

    virtual void MPI_MastBcastOut(void *buf, int count, MPI_Datatype datatype, int tag, MPI_Comm comm);

    //receive from all nodes, one by one, including master. nodes should use MPI_Send plus the relevant tag to send
    template <typename t>
    void MPI_MastBcastIn(t * recvtable, int recvcount, MPI_Datatype recvtype, int recvtag, MPI_Comm comm);

    template <typename t, int tag>
    void bcastThing(void * staticDataBuffer, t * thing);

    template <int tag>
    void bcastSizeThenBuffer(void * staticDataBuffer, int msgSize);

    map<int,int> simulationStatusTable;
protected:
    virtual int run();

private:
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

};

}
}

#endif /* REPLICATESUPERVISOR_H_ */
