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
#include "lm/io/hdf5/HDF5.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Communicator.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"
#include "lm/resource/ResourceMap.h"
#include "lm/resource/SlotList.h"

using lm::resource::ResourceMap;

namespace lm {
namespace main {

SimulationSupervisor::SimulationSupervisor()
    :communicator(lm::MPI::worldRank,THREAD_ID),resourceMap(NULL),simulationInputFilename(""),simulationOutputFilename(""),outputWriterClassName(""),solverClassName(""),useCPUAffinity(false),hasReactionModel(false),hasDiffusionModel(false),slotList(&communicator)
{
}

SimulationSupervisor::~SimulationSupervisor()
{
}

void SimulationSupervisor::wake() throw(lm::thread::PthreadException)
{
}

void SimulationSupervisor::initialize()
{
    // Open the simulation file.
    lm::io::hdf5::Hdf5File * file = new lm::io::hdf5::Hdf5File(simulationInputFilename);

    // Get the simulation parameters.
    file->getParameters(&simulationParameters);

    // Map the simulation parameters.
    for (int i=0; i<simulationParameters.key_size() && i<simulationParameters.value_size(); i++)
        this->simulationParameterMap[simulationParameters.key(i)] = simulationParameters.value(i);

    // Get the reaction model.
    if (file->hasReactionModel())
    {
        hasReactionModel = true;
        file->getReactionModel(&reactionModel);
    }

    // Get the diffusion model.
    if (file->hasDiffusionModel())
    {
        hasDiffusionModel = true;
        file->getDiffusionModel(&diffusionModel);
    }

    // Close the file.
    delete file;
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
            else if (message.has_started_work_unit())
            {
                workUnitStarted(message.started_work_unit());
            }
            else if (message.has_finished_work_unit())
            {
                workUnitFinished(message.finished_work_unit());
            }
            else if (message.has_started_output_writer())
            {
                outputWriterStarted(message.started_output_writer());
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
    Print::printf(Print::INFO, "Resource controller %d:%d on %s registered with %d cpu core(s) and %d gpu device(s).", msg.controller_process(), msg.controller_thread(), msg.hostname().c_str(), msg.cpu_size(), msg.gpu_size());
    if (resourceMap->registerResources(msg))
    {
        allResourcesRegistered();
    }
}

void SimulationSupervisor::allResourcesRegistered()
{
    // Start the work unit runners.
    Print::printf(Print::INFO, "All resources registered with supervisor, starting work unit runners.");

    lm::message::StartWorkUnitRunner * s = slotList.startSlotMsg.mutable_start_work_unit_runner();
    //	s->set_use_cpu_affinity(useCPUAffinity);
    //	s->add_cpu(resources.cpuCores[i]);
    //	if (resources.gpusDevices.size() > 0)
    //		s->add_gpu(resources.gpusDevices[0]);
	s->set_solver(solverClassName);
	*s->mutable_simulation_parameters() = simulationParameters;
	if (hasReactionModel) *s->mutable_reaction_model() = reactionModel;
	if (hasDiffusionModel) *s->mutable_diffusion_model() = diffusionModel;

	map<int,ResourceMap::ComputeResources> allResources = resourceMap->getAvailableResources();
    slotList.addSlots(allResources);

    Print::printf(Print::INFO, "All work unit runners started, beginning simulation.");
    startSimulation();
}

}
}
