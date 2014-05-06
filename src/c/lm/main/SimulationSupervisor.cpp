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


#include "lm/Exceptions.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Communicator.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"
#include "lm/resource/ResourceMap.h"

using lm::resource::ResourceMap;

namespace lm {
namespace main {

SimulationSupervisor::SimulationSupervisor()
    :communicator(lm::MPI::worldRank,THREAD_ID),resourceMap(NULL),slotsStarted(0),slotsRegistered(0)
{
}

SimulationSupervisor::~SimulationSupervisor()
{
}

void SimulationSupervisor::wake() throw(lm::thread::PthreadException)
{
}

int SimulationSupervisor::run()
{
    try
    {
        Print::printf(Print::INFO, "Supervisor %d:%d started.", lm::MPI::worldRank, threadNumber);

        // Loop reading messages.
        lm::message::Message message;
        while (true)
        {
            // Read the next message.
            communicator.receiveMessage(&message);

            // Do something with the message.
            if (message.has_resources_available())
            {
                resourceAvailable(message.resources_available());
            }
            else if (message.has_started_work_unit_runner())
            {
                workUnitRunnerStarted(message.started_work_unit_runner());
            }
            else
            {
                Print::printf(Print::ERROR, "Supervisor received an unknown message: {\n%s}",message.DebugString().c_str());
            }

            // Clear the message object so it can be used again.
            message.Clear();
        }

         //   runSimulation();
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

void SimulationSupervisor::resourceAvailable(const lm::message::ResourcesAvailable& msg)
{
    Print::printf(Print::INFO, "Host %s registered with supervisor.", msg.hostname().c_str());
    if (resourceMap->registerResources(msg))
    {
        allResourcesRegistered();
    }
}

void SimulationSupervisor::allResourcesRegistered()
{
    // Display a status message for the registered resoruces.
    map<int,ResourceMap::ComputeResources> allResources = resourceMap->getRegisteredResources();
    for (map<int,ResourceMap::ComputeResources>::iterator it=allResources.begin(); it != allResources.end(); it++)
    {
        ResourceMap::ComputeResources r = it->second;
        Print::printf(Print::INFO, "Resource controller %d:%d registered with %d cpu core(s) and %d gpu device(s).", r.controller_process, r.controller_thread, r.cpuCores.size(), r.gpusDevices.size());
    }

    // Start the work unit runners.
    Print::printf(Print::INFO, "All resources registered with supervisor, starting work unit runners.");

    // TODO change to use slot allocator code.
    int slotIndex=0;
    for (map<int,ResourceMap::ComputeResources>::iterator it=allResources.begin(); it != allResources.end(); it++)
    {
        ResourceMap::ComputeResources resources = it->second;
        lm::message::Message msg;
        for (int i=0; i<(int)resources.cpuCores.size(); i++, slotIndex++)
        {
            // Send a message to the controller to start a work unit runner.
            lm::message::StartWorkUnitRunner* s = msg.add_start_work_unit_runner();
            s->set_slot(slotIndex);
            s->add_cpu(resources.cpuCores[i]);
            if (resources.gpusDevices.size() > 0)
                s->add_gpu(resources.gpusDevices[0]);
            s->set_solver(solverClassName);
        }
        slotsStarted+=slotIndex-1;
        Print::printf(Print::INFO, "Start work unit runner(s) %d:%d start msg sent.", resources.controller_process, resources.controller_thread);
        communicator.sendMessage(resources.controller_process, resources.controller_thread, &msg);
    }
}

void SimulationSupervisor::workUnitRunnerStarted(const lm::message::StartedWorkUnitRunner& msg)
{
    Print::printf(Print::INFO, "Slot %d work unit runner %d:%d started.", msg.slot(), msg.process(), msg.thread());

    // TODO update slot allocator with slot available and start simulation if all slots are ready.
    if (++slotsRegistered == slotsStarted)
    {
        Print::printf(Print::INFO, "All work unit runners started, beginning simulation.");
        startSimulation();
    }
}


}
}
