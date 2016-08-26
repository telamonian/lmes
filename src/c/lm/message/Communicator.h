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
#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <string>
#include <google/protobuf/message.h>

#include "lm/MPI.h"
#include "lm/message/Endpoint.h"
#include "lm/message/Message.pb.h"

namespace lm {
namespace message {

class Communicator
{
public:
    Communicator(Endpoint source);
    Communicator(int srcProcess, int srcThread);
    virtual ~Communicator();

    // accessors
    std::string getHostname() const;
    int getLastMessageSize() const {return lastMessageSize;}
    int getSourceProcess() const {return source.process;}
    int getSourceThread() const {return source.thread;}
    int getMasterOutputProcess() const {return masterOutput.process;}
    int getMasterOutputThread() const {return masterOutput.thread;}

    // serialize/deserialize messages
    void deserialize(lm::message::Message* msg) const;
    void serialize(Endpoint dest, lm::message::Message* msg) const;

    // send and receive messages
    void sendMessage(int destProcess, int destThread, lm::message::Message* msg, int sleepMilliseconds=-1) const;
    void sendMessage(Endpoint dest, lm::message::Message* msg, int sleepMilliseconds=-1) const;
    void sendMessageToMasterOutput(lm::message::Message* msg, int sleepMilliseconds=-1) const {sendMessage(masterOutput, msg, sleepMilliseconds);}
    void receiveMessage(lm::message::Message* msg, int sleepMilliseconds=0) const;

    // non-blocking send and receive messages
    int isendMessage(Endpoint dest, lm::message::Message* msg, int dummy=0) const;
    int testSendMessage() const;
    int ireceiveMessage(lm::message::Message* msg, int dummy=0) const;
    int testReceiveMessage(lm::message::Message* msg) const;

    // mutators
    void setMasterOutputEndpoint(int moProcess, int moThread);

protected:
    void initBuffers();

protected:
    // the protobuf message size limit (currently 64*MIBI) is set at compile time (for the protobuf lib itself) via the kDefaultTotalBytesLimit const var in google/protobuf/io/coded_stream.h.
    int inputBufferSize;
    int outputBufferSize;

    mutable char* inputBuffer;
    mutable int lastMessageSize;
    mutable MPI_Status messageStatus;
    mutable char* outputBuffer;

    // endpoints
    Endpoint masterOutput;
    Endpoint source;
    Endpoint supervisor;

    // variables for non-blocking operations
    mutable int sendFinished;
    mutable int receiveFinished;

    mutable MPI_Request lastSendRequest;
    mutable MPI_Status lastSendStatus;
    mutable MPI_Request lastReceiveRequest;
    mutable MPI_Status lastReceiveStatus;
};

}
}
#endif // COMMUNICATOR_H
