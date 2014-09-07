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

#include <map>
#include <string>

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/io/FFluxParameters.pb.h"
#include "lm/io/OutputWriter.h"
#include "lm/main/Main.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Message.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/fflux/FFluxSupervisor.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/resource/ResourceMap.h"

using std::map;
using std::string;
using lm::resource::ResourceMap;

namespace lm {
namespace fflux {

bool FFluxSupervisor::registered=FFluxSupervisor::registerClass();

bool FFluxSupervisor::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::main::SimulationSupervisor","lm::fflux::FFluxSupervisor",&FFluxSupervisor::allocateObject);
    return true;
}

void* FFluxSupervisor::allocateObject()
{
    return new FFluxSupervisor();
}

FFluxSupervisor::FFluxSupervisor()
:outputWriterProcess(0),outputWriterThread(3) //TODO: fix to -1,-1 once the slot code has been fixed
{

}

FFluxSupervisor::~FFluxSupervisor()
{
    if (trajectories != NULL) delete trajectories; trajectories = NULL;
}

void FFluxSupervisor::allResourcesRegistered()
{
    // Reserve a core for the output writer.
    ResourceMap::ComputeResources resources = resourceMap->reserveCPUCores(communicator.getSourceProcess(),1);
    Print::printf(Print::INFO, "Reserved core %d on %d:%d for the output writer.", resources.cpuCores[0], resources.controller_process, resources.controller_thread);

    // Start the output writer.
    lm::message::Message msg;
    msg.mutable_start_output_writer()->set_use_cpu_affinity(useCPUAffinity);
    msg.mutable_start_output_writer()->set_cpu(resources.cpuCores[0]);
    msg.mutable_start_output_writer()->set_output_filename(simulationOutputFilename);
    msg.mutable_start_output_writer()->set_output_writer_class(outputWriterClassName);
    communicator.sendMessage(resources.controller_process, resources.controller_thread, &msg);
    // TODO: the outputWriterStarted messaging stuff needs to get fixed
    // Call the base allResourcesRegistered method.
	SimulationSupervisor::allResourcesRegistered();
}

void FFluxSupervisor::outputWriterStarted(const lm::message::StartedOutputWriter& msg)
{
    outputWriterProcess = msg.process();
    outputWriterThread = msg.thread();

//    // Call the base allResourcesRegistered method.
//	SimulationSupervisor::allResourcesRegistered();
}

void FFluxSupervisor::startSimulation()
{
	double zerothInterface = -25.0;
    // Check for some error conditions.
    if (outputWriterProcess == -1 || outputWriterThread == -1)
        throw new Exception("Forward flux supervisor could not start the simulation, no output writer available.");

    Print::printf(Print::INFO, "Forward flux supervisor starting simulation.");

    // Create the new trajectory list.
    trajectories = new FFluxTrajectoryList(slots.getSlotsSize(), zerothInterface, simulationParameterMap, reactionModel, ffluxParameters);

    // Get the trajectories template msg so that we can set some default values in it
    lm::message::RunWorkUnit* runWorkUnitMsg = trajectories->getRunWorkUnitMsg();
	// Set the default source process/thread
	runWorkUnitMsg->set_supervisor_process(communicator.getSourceProcess());
	runWorkUnitMsg->set_supervisor_thread(communicator.getSourceThread());
    // Set the default writer process/thread
	runWorkUnitMsg->set_output_process(outputWriterProcess);
	runWorkUnitMsg->set_output_thread(outputWriterThread);
	// Set the default work unit-specific limits
	runWorkUnitMsg->set_max_steps(100);
	// Set the default trajectory limits
	initLimits();

	//// TEMP : replace; hardcoded increasing/decreasing limits for the forward flux test case
	limits.add_decreasing_species_count(0);
	limits.add_increasing_species_count(0);
	double a_incr = zerothInterface;
	setTestCaseLimits(NULL, &a_incr);
	//// TEMP

	lm::io::TrajectoryLimits* trajectoryLimits = new lm::io::TrajectoryLimits(limits);
	runWorkUnitMsg->set_allocated_limits(trajectoryLimits);

	// Now that the template msg has been set properly, initialize the trajectory list
	trajectories->init();

    // Call the base class method.
    SimulationSupervisor::startSimulation();
}

void FFluxSupervisor::initLimits()
{
    double firstBorder, lastBorder;
    firstBorder = ffluxParameters.interface(0).bin_border(0);
    lastBorder = ffluxParameters.interface(0).bin_border(ffluxParameters.interface(0).bin_border_size()-1);
    lastBorder >= firstBorder ? limits.set_increasing_species_count(0, firstBorder) : limits.set_decreasing_species_count(0, firstBorder);
}

//// TEMP : remove
void FFluxSupervisor::setTestCaseLimits(double* a_decr, double* a_incr)
{
	limits.set_decreasing_species_count(0, -9999);
	limits.set_increasing_species_count(0, -9999);
	if (a_decr!=NULL)
	{
		limits.set_decreasing_species_count(0, *a_decr);
	}
	if (a_incr!=NULL)
	{
		limits.set_increasing_species_count(0, *a_incr);
	}
}
//// TEMP


}
}
