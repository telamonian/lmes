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

#include <string>
#include <google/protobuf/message.h>

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/mpi/MPI.h"
#include "lm/mpi/MPICommunicator.h"
#include "lm/message/Communicator.h"
#include "lm/message/Endpoint.pb.h"

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

MPICommunicator::MPICommunicator()
{
}

MPICommunicator::~MPICommunicator()
{
}

bool MPICommunicator::initializeClass()
{
    // Initialize the MPI library.
    lm::MPI::init(0, NULL);

    // Print the MPI capabilities, if we are on the master.
    if (lm::MPI::worldRank == lm::MPI::MASTER)
    {
        Print::printf(Print::INFO, "Using MPI communicator, master on host %s.", getHostname().c_str());
        lm::MPI::printCapabilities();
    }

    // Return if we are the master process.
    return lm::MPI::worldRank == lm::MPI::MASTER;
}

void MPICommunicator::finalizeClass(bool abort)
{
    // If we are not aborting, wait for all of the processes to synchronize.
    if (!abort)
    {
        MPI_EXCEPTION_CHECK(MPI_Barrier(MPI_COMM_WORLD));
    }

    // Close the MPI library.
    lm::MPI::finalize(abort);
}

std::string MPICommunicator::getHostname() const
{
    // Get the MPI hostname.
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int hostnameLength;
    MPI_EXCEPTION_CHECK(MPI_Get_processor_name(hostname, &hostnameLength));
    return std::string(hostname);
}


lm::message::Endpoint MPICommunicator::constructObject(bool isSupervisor)
{
    return lm::message::Endpoint();
}

lm::message::Endpoint MPICommunicator::getSupervisorAddress() const
{
    return lm::message::Endpoint();
}

void MPICommunicator::sendMessage(lm::message::Endpoint destinationAddress, lm::message::Message* msg, int sleepMilliseconds) const
{
    // Set the sourcre and destination addresses in the message.
    msg->mutable_source_address()->CopyFrom(sourceAddress);
    msg->mutable_destination_address()->CopyFrom(destinationAddress);

    Print::printf(Print::INFO, "Sending message %s->%s", lm::message::Communicator::printableAddress(msg->source_address()).c_str(), lm::message::Communicator::printableAddress(msg->destination_address()).c_str());
}

void MPICommunicator::receiveMessage(lm::message::Message* msg, int sleepMilliseconds) const
{
    Print::printf(Print::INFO, "Receving message %s", lm::message::Communicator::printableAddress(sourceAddress).c_str());

    //Print::printf(Print::INFO, "Receved message %s->%s", lm::message::Communicator::printableAddress(msg->source_address()), lm::message::Communicator::printableAddress(msg->dest_address()));
}

