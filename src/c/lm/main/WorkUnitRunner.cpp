/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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

#include <string>
#include <map>
#include <mpi.h>
#include <pthread.h>
#include "lm/Print.h"
#include "lm/cme/CMESolver.h"
#include "lm/cme/GillespieDSolver.h"
#include "lm/cme/HillSwitch.h"
#include "lm/cme/SelfRegulatingGeneSwitch.h"
#include "lm/cme/TwoStateExpression.h"
#include "lm/cme/TwoStateHillSwitch.h"
#include "lm/cme/TwoStateHillLoopSwitch.h"
#include "lm/cme/GillespieDSolver.h"
#if defined(OPT_CUDA)
#include "lm/Cuda.h"
#endif
#include "lm/main/Main.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/main/WorkUnitRunner.h"
#include "lm/me/MESolverFactory.h"
#include "lm/MPI.h"
#include "lm/rdme/RDMESolver.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using std::string;
using std::map;
using lm::me::MESolverFactory;

namespace lm {
namespace main {

WorkUnitRunner::WorkUnitRunner(const lm::message::StartWorkUnitRunner& properties)
    :communicator(lm::MPI::worldRank, threadNumber)
{
    slot = properties.slot();
    for (int i=0; i<properties.cpu_size(); i++)
        cpus.push_back(properties.cpu(i));
    for (int i=0; i<properties.gpu_size(); i++)
        gpus.push_back(properties.gpu(i));
    solverClassName = properties.solver();
}

WorkUnitRunner::~WorkUnitRunner()
{
}

void WorkUnitRunner::wake() throw(PthreadException)
{
}

int WorkUnitRunner::run()
{
    try
    {
        Print::printf(Print::INFO, "Work Unit runner %d:%d started.", lm::MPI::worldRank, threadNumber);

        // Tell the supervisor the runner was started.
        lm::message::Message msgp;
        lm::message::StartedWorkUnitRunner* msg = msgp.mutable_started_work_unit_runner();
        msg->set_slot(slot);
        msg->set_process(lm::MPI::worldRank);
        msg->set_thread(getThreadNumber());
        communicator.sendMessage(lm::MPI::MASTER, lm::main::SimulationSupervisor::THREAD_ID, &msgp);

        // Loop reading messages.
        lm::message::Message message;
        while (true)
        {
            // Read the next message.
            communicator.receiveMessage(&message);

            // Do something with the message.
            if (false)//message.has_start_work_unit_runner())
            {
                //startWorkUnitRunner(message.start_work_unit_runner());
            }
            else
            {
                Print::printf(Print::ERROR, "Work unit runner received an unknown message: {\n%s}",message.DebugString().c_str());
            }

            // Clear the message object so it can be used again.
            message.Clear();
        }

        Print::printf(Print::INFO, "Work unit runner %d:%d finished.", lm::MPI::worldRank, threadNumber);
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
    return -1;
}
}
}
