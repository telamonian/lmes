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

#include <queue>

#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#include "hrtime.h"
#include "lm/Print.h"
#include "lm/MPI.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Communicator.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ProcessWorkUnitOutput.pb.h"
#include "lm/message/StartedOutputWriter.pb.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"

#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

namespace lm {
namespace io {


OutputWriter::OutputWriter()
    :outputFilename(""),communicator(lm::MPI::worldRank, threadNumber),messageQueueSize(0)
{
    // Create the queue mutex.
    pthread_mutexattr_t attr;
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_init(&attr));
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL));
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_init(&messageQueueMutex, &attr));
    PTHREAD_EXCEPTION_CHECK(pthread_mutexattr_destroy(&attr));

    // Create the queue signal.
    PTHREAD_EXCEPTION_CHECK(pthread_cond_init(&messageQueueSignal, NULL));
}

OutputWriter::~OutputWriter()
{
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_destroy(&messageQueueMutex));
    PTHREAD_EXCEPTION_CHECK(pthread_cond_destroy(&messageQueueSignal));
}

void OutputWriter::initialize()
{
}

void OutputWriter::wake() throw(lm::thread::PthreadException)
{
}

void OutputWriter::flush()
{
}

int OutputWriter::run()
{
    PROF_SET_THREAD(threadNumber);
    PROF_BEGIN(PROF_DATAOUTPUT_RUN);

    // Create a helper thread.
    HelperThread helperThread(this);

    try
    {
        Print::printf(Print::INFO, "OutputWriter %d:%d started.", communicator.getSourceProcess(), communicator.getSourceThread());

        // Start the helper thread.
        if (cpuNumber >= 0) helperThread.setAffinity(cpuNumber);
        helperThread.start();

        // TODO comment back in once slot is fixed.
        // Register our info with the supervisor.
//        lm::message::Message msgp;
//        lm::message::StartedOutputWriter* msg = msgp.mutable_started_output_writer();
//        msg->set_process(communicator.getSourceProcess());
//        msg->set_thread(communicator.getSourceThread());
//        communicator.sendMessage(lm::MPI::MASTER, lm::main::SimulationSupervisor::THREAD_ID, &msgp);

        // Loop reading messages.
        while (true)
        {
            // Read the next message.
            lm::message::Message* message = new lm::message::Message();
            communicator.receiveMessage(message);

            if (message->process_work_unit_output_size() > 0)
            {
                //// BEGIN CRITICAL SECTION: messageQueueMutex
                PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&messageQueueMutex));

                // Add the message to the queue.
                messageQueue.push(message);

                // Track the size of the queue.
                messageQueueSize += message->ByteSize();
                int tmpMessageQueueSize = messageQueueSize;

                // Signal that data is aavailable.
                PTHREAD_EXCEPTION_CHECK(pthread_cond_signal(&messageQueueSignal));

                PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&messageQueueMutex));
                //// END CRITICAL SECTION: messageQueueMutex

                // If the queue is too full, wait until it empties before reading any more messages.
                while (tmpMessageQueueSize > MESSAGE_QUEUE_MAX_SIZE)
                {
                    //// BEGIN CRITICAL SECTION: messageQueueMutex
                    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&messageQueueMutex));
                    tmpMessageQueueSize = messageQueueSize;
                    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&messageQueueMutex));
                    //// END CRITICAL SECTION: messageQueueMutex

                    Print::printf(Print::WARNING, "OutputWriter is receiving too much data, performance may be degraded. If this this message appear frequently, increase write intervals to increase performance. (%d bytes queued)",tmpMessageQueueSize);
                    sleep(5);
                }
            }
            else
            {
                Print::printf(Print::ERROR, "OutputWriter received an unknown message: {\n%s}",message->DebugString().c_str());
            }
        }

        // Stop the helper thread.
        helperThread.stop();

        Print::printf(Print::INFO, "OutputWriter finished.");
        PROF_END(PROF_DATAOUTPUT_RUN);
        return 0;
    }
    catch (lm::Exception e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (std::exception& e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (...)
    {
        Print::printf(Print::FATAL, "Unknown Exception during execution (%s:%d)", __FILE__, __LINE__);
    }
    PROF_END(PROF_DATAOUTPUT_RUN);

    // Stop the helper thread.
    helperThread.stop();

    return -1;
}

OutputWriter::HelperThread::HelperThread(OutputWriter* p)
:p(p)
{
}

OutputWriter::HelperThread::~HelperThread()
{
}

void OutputWriter::HelperThread::wake() throw(lm::thread::PthreadException)
{
    //// BEGIN CRITICAL SECTION: messageQueueMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&p->messageQueueMutex));
    PTHREAD_EXCEPTION_CHECK(pthread_cond_signal(&p->messageQueueSignal));
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&p->messageQueueMutex));
    //// END CRITICAL SECTION: messageQueueMutex
}

