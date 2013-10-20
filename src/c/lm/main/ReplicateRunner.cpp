/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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
 * Author(s): Elijah Roberts
 */

#include <string>
#include <map>
#include <mpi.h>
#include <pthread.h>
#include "lm/Print.h"
#include "lm/cme/CMESolver.h"
#include "lm/cme/GillespieDSolver.h"
#include "lm/cme/HillSwitch.h"
#include "lm/cme/SelfRegulatingGeneSwitch.h"
#include "lm/cme/TwoStateExpression.h"
#include "lm/cme/TwoStateHillSwitch.h"
#include "lm/cme/TwoStateHillLoopSwitch.h"
#include "lm/cme/GillespieDSolver.h"
#if defined(OPT_CUDA)
#include "lm/Cuda.h"
#endif
#include "lm/main/Main.h"
#include "lm/main/ReplicateRunner.h"
#include "lm/me/MESolverFactory.h"
#include "lm/MPI.h"
#include "lm/rdme/RDMESolver.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using std::string;
using std::map;
using lm::me::MESolverFactory;

namespace lm {
namespace main {

ReplicateRunner::ReplicateRunner(int replicate, MESolverFactory solverFactory, map<string,string> * parameters, lm::io::ReactionModel * reactionModel, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize, ResourceAllocator::ComputeResources resources) throw(PthreadException)
:replicate(replicate),solverFactory(solverFactory),parameters(parameters),reactionModel(reactionModel),diffusionModel(diffusionModel),lattice(lattice),latticeSize(latticeSize),latticeSites(latticeSites),latticeSitesSize(latticeSitesSize),resources(resources),replicateFinished(false),replicateExitCode(-1)
{
}

ReplicateRunner::~ReplicateRunner() throw(PthreadException)
{
}

void ReplicateRunner::wake() throw(PthreadException)
{
}

bool ReplicateRunner::hasReplicateFinished()
{
    bool ret;

    //// BEGIN CRITICAL SECTION: controlMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&controlMutex));

    ret = replicateFinished;

    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&controlMutex));
    //// END CRITICAL SECTION: controlMutex

    return ret;
}

int ReplicateRunner::getReplicateExitCode()
{
    int ret;

    //// BEGIN CRITICAL SECTION: controlMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&controlMutex));

    ret = replicateExitCode;

    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&controlMutex));
    //// END CRITICAL SECTION: controlMutex

    return ret;
}

//void ReplicateRunner::start() throw(PthreadException)
//{
//    //// BEGIN CRITICAL SECTION: controlMutex
//    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&controlMutex));
//    if (!running)
//    {
//        void * (*foo)(void *) = &lm::main::ReplicateRunner::start_thread;
//        running=true;
//        pthread_attr_t attr;
//        PTHREAD_EXCEPTION_CHECK(pthread_attr_init(&attr));
//        PTHREAD_EXCEPTION_CHECK(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE));
//        PTHREAD_EXCEPTION_CHECK(pthread_create(&threadId, &attr, foo, this));
//        PTHREAD_EXCEPTION_CHECK(pthread_attr_destroy(&attr));
//
//        // Set the processor affinity, if we have a cpu assigned.
//        if (cpuNumber >= 0)
//        {
//            #if defined(LINUX)
//            cpu_set_t cpuset;
//            CPU_ZERO(&cpuset);
//            CPU_SET(cpuNumber, &cpuset);
//            if (pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpuset) != 0)
//                Print::printf(Print::WARNING, "Could not bind thread %u to CPU core %d", threadId, cpuNumber);
//            #endif
//        }
//        Print::printf(Print::DEBUG, "Started thread %u.", threadId);
//    }
//    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&controlMutex));
//    //// END CRITICAL SECTION: controlMutex
//}
//
//void * ReplicateRunner::start_thread(void * obj)
//{
//    Print::printf(Print::DEBUG, "in start_thread method for replicate %d", this->getReplicateExitCode());
//    int ret = (reinterpret_cast<Thread *>(obj))->run();
//    signalFinshed
//    pthread_exit((void *)ret); //why is this not wrapped with PTHREAD_EXCEPTION_CHECK?
//}

void ReplicateRunner::signalFinished()
{
    // inform the master process that this replicate is finished
    int finishedMessage[] = {this->getReplicate(), this->getReplicateExitCode()};
    Print::printf(Print::DEBUG, "sending finished message for replicate %d", finishedMessage[0]);
    MPI_EXCEPTION_CHECK(MPI_Send(&finishedMessage, 2, MPI_INT, lm::MPI::MASTER, lm::MPI::MSG_SIMULATION_FINISHED, MPI_COMM_WORLD));
    delete this;
}

}
}
