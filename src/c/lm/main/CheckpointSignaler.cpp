/*
  * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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

#include <cerrno>
#include <ctime>
#if defined(MACOSX)
#include <sys/time.h>
#endif
#include <pthread.h>
#include "lm/Print.h"
#include "lm/message/Communicator.h"
#include "lm/message/Endpoint.h"
#include "lm/main/CheckpointSignaler.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lm/thread/WorkerManager.h"

using lm::thread::PthreadException;
using lm::thread::WorkerManager;

namespace lm {
namespace main {

CheckpointSignaler::CheckpointSignaler(time_t checkpointInterval, int supervisorProcess, int supervisorThread)
: checkpointInterval(checkpointInterval),communicator(lm::MPI::worldRank, threadNumber),supervisorEndpoint(supervisorProcess,supervisorThread)
{
    PTHREAD_EXCEPTION_CHECK(pthread_cond_init(&controlSignal, NULL));
    nextCheckpoint.tv_sec = 0;
	nextCheckpoint.tv_nsec = 0;
}

CheckpointSignaler::~CheckpointSignaler()
{
    PTHREAD_EXCEPTION_CHECK(pthread_cond_destroy(&controlSignal));
}

void CheckpointSignaler::wake() throw(PthreadException)
{
    //// BEGIN CRITICAL SECTION: controlMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&controlMutex));
    PTHREAD_EXCEPTION_CHECK(pthread_cond_signal(&controlSignal));
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&controlMutex));
    //// END CRITICAL SECTION: controlMutex
}

int CheckpointSignaler::run()
{
    try
    {
        Print::printf(Print::INFO, "CheckpointSignaler %d:%d started, creating a checkpoint file every %d seconds.", communicator.getSourceProcess(), communicator.getSourceThread(), checkpointInterval);
        setNextCheckpointTime();

        // Register our info with the supervisor.
        lm::message::Message msgp;
        lm::message::StartedCheckpointSignaler* msg = msgp.mutable_started_checkpoint_signaler();
        msg->set_process(communicator.getSourceProcess());
        msg->set_thread(communicator.getSourceThread());
        communicator.sendMessage(supervisorEndpoint, &msgp);

        bool looping = true;
        while (looping)
        {
        	bool doCheckpoint = false;

            //// BEGIN CRITICAL SECTION: controlMutex
            PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&controlMutex));

            // See if we are checkpointing.
            if (checkpointInterval > 0)
            {
				// Sleep until our next checkpoint interval or we receive a control signal.
            	Print::printf(Print::DEBUG, "Checkpoint signaler thread waiting until the next checkpoint time.");
                int ret=pthread_cond_timedwait(&controlSignal, &controlMutex, &nextCheckpoint);
				if (ret != 0 && ret != ETIMEDOUT) throw lm::thread::PthreadException(ret,__FILE__,__LINE__);
            }
            else
            {
                // Otherwise we are not checkpointing, so just sleep until our next control change.
            	Print::printf(Print::DEBUG, "Checkpoint signaler thread waiting for a control change.");
                PTHREAD_EXCEPTION_CHECK(pthread_cond_wait(&controlSignal, &controlMutex));
            }

            Print::printf(Print::DEBUG, "Checkpoint signaler thread awoken.");

            // If we should abort or have been stopped, stop looping.
            if (aborted || !running)
            {
                looping=false;
            }

            // Otherwise, see if we need to perform a checkpoint.
            else if (checkpointInterval > 0)
            {
            	// Get the current time.
				#if defined(LINUX)
				struct timespec now;
				clock_gettime(CLOCK_REALTIME, &now);
				#elif defined(MACOSX)
				struct timeval now;
				gettimeofday(&now, NULL);
				#endif

            	// See if we are past the checkpoint time.
				if (now.tv_sec >= nextCheckpoint.tv_sec)
				{
					// Mark that we should perform the checkpoint once we are out of the critical section.
					doCheckpoint = true;
				}
            }

			PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&controlMutex));
			//// END CRITICAL SECTION: controlMutex

			// See if we should perform the checkpoint.
			if (doCheckpoint)
			{
                // Send a checkpoint message to the supervisor. Calling .mutable_perform_checkpointing() initializes the message
                Print::printf(Print::DEBUG, "Signaling a checkpoint.");
                msgp.Clear();
                msgp.mutable_perform_checkpointing();
                communicator.sendMessage(supervisorEndpoint, &msgp);

				// Update the next checkpoint time.
				setNextCheckpointTime();
			}
        }
        Print::printf(Print::INFO, "CheckpointSignaler %d:%d finished.", communicator.getSourceProcess(), communicator.getSourceThread());
        return 0;
    }
    catch (lm::thread::PthreadException & e)
    {
        Print::printf(Print::FATAL, "Pthread exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (lm::Exception & e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (std::exception & e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (...)
    {
        Print::printf(Print::FATAL, "Unknown Exception during execution (%s:%d)", __FILE__, __LINE__);
    }

    return -1;
}

void CheckpointSignaler::setNextCheckpointTime()
{
	// Set the next checkpoint time.
	#if defined(LINUX)
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	nextCheckpoint.tv_sec = now.tv_sec+checkpointInterval;
	#elif defined(MACOSX)
	struct timeval now;
	gettimeofday(&now, NULL);
	nextCheckpoint.tv_sec = now.tv_sec+checkpointInterval;
	#endif
}


}
}
