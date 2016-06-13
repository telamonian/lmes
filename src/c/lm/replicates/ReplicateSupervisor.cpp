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

#include "hrtime.h"
#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/io/OutputWriter.h"
#include "lm/input/SimulationPhase.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/main/Main.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Message.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/replicates/ReplicateSupervisor.h"
#include "lm/replicates/ReplicateTrajectoryList.h"
#include "lm/resource/ResourceMap.h"

using std::map;
using std::string;
using lm::resource::ResourceMap;

namespace lm {
namespace replicates {

bool ReplicateSupervisor::registered=ReplicateSupervisor::registerClass();

bool ReplicateSupervisor::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::main::SimulationSupervisor","lm::replicates::ReplicateSupervisor",&ReplicateSupervisor::allocateObject);
    return true;
}

void* ReplicateSupervisor::allocateObject()
{
    return new ReplicateSupervisor();
}

ReplicateSupervisor::ReplicateSupervisor()
:simulationStartTime(0),numberReplicates(0)
{
}

ReplicateSupervisor::~ReplicateSupervisor()
{
}

void ReplicateSupervisor::startSimulation()
{
    simulationStartTime=getHrTime();

    // Check for some error conditions.
    if (outputWriterProcess == -1 || outputWriterThread == -1)
        throw new Exception("ReplicateSupervisor could not start the simulation, no output writer available.");

    Print::printf(Print::INFO, "Replicate supervisor starting simulation.");

    // Call the base class method
    SimulationSupervisor::startSimulation();
}

void ReplicateSupervisor::buildSimulationPhaseList()
{
    simulationPhaseList.push_back(new lm::input::SimulationPhase);
    lm::input::SimulationPhase* phase = simulationPhaseList.back();

    phase->set_id(0);
    lm::trajectory::Trajectory initialTrajectory(0, phase->id(), *input);
    for (uint64_t i=::replicates.front(); i<=::replicates.back(); i++)
    {
        phase->add_trajectory_states()->CopyFrom(initialTrajectory.getState());
    }
    phase->mutable_trajectory_limits()->CopyFrom(input->getTrajectoryLimitsMsg());
    phase->mutable_output_options()->CopyFrom(input->getOutputOptionsMsg());
}

lm::trajectory::TrajectoryList* ReplicateSupervisor::initTrajectoryList(const lm::input::SimulationPhase& phase)
{
    return new lm::replicates::ReplicateTrajectoryList(phase);
}

lm::trajectory::TrajectoryList* ReplicateSupervisor::initTrajectoryList(const lm::input::SimulationPhase& phase, const lm::trajectory::TrajectoryList& previousList)
{
    return new lm::replicates::ReplicateTrajectoryList(phase, previousList);
}

void ReplicateSupervisor::buildTrajectoryList()
{
//    // Create the new trajectory list.
//    setTrajectoryList(new ReplicateTrajectoryList(*input, ::replicates.front(), ::replicates.back()));

    // call the parent class method
    SimulationSupervisor::buildTrajectoryList();

    // grab some extra info
    numberReplicates += trajectoryList->size();
}

void ReplicateSupervisor::finishSimulation()
{
    Print::printf(Print::INFO, "Replicate supervisor finished %lld replicates in %0.2f seconds.", numberReplicates, convertHrToSeconds(getHrTime()-simulationStartTime));
    SimulationSupervisor::finishSimulation();
}

}
}