/*
 *




Communicator::Communicator(Endpoint source)
:source(source),lastMessageSize(0),masterOutput(source),inputBufferSize(100*1024*1024),inputBuffer(NULL),outputBufferSize(100*1024*1024),outputBuffer(NULL)
{
    initBuffers();
}

Communicator::Communicator(int srcProcess, int srcThread)
:source(srcProcess,srcThread),lastMessageSize(0),masterOutput(srcProcess,srcThread),inputBufferSize(100*1024*1024),inputBuffer(NULL),outputBufferSize(100*1024*1024),outputBuffer(NULL)
{
    initBuffers();
}

Communicator::~Communicator()
{
    if (inputBuffer != NULL)
    {
        MPI_EXCEPTION_CHECK(MPI_Free_mem(inputBuffer));
        inputBuffer = NULL;
    }
    if (outputBuffer != NULL)
    {
        MPI_EXCEPTION_CHECK(MPI_Free_mem(outputBuffer));
        outputBuffer = NULL;
    }
}

void Communicator::initBuffers()
{
    MPI_EXCEPTION_CHECK(MPI_Alloc_mem(inputBufferSize, MPI_INFO_NULL, &inputBuffer));
    MPI_EXCEPTION_CHECK(MPI_Alloc_mem(outputBufferSize, MPI_INFO_NULL, &outputBuffer));
}

std::string Communicator::getHostname() const
{
    char hostname[MPI_MAX_PROCESSOR_NAME+1];
    memset(hostname,0,sizeof(hostname));
    int hostnameLength;
    MPI_EXCEPTION_CHECK(MPI_Get_processor_name(hostname, &hostnameLength));
    return std::string(hostname);
}

void Communicator::sendMessage(int destProcess, int destThread, lm::message::Message* msg, int sleepMilliseconds) const
{
    sendMessage(Endpoint(destProcess,destThread), msg, sleepMilliseconds);
}

void Communicator::sendMessage(Endpoint dest, lm::message::Message* msg, int sleepMilliseconds) const
{
    PROF_BEGIN(PROF_MESSAGE_SEND);

    // Set the message values.
    msg->set_source_process(source.process);
    msg->set_source_thread(source.thread);
    msg->set_dest_process(dest.process);
    msg->set_dest_thread(dest.thread);

    // Serialize the message into the buffer.
    int messageLength=msg->ByteSize();
    lastMessageSize = messageLength;
    if (messageLength > outputBufferSize) throw lm::Exception("Message too large to serialize into output buffer",messageLength,outputBufferSize);

    PROF_BEGIN(PROF_MESSAGE_SERIALIZE);
    if (!msg->SerializeToArray(outputBuffer,messageLength)) throw lm::Exception("Unable to serialize message");
    PROF_END(PROF_MESSAGE_SERIALIZE);

    // Send the buffer.
    //lm::Print::printf(lm::Print::DEBUG, "Sending message %d:%d->%d:%d = %d",process,thread,destProcess,destThread,messageLength);
    if (sleepMilliseconds==-1)
    {
        MPI_Request request;
        MPI_EXCEPTION_CHECK(MPI_Isend(outputBuffer, messageLength, MPI_BYTE, dest.process, dest.thread, MPI_COMM_WORLD, &request));
        int messageSent=0;
        while (true)
        {
            MPI_EXCEPTION_CHECK(MPI_Test(&request, &messageSent, &messageStatus));
            if (messageSent)
                break;
        }
    }
    else if (sleepMilliseconds<=0)
    {
        MPI_EXCEPTION_CHECK(MPI_Send(outputBuffer, messageLength, MPI_BYTE, dest.process, dest.thread, MPI_COMM_WORLD));
    }
    else
    {
        MPI_Request request;
        MPI_EXCEPTION_CHECK(MPI_Isend(outputBuffer, messageLength, MPI_BYTE, dest.process, dest.thread, MPI_COMM_WORLD, &request));
        int messageSent=0;
        while (true)
        {
            MPI_EXCEPTION_CHECK(MPI_Test(&request, &messageSent, &messageStatus));
            if (messageSent)
                break;
            usleep(sleepMilliseconds*1000);
        }
    }
    //lm::Print::printf(lm::Print::DEBUG, "Sent message %d:%d->%d:%d = %d",process,thread,destProcess,destThread,messageLength);

    PROF_END(PROF_MESSAGE_SEND);
}

void Communicator::setMasterOutputEndpoint(int moProcess, int moThread)
{
    masterOutput.process = moProcess;
    masterOutput.thread = moThread;
}

void Communicator::receiveMessage(lm::message::Message* msg, int sleepMilliseconds) const
{
    PROF_BEGIN(PROF_MESSAGE_RECEIVE);

    // Receive the data.
    //lm::Print::printf(lm::Print::DEBUG, "Receiving message %d:%d",process,thread);

    // If we shouldn't sleep while waiting, call blocking receive.
    if (sleepMilliseconds==-1)
    {
        MPI_Request request;
        MPI_EXCEPTION_CHECK(MPI_Irecv(inputBuffer, inputBufferSize, MPI_BYTE, MPI_ANY_SOURCE, source.thread, MPI_COMM_WORLD, &request));
        int messageReceived=0;
        while (true)
        {
            MPI_EXCEPTION_CHECK(MPI_Test(&request, &messageReceived, &messageStatus));
            if (messageReceived)
                break;
        }
    }
    else if (sleepMilliseconds <= 0)
    {
        MPI_EXCEPTION_CHECK(MPI_Recv(inputBuffer, inputBufferSize, MPI_BYTE, MPI_ANY_SOURCE, source.thread, MPI_COMM_WORLD, &messageStatus));
    }
    else
    {
        // Otherwise, poll for the message sleeping in between.
        MPI_Request request;
        MPI_EXCEPTION_CHECK(MPI_Irecv(inputBuffer, inputBufferSize, MPI_BYTE, MPI_ANY_SOURCE, source.thread, MPI_COMM_WORLD, &request));
        int messageReceived=0;
        while (true)
        {
            MPI_EXCEPTION_CHECK(MPI_Test(&request, &messageReceived, &messageStatus));
            if (messageReceived)
                break;
            usleep(sleepMilliseconds*1000);
        }
    }

    // Get the length of the data.
    int messageLength;
    MPI_EXCEPTION_CHECK(MPI_Get_count(&messageStatus, MPI_BYTE, &messageLength));

    // Deserialize the message.
    PROF_BEGIN(PROF_MESSAGE_PARSE);
    if (!msg->ParseFromArray(inputBuffer, messageLength)) throw lm::Exception("Unable to deserialize message");
    PROF_END(PROF_MESSAGE_PARSE);

    //lm::Print::printf(lm::Print::DEBUG, "Received message %d:%d->%d:%d %d bytes: {\n%s}",msg->source_process(),msg->source_thread(),msg->dest_process(),msg->dest_thread(),msg->ByteSize(), msg->DebugString().c_str());

    PROF_END(PROF_MESSAGE_RECEIVE);
}
*/

}
}
