/*
 * University of Illinois Open Source License
 * Copyright 2016-2016 Roberts Group,
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
 * Author(s): Elijah Roberts
 */

#include <list>
#include <string>
#include <pthread.h>
#include <google/protobuf/message.h>

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/message/Communicator.h"
#include "lm/message/Endpoint.pb.h"
#include "lm/message/LocalCommunicator.h"
#include "lm/thread/Thread.h"

using std::list;

namespace lm {
namespace message {

bool LocalCommunicator::registered=LocalCommunicator::registerClass();

bool LocalCommunicator::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::message::Communicator","lm::message::LocalCommunicator",&LocalCommunicator::allocateObject);
    return true;
}

void* LocalCommunicator::allocateObject()
{
    return new LocalCommunicator();
}


pthread_mutex_t LocalCommunicator::addressMutex;
lm::message::Endpoint LocalCommunicator::supervisorAddress;
vector<LocalCommunicator::AddressRecord*> LocalCommunicator::addressRecords;


LocalCommunicator::LocalCommunicator()
{
}

LocalCommunicator::~LocalCommunicator()
{
}

bool LocalCommunicator::initializeClass()
{
    // Initialize the next address and mutex.
    pthread_mutexattr_t attr;
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_init(&attr));
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL));
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_init(&addressMutex, &attr));
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_destroy(&attr));

    // Initialize the supervisor address.
    supervisorAddress.Clear();
    supervisorAddress.add_values(0);

    // Add the supervisor entry to the record table.
    addressRecords.push_back(new AddressRecord());

    // Print a message.
    Print::printf(Print::INFO, "Using local communicator on host %s.", getHostname().c_str());

    // Return true since the only host must be the master.
    return true;
}

void LocalCommunicator::finalizeClass(bool abort)
{
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_destroy(&addressMutex));
}

lm::message::Endpoint LocalCommunicator::constructObject(bool isSupervisor)
{
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
        addressRecords.push_back(new AddressRecord());
        sourceAddress.add_values(addressRecords.size()-1);
        PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&addressMutex));
        //// END CRITICAL SECTION: addressMutex

        return sourceAddress;
    }
}

lm::message::Endpoint LocalCommunicator::getSupervisorAddress() const
{
    return supervisorAddress;
}

LocalCommunicator::AddressRecord::AddressRecord()
:receivingMessage(NULL)
{
    // Create the record mutex.
    pthread_mutexattr_t attr;
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_init(&attr));
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL));
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_init(&recordMutex, &attr));
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_destroy(&attr));

    // Create the record signal.
    PTHREAD_EXCEPTION_CHECK(pthread_cond_init(&recordSignal, NULL));
}

LocalCommunicator::AddressRecord::~AddressRecord()
{
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_destroy(&recordMutex));
    PTHREAD_EXCEPTION_CHECK(pthread_cond_destroy(&recordSignal));
}

void LocalCommunicator::sendMessage(Endpoint destinationAddress, lm::message::Message* msg, int sleepMilliseconds) const
{
    // Set the sourcre and destination addresses in the message.
    msg->mutable_source_address()->CopyFrom(sourceAddress);
    msg->mutable_destination_address()->CopyFrom(destinationAddress);

    //Print::printf(Print::INFO, "Sending message %s->%s", lm::message::Communicator::printableAddress(msg->source_address()).c_str(), lm::message::Communicator::printableAddress(msg->destination_address()).c_str());

    AddressRecord* record = NULL;

    //// BEGIN CRITICAL SECTION: addressMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&addressMutex));
    record = addressRecords[destinationAddress.values(0)];
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&addressMutex));
    //// END CRITICAL SECTION: addressMutex

    //// BEGIN CRITICAL SECTION: recordMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&record->recordMutex));

    // See if there is a message waiting to be received.
    if (record->receivingMessage != NULL)
    {
        // Copy the message.
        record->receivingMessage->CopyFrom(*msg);

        // Mark that the message was received.
        record->receivingMessage = NULL;

        // Signal that the message was received.
        PTHREAD_EXCEPTION_CHECK(pthread_cond_broadcast(&record->recordSignal));
    }

    // Otherwise, there is no message so wait for one.
    else
    {
        // Append the message to the list of messages waiting to be sent.
        record->sendingMessages.push_back(msg);

        // Loop until we are sure we sent the message.
        while (true)
        {
            // Wait for a signal, after we wake up the message may have been sent.
            PTHREAD_TIMEOUT_EXCEPTION_CHECK(pthread_cond_wait(&record->recordSignal, &record->recordMutex));

            // Check to see if our message was sent.
            bool messageSent = true;
            for (list<lm::message::Message*>::iterator it=record->sendingMessages.begin(); it != record->sendingMessages.end(); it++)
            {
                if (*it == msg)
                {
                    messageSent = false;
                    break;
                }
            }

            // Make sure that this was not a spurious wakeup signal.
            if (messageSent)
                break;
        }
    }

    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&record->recordMutex));
    //// END CRITICAL SECTION: recordMutex
    ///
    //Print::printf(Print::INFO, "Sent message %s->%s", lm::message::Communicator::printableAddress(msg->source_address()).c_str(), lm::message::Communicator::printableAddress(msg->destination_address()).c_str());
}

void LocalCommunicator::receiveMessage(lm::message::Message* msg, int sleepMilliseconds) const
{    
    //Print::printf(Print::INFO, "Receving message %s", lm::message::Communicator::printableAddress(sourceAddress).c_str());

    AddressRecord* record = NULL;

    //// BEGIN CRITICAL SECTION: addressMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&addressMutex));
    record = addressRecords[sourceAddress.values(0)];
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&addressMutex));
    //// END CRITICAL SECTION: addressMutex

    //// BEGIN CRITICAL SECTION: recordMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&record->recordMutex));

    // See if there are any messages waiting to be sent.
    if (record->sendingMessages.size() > 0)
    {
        // Pop the first message off the list.
        lm::message::Message* sendingMessage = record->sendingMessages.front();
        record->sendingMessages.pop_front();

        // Copy the message.
        msg->CopyFrom(*sendingMessage);

        // Signal that a message was received.
        PTHREAD_EXCEPTION_CHECK(pthread_cond_broadcast(&record->recordSignal));
    }

    // Otherwise, there is no message so wait for one.
    else
    {
        // Mark that a message is waiting to be recieved.
        record->receivingMessage = msg;

        // Loop until we are sure we received a message.
        while (true)
        {
            // Wait for a signal, after we wake up the message may have been copied.
            PTHREAD_TIMEOUT_EXCEPTION_CHECK(pthread_cond_wait(&record->recordSignal, &record->recordMutex));

            // Make sure that this was not a spurious wakeup signal.
            if (record->receivingMessage == NULL)
                break;
        }
    }

    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&record->recordMutex));
    //// END CRITICAL SECTION: recordMutex

    //Print::printf(Print::INFO, "Receved message %s->%s on %s", lm::message::Communicator::printableAddress(msg->source_address()).c_str(), lm::message::Communicator::printableAddress(msg->destination_address()).c_str(), lm::message::Communicator::printableAddress(sourceAddress).c_str());
}

}
}
