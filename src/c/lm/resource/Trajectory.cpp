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
#include <list>
#include <map>
#include <string>

#include "lm/Print.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/resource/Trajectory.h"
#include "lm/Types.h"

using std::map;
using std::string;

namespace lm {
namespace resource {

Trajectory::Trajectory(uint64_t trajectoryID) : id(trajectoryID),status(NOT_STARTED)
{
}

Trajectory::~Trajectory()
{
}

void Trajectory::initMsg(int supervisorProcess, int supervisorThread, int outputProcess, int outputThread)
{
    getRunMsg()->set_supervisor_process(supervisorProcess);
    getRunMsg()->set_supervisor_thread(supervisorThread);
    // Set the default writer process/thread
    getRunMsg()->set_output_process(outputProcess);
    getRunMsg()->set_output_thread(outputThread);
    // Set the default work unit-specific limits
    getRunMsg()->set_max_steps(100);
}

void Trajectory::initMsg(const lm::message::Message& newMsg)
{
    setMsg(newMsg);
}

void Trajectory::initState(const lm::io::ReactionModel& reactionModel) // this version of initState creates the zeroth state from scratch
{
    state.set_trajectory_id(id);
    state.mutable_cme_state()->mutable_species_counts()->set_trajectory_id(id);
    state.mutable_cme_state()->mutable_species_counts()->set_number_species(reactionModel.number_species());
    state.mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
    for (int j=0; j<(int)reactionModel.number_species(); j++)
        state.mutable_cme_state()->mutable_species_counts()->add_species_count(reactionModel.initial_species_count(j));
    state.mutable_cme_state()->mutable_species_counts()->add_time(0.0);
}

void Trajectory::initState(const lm::io::TrajectoryState& initState)
{
    // Make instance local copy of the passed state
    setState(initState);

    // Set the trajectory id in the trajectory state.
    getState().set_trajectory_id(id);

    // Set the trajectory id in the CME state of the trajectory state (if applicable).
    if (getState().has_cme_state())
    {
        getState().mutable_cme_state()->mutable_species_counts()->set_trajectory_id(id);
    }
}



// getter definitions
lm::message::Message* Trajectory::getMsg()
{
    return &msg;
}

lm::message::RunWorkUnit* Trajectory::getRunMsg()
{
	return msg.mutable_run_work_unit();
}

Trajectory::status_t Trajectory::getStatus()
{
    return status;
}

lm::io::TrajectoryState& Trajectory::getState()
{
    return state;
}

// setter definitions
void Trajectory::setMsg(const lm::message::Message& newMsg)
{
    msg = newMsg;
}

void Trajectory::setWorkUnitId(int64_t id)
{
    getRunMsg()->set_work_unit_id(id);
}

void Trajectory::setStarted(bool trajectoryStarted)
{
    state.set_trajectory_started(trajectoryStarted);
}

void Trajectory::setStatus(status_t newStatus)
{
    status = newStatus;
}

void Trajectory::setState(const lm::io::TrajectoryState& newState)
{
    state = newState;
}

void Trajectory::updateInitialRunState()
{
	lm::io::TrajectoryState* runState = new lm::io::TrajectoryState(state);
	getRunMsg()->set_allocated_initial_state(runState);
}

}
}
