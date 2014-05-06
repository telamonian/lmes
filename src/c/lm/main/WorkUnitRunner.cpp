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
#include "lm/MPI.h"
#include "lm/rdme/RDMESolver.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using std::string;
using std::map;

namespace lm {
namespace main {

WorkUnitRunner::WorkUnitRunner(const lm::message::StartWorkUnitRunner& msg)
    :communicator(lm::MPI::worldRank,threadNumber),properties(msg),solver(NULL)
{
}

WorkUnitRunner::~WorkUnitRunner()
{
    if (solver != NULL) delete solver; solver = NULL;
}

void WorkUnitRunner::wake() throw(PthreadException)
{
}

int WorkUnitRunner::run()
{
    try
    {
        Print::printf(Print::INFO, "Work Unit runner %d:%d started.", lm::MPI::worldRank, threadNumber);

        // Set the processor affinity.
        if (properties.use_cpu_affinity() && properties.cpu_size() >0)
        {
            Print::printf(Print::INFO, "Work Unit runner %d:%d using cpu core %d.", lm::MPI::worldRank, threadNumber, properties.cpu(0));
            setAffinity(properties.cpu(0));
        }

        // Set the GPU affinity.
        #if defined(OPT_CUDA)
        if (properties.gpu_size() > 0)
        {
            Print::printf(Print::INFO, "Work Unit runner %d:%d using gpu device %d.", lm::MPI::worldRank, threadNumber, properties.gpu(0));
            lm::CUDA::setCurrentDevice(properties.gpu(0));
        }
        #endif

        // Instantiate the solver.
        solver = static_cast<lm::me::MESolver*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::me::MESolver",properties.solver()));

        // Set the model for the solver.
        //solver->initialize(replicate, parameters, &resources);
        if (solver->needsReactionModel())
        {
            if (properties.has_reaction_model())
                ((lm::cme::CMESolver *)solver)->setReactionModel(properties.mutable_reaction_model());
            else
                throw Exception("Work Unit runner terminating, solver requires a reaction model but none was specified", properties.solver().c_str());
        }
        if (solver->needsDiffusionModel())
        {
            if (properties.has_diffusion_model())
                ((lm::rdme::RDMESolver *)solver)->setDiffusionModel(properties.mutable_diffusion_model());
            else
                throw Exception("Work Unit runner terminating, solver requires a diffusion model but none was specified", properties.solver().c_str());
        }

        // Tell the supervisor the runner was started.
        lm::message::Message msgp;
        lm::message::StartedWorkUnitRunner* msg = msgp.mutable_started_work_unit_runner();
        msg->set_slot(properties.slot());
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
            if (message.has_run_work_unit())
            {
                runWorkUnit(message.run_work_unit());
            }
            else
            {
                Print::printf(Print::ERROR, "Work unit runner received an unknown message: {\n%s}",message.DebugString().c_str());
            }

            // Clear the message object so it can be used again.
            message.Clear();
        }

        //Delete the solver.
        if (solver != NULL) delete solver; solver = NULL;

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

void WorkUnitRunner::runWorkUnit(const lm::message::RunWorkUnit& msg)
{

}

}
}
