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

#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/message/Communicator.h"

namespace lm {
namespace message {

Communicator::Communicator(int process, int thread)
    :process(process),thread(thread),inputBufferSize(10*1024*1024),inputBuffer(NULL),outputBufferSize(10*1024*1024),outputBuffer(NULL)
{
    MPI_EXCEPTION_CHECK(MPI_Alloc_mem(inputBufferSize, MPI_INFO_NULL, &inputBuffer));
    MPI_EXCEPTION_CHECK(MPI_Alloc_mem(outputBufferSize, MPI_INFO_NULL, &outputBuffer));
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

std::string Communicator::getHostname()
{
    char hostname[MPI_MAX_PROCESSOR_NAME+1];
    memset(hostname,0,sizeof(hostname));
    int hostnameLength;
    MPI_EXCEPTION_CHECK(MPI_Get_processor_name(hostname, &hostnameLength));
    return std::string(hostname);
}


void Communicator::sendMessage(int destProcess, int destThread, lm::message::Message* message)
{
    // Set the message values.
    message->set_source_process(process);
    message->set_source_thread(thread);
    message->set_dest_process(destProcess);
    message->set_dest_thread(destThread);

    // Serialize the message into the buffer.
    int messageLength=message->ByteSize();
    if (messageLength > outputBufferSize) throw lm::Exception("Message to large to serialize into output buffer",messageLength,outputBufferSize);
    if (!message->SerializeToArray(outputBuffer,messageLength)) throw lm::Exception("Unable to serialize message");

    // Send the buffer.
    //lm::Print::printf(lm::Print::DEBUG, "Sending message %d:%d->%d:%d = %d",process,thread,destProcess,destThread,messageLength);
    MPI_EXCEPTION_CHECK(MPI_Send(outputBuffer, messageLength, MPI_BYTE, destProcess, destThread, MPI_COMM_WORLD));
    //lm::Print::printf(lm::Print::DEBUG, "Sent message %d:%d->%d:%d = %d",process,thread,destProcess,destThread,messageLength);
}

void Communicator::receiveMessage(lm::message::Message* message)
{
    // Receive the data.
    //lm::Print::printf(lm::Print::DEBUG, "Receiving message %d:%d",process,thread);
    MPI_EXCEPTION_CHECK(MPI_Recv(inputBuffer, inputBufferSize, MPI_BYTE, MPI_ANY_SOURCE, thread, MPI_COMM_WORLD, &messageStatus));

    // Get the length of the data.
    int messageLength;
    MPI_EXCEPTION_CHECK(MPI_Get_count(&messageStatus, MPI_BYTE, &messageLength));

    // Deserialize the message.
    if (!message->ParseFromArray(inputBuffer, messageLength)) throw lm::Exception("Unable to deserialize message");

    //lm::Print::printf(lm::Print::DEBUG, "Received message %d:%d->%d:%d = %d",message->source_process(),message->source_thread(),message->dest_process(),message->dest_thread(),messageLength);
}

}
}
