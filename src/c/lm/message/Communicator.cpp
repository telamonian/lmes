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
#include <climits>
#if !defined(HOST_NAME_MAX) and defined(_POSIX_HOST_NAME_MAX)
#define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#endif
#include <cstdio>
#include <sched.h>
#include <string>
#include <unistd.h>
#include <google/protobuf/message.h>

#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/main/Globals.h"
#include "lm/message/Communicator.h"
#include "lm/message/Endpoint.pb.h"

namespace lm {
namespace message {


bool Communicator::initializeDefaultSubclass()
{
    Communicator* ret = static_cast<lm::message::Communicator*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::message::Communicator", communicatorClassName));
    return ret->initializeClass();
}

void Communicator::finalizeDefaultSubclass(bool abort)
{
    Communicator* ret = static_cast<lm::message::Communicator*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::message::Communicator", communicatorClassName));
    return ret->finalizeClass(abort);
}

Communicator* Communicator::createObjectOfDefaultSubclass(bool isSupervisor)
{
    Communicator* ret = static_cast<lm::message::Communicator*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::message::Communicator", communicatorClassName));
    ret->sourceAddress = ret->constructObject(isSupervisor);
    return ret;
}

string Communicator::printableAddress(const lm::message::Endpoint& address)
{
    string ret = "";
    for (int i=0; i<address.values_size(); i++)
    {
        char buffer[101];
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer)-1, "%s%d", (i==0?"@":":"), address.values(i));
        ret += string(buffer);
    }
    return ret;
}

Communicator::Communicator()
{
}

Communicator::~Communicator()
{
}

std::string Communicator::getHostname() const
{
    char hostname[HOST_NAME_MAX+1];
    memset(hostname,0,sizeof(hostname));
    if (gethostname(hostname, sizeof(hostname)) != 0)
        throw lm::Exception("unable to get host name");
    return std::string(hostname);
}

Endpoint Communicator::getSourceAddress() const
{
    return sourceAddress;
}

}
}
