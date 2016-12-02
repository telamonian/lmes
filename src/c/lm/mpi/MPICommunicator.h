/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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

#ifndef LM_MPI_MPICOMMUNICTOR_H
#define LM_MPI_MPICOMMUNICTOR_H

#include <string>
#include <pthread.h>
#include <google/protobuf/message.h>

#include "lm/message/Communicator.h"
#include "lm/message/Endpoint.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/mpi/MPI.h"

using std::string;

namespace lm {
namespace mpi {

class MPICommunicator : public lm::message::Communicator
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

private:
    static bool classInitialized;
    static pthread_mutex_t addressMutex;
    static lm::message::Endpoint supervisorAddress;
    static int nextAddress;

public:
    MPICommunicator();
    virtual ~MPICommunicator();

    // accessors
    virtual string getHostname() const;
    virtual lm::message::Endpoint getSupervisorAddress() const;

    // send and receive messages
    virtual void sendMessage(lm::message::Endpoint dest, lm::message::Message* msg, int sleepMilliseconds=0) const;
    virtual void receiveMessage(lm::message::Message* msg, int sleepMilliseconds=0) const;

protected:
    virtual bool initializeClass();
    virtual void finalizeClass(bool abort);
    virtual lm::message::Endpoint constructObject(bool isSupervisor);
    virtual void mpiSend(const void* buffer, int count, lm::message::Endpoint dest, int sleepMilliseconds) const;
    virtual int mpiRecv(void* buffer, int bufferSize, lm::message::Endpoint listenAddress, int sleepMilliseconds) const;

protected:
    size_t inputBufferSize, outputBufferSize;
    void* inputBuffer;
    void* outputBuffer;
};

}
}
#endif // LM_MPI_MPICOMMUNICTOR_H

#endif
