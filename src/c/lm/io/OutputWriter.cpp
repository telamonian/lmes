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

#include <lm/Print.h>
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
    :communicator(lm::MPI::worldRank, threadNumber),messageQueueSize(0)
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

//void OutputWriter::pushDataSet(DataSet * dataSet) throw(PthreadException)
//{
//    bool success=false;
//    void * staticDataBuffer = NULL;
//    MPI_EXCEPTION_CHECK(MPI_Alloc_mem(lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE, MPI_INFO_NULL, &staticDataBuffer));
////    //// BEGIN CRITICAL SECTION: dataMutex
////    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&dataMutex));
////    if (running)
////    {
////        if (dataSet != NULL)
////        {
////            dataQueue.push(dataSet);
////            PTHREAD_EXCEPTION_CHECK(pthread_cond_signal(&dataAvailable));
////        }
////        success = true;
////    }
////    else
////    {
////        delete dataSet;
////    }
////    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&dataMutex));
////    //// END CRITICAL SECTION: dataMutex
//    if (running)
//    {
//        if (dataSet !=NULL)
//        {
//            Print::printf(Print::VERBOSE_DEBUG, "Sending output data set from process %d.", lm::MPI::worldRank);
//
//            // Put the next data set into the send buffer.
//            size_t messageSize=dataOutputQueue->popDataSetIntoBuffer(staticDataBuffer, lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE);
//
//            MPI_EXCEPTION_CHECK(MPI_Send(staticDataBuffer, messageSize, MPI_BYTE, lm::MPI::MASTER, lm::MPI::MSG_OUTPUT_DATA_STATIC, MPI_COMM_WORLD));
//        }
//        else
//        {
//            delete dataSet;
//        }
//    }
//    MPI_EXCEPTION_CHECK(MPI_Free_mem(staticDataBuffer));
//    if (!success) throw lm::Exception("OutputWriter is not running.");
//}

void OutputWriter::wake() throw(lm::thread::PthreadException)
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
                // Add the message to the queue.
                messageQueue.push(message);

                // Signal that data is aavailable.
                //// BEGIN CRITICAL SECTION: messageQueueMutex
                PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&messageQueueMutex));
                PTHREAD_EXCEPTION_CHECK(pthread_cond_signal(&messageQueueSignal));
                PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&messageQueueMutex));
                //// END CRITICAL SECTION: messageQueueMutex
            }
            else
            {
                Print::printf(Print::ERROR, "OutputWriter received an unknown message: {\n%s}",message->DebugString().c_str());
            }
        }

//            // See if we should display some stats.
//            timing_time_t currentTime = TIMING_GET_TIME;
//            if (currentTime-lastUpdateTime > 60*1000000000ULL)
//            {
//                Print::printf(Print::INFO, "Wrote %u data sets (%u bytes) in the last %0.2f seconds (%0.2f seconds writing). %u datasets queued. Flushing.",datasetsWritten,bytesWritten,((double)(currentTime-lastUpdateTime))/1000000000.0, ((double)writingTime)/1000000000.0, datasetsRemaining);
//                file->flush();
//                lastUpdateTime = currentTime;
//                writingTime = 0;
//                datasetsWritten = 0;
//                bytesWritten = 0;
//            }
//        }

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

        while (running)
        {
            lm::message::Message* message=NULL;

            //// BEGIN CRITICAL SECTION: messageQueueMutex
            PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&p->messageQueueMutex));

            // See if there are any messages.
            if (!p->messageQueue.empty())
            {
                // Get the next message.
                message = p->messageQueue.front();
                p->messageQueue.pop();

                // Update the total message size in the queue.
                p->messageQueueSize -= message->ByteSize();
            }
            else
            {
                PTHREAD_EXCEPTION_CHECK(pthread_cond_wait(&p->messageQueueSignal, &p->messageQueueMutex));
            }

            PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&p->messageQueueMutex));
            //// END CRITICAL SECTION: messageQueueMutex

            // If we got a message off of the queue, process it.
            if (message != NULL)
            {
                // Loop over every output in the message.
                for (int i=0; i<message->process_work_unit_output_size(); i++)
                {
                    if (message->process_work_unit_output(i).has_species_counts())
                    {
                        p->processSpeciesCounts(message->process_work_unit_output(i).species_counts());
                    }
                    else
                    {
                        Print::printf(Print::ERROR, "OutputWriter received an unsupported data message: {\n%s}",message->DebugString().c_str());
                    }
                }

                // Delete the message.
                delete message;
                message = NULL;
            }

            //            // See if we should display some stats.
            //            timing_time_t currentTime = TIMING_GET_TIME;
            //            if (currentTime-lastUpdateTime > 60*1000000000ULL)
            //            {
            //                Print::printf(Print::INFO, "Wrote %u data sets (%u bytes) in the last %0.2f seconds (%0.2f seconds writing). %u datasets queued. Flushing.",datasetsWritten,bytesWritten,((double)(currentTime-lastUpdateTime))/1000000000.0, ((double)writingTime)/1000000000.0, datasetsRemaining);
            //                file->flush();
            //                lastUpdateTime = currentTime;
            //                writingTime = 0;
            //                datasetsWritten = 0;
            //                bytesWritten = 0;
            //            }
            //        }
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
    return 0;
}

}
}
