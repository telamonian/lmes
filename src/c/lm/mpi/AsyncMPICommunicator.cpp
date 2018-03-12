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
#include "lm/mpi/AsyncMPICommunicator.h"
#include "lm/message/Communicator.h"
#include "lm/message/Endpoint.pb.h"
#include "lm/thread/Thread.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

namespace lm {
namespace mpi {

bool AsyncMPICommunicator::registered=AsyncMPICommunicator::registerClass();

bool AsyncMPICommunicator::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::message::Communicator","lm::mpi::AsyncMPICommunicator",&AsyncMPICommunicator::allocateObject);
    return true;
}

void* AsyncMPICommunicator::allocateObject()
{
    return new AsyncMPICommunicator();
}

AsyncMPICommunicator::AsyncMPICommunicator()
{
}

AsyncMPICommunicator::~AsyncMPICommunicator()
{
}

void AsyncMPICommunicator::mpiSend(const void* buffer, int count, lm::message::Endpoint dest, int sleepMilliseconds) const
{
    MPI_Request request;
    MPI_Status messageStatus;
    MPI_EXCEPTION_CHECK(MPI_Isend(buffer, count, MPI_BYTE, dest.values(0), dest.values(1), MPI_COMM_WORLD, &request));
    int messageSent=0;
    while (true)
    {
        MPI_EXCEPTION_CHECK(MPI_Test(&request, &messageSent, &messageStatus));
        if (messageSent)
            break;

        // See if we should sleep during our polling.
        if (sleepMilliseconds >= 0)
            usleep(sleepMilliseconds*1000);
    }
}

int AsyncMPICommunicator::mpiRecv(void* buffer, int bufferSize, lm::message::Endpoint listenAddress, int sleepMilliseconds) const
{
    MPI_Request request;
    MPI_Status messageStatus;
    MPI_EXCEPTION_CHECK(MPI_Irecv(buffer, bufferSize, MPI_BYTE, MPI_ANY_SOURCE, listenAddress.values(1), MPI_COMM_WORLD, &request));
    int messageReceived=0;
    while (true)
    {
        MPI_EXCEPTION_CHECK(MPI_Test(&request, &messageReceived, &messageStatus));
        if (messageReceived)
            break;

        // See if we should sleep during our polling.
        if (sleepMilliseconds >= 0)
            usleep(sleepMilliseconds*1000);
    }

    // Get the length of the data.
    int messageLength;
    MPI_EXCEPTION_CHECK(MPI_Get_count(&messageStatus, MPI_BYTE, &messageLength));

    return messageLength;
}

}
}

#endif
