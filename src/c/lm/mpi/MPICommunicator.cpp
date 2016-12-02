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

#ifdef OPT_MPI

#include <string>
#include <google/protobuf/message.h>

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/mpi/MPI.h"
#include "lm/mpi/MPICommunicator.h"
#include "lm/message/Communicator.h"
#include "lm/message/Endpoint.pb.h"
#include "lm/thread/Thread.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

namespace lm {
namespace mpi {

bool MPICommunicator::registered=MPICommunicator::registerClass();

bool MPICommunicator::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::message::Communicator","lm::mpi::MPICommunicator",&MPICommunicator::allocateObject);
    return true;
}

void* MPICommunicator::allocateObject()
{
    return new MPICommunicator();
}

bool MPICommunicator::classInitialized = false;
pthread_mutex_t MPICommunicator::addressMutex;
lm::message::Endpoint MPICommunicator::supervisorAddress;
int MPICommunicator::nextAddress;

MPICommunicator::MPICommunicator()
:inputBufferSize(0),outputBufferSize(0),inputBuffer(NULL),outputBuffer(NULL)
{
}

MPICommunicator::~MPICommunicator()
{
    if (inputBuffer != NULL)
    {
        MPI_EXCEPTION_CHECK(MPI_Free_mem(inputBuffer));
    }
    inputBuffer = NULL;
    if (outputBuffer != NULL)
    {
        MPI_EXCEPTION_CHECK(MPI_Free_mem(outputBuffer));
    }
    outputBuffer = NULL;
}

bool MPICommunicator::initializeClass()
{
    classInitialized = true;

    // Initialize the MPI library.
    lm::MPI::init(0, NULL);

    // Print the MPI capabilities, if we are on the master.
    if (lm::MPI::worldRank == lm::MPI::MASTER)
    {
        Print::printf(Print::INFO, "Using MPI communicator, master on host %s.", getHostname().c_str());
        lm::MPI::printCapabilities();
    }

    // Initialize the next address and mutex.
    pthread_mutexattr_t attr;
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_init(&attr));
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL));
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_init(&addressMutex, &attr));
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_destroy(&attr));
    nextAddress = 1;

    // Initialize the supervisor address.
    supervisorAddress.Clear();
    supervisorAddress.add_values(lm::MPI::MASTER);
    supervisorAddress.add_values(0);

    // Return if we are the master process.
    return lm::MPI::worldRank == lm::MPI::MASTER;
}

void MPICommunicator::finalizeClass(bool abort)
{
    if (classInitialized)
    {
        // Destroy the mutex.
        PTHREAD_EXCEPTION_CHECK(pthread_mutex_destroy(&addressMutex));

        // If we are not aborting, wait for all of the processes to synchronize.
        if (!abort)
        {
            MPI_EXCEPTION_CHECK(MPI_Barrier(MPI_COMM_WORLD));
        }

        // Close the MPI library.
        lm::MPI::finalize(abort);
    }
}

std::string MPICommunicator::getHostname() const
{
    // Get the MPI hostname.
    char hostname[MPI_MAX_PROCESSOR_NAME+1];
    memset(hostname,0,sizeof(hostname));
    int hostnameLength;
    MPI_EXCEPTION_CHECK(MPI_Get_processor_name(hostname, &hostnameLength));
    return std::string(hostname);
}


lm::message::Endpoint MPICommunicator::constructObject(bool isSupervisor)
{
    // Allocate the buffers.
    inputBufferSize = 100*1024*1024;
    outputBufferSize = 100*1024*1024;
    MPI_EXCEPTION_CHECK(MPI_Alloc_mem(inputBufferSize, MPI_INFO_NULL, &inputBuffer));
    MPI_EXCEPTION_CHECK(MPI_Alloc_mem(outputBufferSize, MPI_INFO_NULL, &outputBuffer));

    // Return the source address.
    if (isSupervisor)
    {
        return supervisorAddress;
    }
    else
    {
        lm::message::Endpoint sourceAddress;

        //// BEGIN CRITICAL SECTION: addressMutex
        PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&addressMutex));
        sourceAddress.add_values(lm::MPI::worldRank);
        sourceAddress.add_values(nextAddress++);
        PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&addressMutex));
        //// END CRITICAL SECTION: addressMutex

        return sourceAddress;
    }
}

lm::message::Endpoint MPICommunicator::getSupervisorAddress() const
{
    return supervisorAddress;
}

void MPICommunicator::sendMessage(lm::message::Endpoint destinationAddress, lm::message::Message* msg, int sleepMilliseconds) const
{
    PROF_BEGIN(PROF_MESSAGE_SEND);

    // Set the sourcre and destination addresses in the message.
    msg->mutable_source_address()->CopyFrom(sourceAddress);
    msg->mutable_destination_address()->CopyFrom(destinationAddress);

    // Serialize the message into the buffer.
    int messageLength=msg->ByteSize();
    if (messageLength > outputBufferSize) throw lm::Exception("Message too large to serialize into output buffer",messageLength,outputBufferSize);
    PROF_BEGIN(PROF_MESSAGE_SERIALIZE);
    if (!msg->SerializeToArray(outputBuffer,messageLength)) throw lm::Exception("Unable to serialize message");
    PROF_END(PROF_MESSAGE_SERIALIZE);

    mpiSend(outputBuffer, messageLength, destinationAddress, sleepMilliseconds);

    Print::printf(Print::VERBOSE_DEBUG, "Sent message %s->%s", lm::message::Communicator::printableAddress(msg->source_address()).c_str(), lm::message::Communicator::printableAddress(msg->destination_address()).c_str());
    PROF_END(PROF_MESSAGE_SEND);
}

void MPICommunicator::mpiSend(const void* buffer, int count, lm::message::Endpoint dest, int sleepMilliseconds) const
{
    MPI_EXCEPTION_CHECK(MPI_Send(buffer, count, MPI_BYTE, dest.values(0), dest.values(1), MPI_COMM_WORLD));
}

void MPICommunicator::receiveMessage(lm::message::Message* msg, int sleepMilliseconds) const
{
    PROF_BEGIN(PROF_MESSAGE_RECEIVE);

    int messageLength = mpiRecv(inputBuffer, inputBufferSize, sourceAddress, sleepMilliseconds);

    // Deserialize the message.
    PROF_BEGIN(PROF_MESSAGE_PARSE);
    if (!msg->ParseFromArray(inputBuffer, messageLength)) throw lm::Exception("Unable to deserialize message");
    PROF_END(PROF_MESSAGE_PARSE);

    Print::printf(Print::VERBOSE_DEBUG, "Received message %s->%s on %s", lm::message::Communicator::printableAddress(msg->source_address()).c_str(), lm::message::Communicator::printableAddress(msg->destination_address()).c_str(), lm::message::Communicator::printableAddress(sourceAddress).c_str());
    PROF_END(PROF_MESSAGE_RECEIVE);
}

int MPICommunicator::mpiRecv(void* buffer, int bufferSize, lm::message::Endpoint listenAddress, int sleepMilliseconds) const
{
    MPI_Status messageStatus;
    MPI_EXCEPTION_CHECK(MPI_Recv(buffer, bufferSize, MPI_BYTE, MPI_ANY_SOURCE, listenAddress.values(1), MPI_COMM_WORLD, &messageStatus));

    // Get the length of the data.
    int messageLength;
    MPI_EXCEPTION_CHECK(MPI_Get_count(&messageStatus, MPI_BYTE, &messageLength));

    return messageLength;
}

}
}

#endif
