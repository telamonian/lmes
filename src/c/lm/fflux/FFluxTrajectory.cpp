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

#include "lm/fflux/FFluxTrajectory.h"

namespace lm {
namespace fflux {

FFluxTrajectory::FFluxTrajectory(uint64_t id, lm::io::TrajectoryState* state, ):
Trajectory(id)
{
        // Construct new trajectory
        trajectories[id] = new lm::resource::Trajectory(id);

        // Initialize the trajectory's runWorkUnit message
        trajectories[id]->setMsg(trajectoryTemplateMsg);

        // Copy the TrajectoryState referenced in the function args to the TrajectoryState of the newly constructed trajectory
        trajectories[id]->setState(*state);

        // Set the trajectory id in the trajectory state.
        trajectories[id]->getState().set_trajectory_id(id);

        // Set the trajectory id in the CME state of the trajectory state (if applicable).
        if (trajectories[id]->getState().has_cme_state())
            trajectories[id]->getState().mutable_cme_state()->mutable_species_counts()->set_trajectory_id(id);

        // Set the trajectory id in the RDME state of the trajectory state (if applicable).
    //    if (trajectories[id]->getState().has_rdme_state())
    //        trajectories[id]->getState().mutable_rdme_state()->mutable_species_counts()->set_trajectory_id(id);

}

FFluxTrajectory::~FFluxTrajectory()
{
}

bool FFluxTrajectory::hasFluxedBackward()
{
    if (order==ASCENDING)
    {
        return (state.final_limit_type()==DecreasingOrderParameter);
    }
    else
    {
        return (state.final_limit_type()==IncreasingOrderParameter);
    }
}

bool FFluxTrajectory::hasFluxedForward()
{
    if (order==ASCENDING)
    {
        return (state.final_limit_type()==IncreasingOrderParameter);
    }
    else
    {
        return (state.final_limit_type()==DecreasingOrderParameter);
    }
}

void FFluxTrajectory::getSimTime()
{
    (crossings[ffluxPhase].back()->cme_state().species_counts().time(crossings[ffluxPhase].back()->cme_state().species_counts().number_entries() - 1) > maxPhaseZeroTime);
}

}
}
