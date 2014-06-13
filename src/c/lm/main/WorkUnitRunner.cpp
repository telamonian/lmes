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
#include <vector>

#include "hrtime.h"
#include "lm/ClassFactory.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#if defined(OPT_CUDA)
#include "lm/Cuda.h"
#endif
#include "lm/main/Main.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/main/WorkUnitRunner.h"
#include "lm/me/MESolver.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using std::map;
using std::string;
using std::vector;

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
    lm::message::Message msg;
    msg.mutable_ping_target()->set_id(0);
    communicator.sendMessage(communicator.getSourceProcess(), communicator.getSourceThread(), &msg);
}

int WorkUnitRunner::run()
{
    try
    {
        Print::printf(Print::INFO, "Work Unit runner %d:%d started with %d cpu cores (affinity=%d) and %d gpus.", lm::MPI::worldRank, threadNumber, properties.cpu_size(), properties.use_cpu_affinity(), properties.gpu_size());

        // Set the processor affinity.
        if (properties.use_cpu_affinity() && properties.cpu_size() > 0)
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

        // Set the solver resources.
        vector<int> cpus;
        vector<int> gpus;
        for (int i=0; i<properties.cpu_size(); i++) cpus.push_back(properties.cpu(i));
        for (int i=0; i<properties.gpu_size(); i++) gpus.push_back(properties.gpu(i));
        solver->setComputeResources(cpus, gpus);

        // Set the simulation parameters.
        solver->setSimulationParameters(properties.simulation_parameters());

        // Set the model for the solver.
        if (solver->needsReactionModel())
        {
            if (properties.has_reaction_model())
                solver->setReactionModel(properties.reaction_model());
            else
                throw Exception("Work Unit runner terminating, solver requires a reaction model but none was specified", properties.solver().c_str());
        }
        if (solver->needsDiffusionModel())
        {
            if (properties.has_diffusion_model())
                solver->setDiffusionModel(properties.diffusion_model());
            else
                throw Exception("Work Unit runner terminating, solver requires a diffusion model but none was specified", properties.solver().c_str());
        }

        // Tell the supervisor the runner was started.
        lm::message::Message msgp;
        lm::message::StartedWorkUnitRunner* msg = msgp.mutable_started_work_unit_runner();
        msg->set_process(lm::MPI::worldRank);
        msg->set_thread(getThreadNumber());
        msg->set_simultaneous_work_units(solver->getSimultaneousTrajectories());
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
            else if (message.has_ping_target())
            {
                // If we are done running, stop the loop.
                if (!running) break;
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

void WorkUnitRunner::runWorkUnit(const lm::message::RunWorkUnit& wu)
{
    // Tell the supervisor the work unit is started.
    lm::message::Message msgp1;
    lm::message::StartedWorkUnit* msg1 = msgp1.mutable_started_work_unit();
    msg1->set_work_unit_id(wu.work_unit_id());
    communicator.sendMessage(wu.supervisor_process(), wu.supervisor_thread(), &msgp1);

    // Reset the solver.
    solver->reset();

    // Set the communicator.
    solver->setCommunicator(&communicator, wu.output_process(), wu.output_thread(), wu.work_unit_id());

    // Set the initial state.
    solver->setState(wu.initial_state());

    // Set the limits.
    if (wu.has_limits()) solver->setLimits(wu.limits());

    // Run the work unit.
    hrtime t1=getHrTime();
    bool limitReached=solver->generateTrajectory(wu.max_steps());
    hrtime t2=getHrTime();

    // Tell the supervisor the work unit has finished.
    lm::message::Message msgp2;
    lm::message::FinishedWorkUnit* msg2 = msgp2.mutable_finished_work_unit();
    msg2->set_work_unit_id(wu.work_unit_id());
    msg2->set_process(lm::MPI::worldRank);
    msg2->set_thread(threadNumber);
    if (limitReached)
        msg2->set_status(lm::message::FinishedWorkUnit::LIMIT_REACHED);
    else
        msg2->set_status(lm::message::FinishedWorkUnit::STEPS_FINISHED);
    msg2->set_run_time(convertHrToSeconds(t2-t1));
    msg2->mutable_final_state()->set_trajectory_id(wu.initial_state().trajectory_id());
    solver->getState(msg2->mutable_final_state());

    communicator.sendMessage(wu.supervisor_process(), wu.supervisor_thread(), &msgp2);

}

}
}
