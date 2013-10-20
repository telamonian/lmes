/*
 * LocalReplicateWorker.h
 *
 *  Created on: Oct 4, 2013
 *      Author: tel
 */

#ifndef LOCALREPLICATEWORKER_H_
#define LOCALREPLICATEWORKER_H_

#include <list>
#include <map>
#include <string>
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/main/ResourceAllocator.h"
#include "lm/main/ReplicateRunner.h"
#include "lm/me/MESolverFactory.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/thread/Worker.h"

namespace lm {
namespace main {

using lm::me::MESolverFactory;
using lm::main::ReplicateRunner;
using std::list;
using std::map;
using std::string;

class LocalReplicateWorker : public lm::thread::Worker
{


public:
    LocalReplicateWorker(lm::io::hdf5::SimulationFile * file) throw(PthreadException);
    virtual ~LocalReplicateWorker() throw(PthreadException);

    virtual void wake() throw(PthreadException);
    virtual void abort() throw(PthreadException);
    virtual void checkpoint() throw(PthreadException);
    void broadcastSimulationParameters(void * staticDataBuffer, map<string,string> & simulationParameters);
    void broadcastReactionModel(void * staticDataBuffer, lm::io::ReactionModel * reactionModel);
    void broadcastDiffusionModel(void * staticDataBuffer, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize);
    int FindRunRep(int destProc);
    void MPI_MastBcastOut(void *buf, int count, MPI_Datatype datatype, int tag, MPI_Comm comm);

    //receive from all nodes, one by one, including master. nodes should use MPI_Send plus the relevant tag to send
    template <typename t>
    void MPI_MastBcastIn(t * recvtable, int recvcount, MPI_Datatype recvtype, int recvtag, MPI_Comm comm)
    {
        MPI_Status messageStatus;
        for(int sendProc; sendProc < lm::MPI::worldSize; ++sendProc)
        {
            MPI_EXCEPTION_CHECK(MPI_Recv(recvtable + sendProc, recvcount, recvtype, sendProc, recvtag, comm, &messageStatus));
        }
    }

    template <typename t, int tag>
    void bcastThing(void * staticDataBuffer, t * thing)
    {
        int msgSize = thing->ByteSize();
        if (msgSize > lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE) throw Exception("Message exceeded buffer size. Message tag:", tag);
        thing->SerializeToArray(staticDataBuffer, msgSize);
        bcastSizeThenBuffer<tag>(staticDataBuffer, msgSize);
    }

    template <int tag>
    void bcastSizeThenBuffer(void * staticDataBuffer, int msgSize)
    {
        Print::printf(Print::DEBUG, "sending msg with tag %d.", tag);
        MPI_MastBcastOut(&msgSize, 1, MPI_INT, lm::MPI::MSG_MSG_SIZE, MPI_COMM_WORLD);

        MPI_MastBcastOut(staticDataBuffer, msgSize, MPI_BYTE, tag, MPI_COMM_WORLD);
        Print::printf(Print::DEBUG, "messge with tag %d sent.", tag);
//        while(true);
    }

protected:
    virtual int run();

private:
    //variables relating to Worker behavior
    bool shouldCheckpoint;
    bool shouldAbort;

    // Create a table for the simulation status.
    // key is replicate number, val is status: 0=waiting to run, 2=finished, other values=(?)(indicate at least not finished)
    lm::io::hdf5::SimulationFile * file;
    map<int,int> simulationStatusTable;
    map<int,struct timespec> simulationStartTimeTable; //TODO: figure out what header timespec is in and put it in this header

};

}
}

#endif /* LOCALREPLICATEWORKER_H_ */