int OutputWriter::HelperThread::run()
{
    try
    {
        Print::printf(Print::INFO, "OutputWriter::HelperThread %d:%d started.", p->communicator.getSourceProcess(), threadNumber);

        // Performance stats.
        hrtime lastUpdateTime = getHrTime();
        hrtime writingTime = 0;
        int bytesWritten = 0;
        int messagesWritten = 0;
        int messagesQueued;
        int bytesQueued;

        bool finished = false;
        while (!finished)
        {
            lm::message::Message* message=NULL;
            int messageSize=0;

            //// BEGIN CRITICAL SECTION: messageQueueMutex
            PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&p->messageQueueMutex));

            // See if there are any messages.
            if (!p->messageQueue.empty())
            {
                // Get the next message.
                message = p->messageQueue.front();
                p->messageQueue.pop();

                // Update the total message size in the queue.
                messageSize = message->ByteSize();
                p->messageQueueSize -= messageSize;

                // Get some queue stats.
                messagesQueued = p->messageQueue.size();
                bytesQueued = p->messageQueueSize;
            }

            // If we are not running and the queue is empty, stop after this iteration of the loop.
            else if (!running)
            {
                finished = true;
            }

            // Otherwise, wait for more data.
            else
            {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                struct timespec waitTime;
                waitTime.tv_sec = tv.tv_sec;
                waitTime.tv_sec += 60;
                PTHREAD_TIMEOUT_EXCEPTION_CHECK(pthread_cond_timedwait(&p->messageQueueSignal, &p->messageQueueMutex, &waitTime));
            }

            PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&p->messageQueueMutex));
            //// END CRITICAL SECTION: messageQueueMutex

            // If we got a message off of the queue, process it.
            if (message != NULL)
            {
                // Loop over every output in the message.
                for (int i=0; i<message->process_work_unit_output_size(); i++)
                {
                    hrtime startWriting = getHrTime();
                    if (message->process_work_unit_output(i).has_species_counts())
                    {
                        p->processSpeciesCounts(message->process_work_unit_output(i).species_counts());
                    }
                    else
                    {
                        Print::printf(Print::ERROR, "OutputWriter received an unsupported data message: {\n%s}",message->DebugString().c_str());
                    }
                    writingTime += getHrTime()-startWriting;
                }
                bytesWritten += messageSize;
                messagesWritten++;

                // Delete the message.
                delete message;
                message = NULL;
            }

            // See if we should display some stats.
            hrtime currentTime = getHrTime();
            if (convertHrToSeconds(currentTime-lastUpdateTime) > 60.0 && bytesWritten > 0 || finished)
            {
                Print::printf(Print::INFO, "Wrote %u messages (%u bytes) in the last %0.1f seconds (%0.6f seconds writing). %u messages (%d bytes) queued. Flushing.",messagesWritten,bytesWritten,convertHrToSeconds(currentTime-lastUpdateTime), convertHrToSeconds(writingTime), messagesQueued, bytesQueued);
                p->flush();
                lastUpdateTime = currentTime;
                writingTime = 0;
                messagesWritten = 0;
                bytesWritten = 0;
            }
        }
    }
    catch (lm::Exception e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (std::exception& e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (...)
    {
        Print::printf(Print::FATAL, "Unknown Exception during execution (%s:%d)", __FILE__, __LINE__);
    }

    Print::printf(Print::INFO, "OutputWriter::HelperThread finished.");
    return 0;
}

}
}
