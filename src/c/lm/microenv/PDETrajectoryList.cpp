/*
 * Copyright 2016 Johns Hopkins University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Author(s): Elijah Roberts
 */

#include <limits>
#include <list>
#include <map>
#include <string>
#include "hrtime.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/input/Input.h"
#include "lm/io/DiffusionPDEState.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/microenv/PDETrajectoryList.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/trajectory/TrajectoryList.h"
#include "robertslab/pbuf/NDArraySerializer.h"

using std::map;
using std::string;

using lm::trajectory::Trajectory;

namespace lm {
namespace microenv {

PDETrajectoryList::PDETrajectoryList(const lm::input::Input& input, uint64_t replicate)
:replicate(replicate),grid(NULL),gridElementVolume(0.0)
{
    if (!input.hasMicroenvironmentModel()) throw RuntimeException("PDETrajectoryList requires a MicroenvironmentModel as input");
    tuple<uint> gridShape(input.getMicroenvironmentModel().grid_shape().size(), (const uint*)input.getMicroenvironmentModel().grid_shape().data());
    grid = new ndarray<double>(gridShape);
    gridElementVolume = pow(input.getMicroenvironmentModel().grid_spacing(), 3.0)*1000;

    // Create the trajectory.
    trajectories[replicate] = new lm::trajectory::Trajectory(replicate, getSimulationPhase(), input, false, false, false, true);
    waitingTrajectories.insert(replicate);
}

PDETrajectoryList::~PDETrajectoryList()
{
    if (grid != NULL) delete grid; grid = NULL;
}

void PDETrajectoryList::reconcileDiffusionGrid(ndarray<uint32_t>* cellGridPoints, ndarray<double>* cellVolumes, ndarray<int32_t>* cellCurrentCounts, ndarray<int32_t>* cellFlux, uint column)
{
    // Get the trajectory state.
    lm::io::DiffusionPDEState* state = trajectories[replicate]->getMutableState()->mutable_diffusion_pde_state();

    // Get the current diffusion grid.
    robertslab::pbuf::NDArraySerializer::deserializeInto<double>(grid, state->concentrations(column));

    // Go through each cell.
    for (uint i=0; i<cellGridPoints->shape[0]; i++)
    {
        // Get the grid index of the cell.
        utuple gridIndex((*cellGridPoints)[utuple(i,0U)],(*cellGridPoints)[utuple(i,1U)],(*cellGridPoints)[utuple(i,2U)]);

        // Update the grid with the flux from the cells.
        double newC = (*grid)[gridIndex] + double((*cellFlux)[utuple(i,column)])/(gridElementVolume*NA);
        if (newC < 0.0 && fabs(newC*gridElementVolume*NA) > 0.01)
            Print::printf(Print::WARNING, "Had a negative concentation after cell %d flux: %e M, %e particles.", i, newC, newC*gridElementVolume*NA);
        (*grid)[gridIndex] = newC>0.0?newC:0.0;

        // Reset the cell counts with the concentration from the grid.
        (*cellCurrentCounts)[utuple(i,column)] = int32_t(floor((*grid)[gridIndex]*((*cellVolumes)[utuple(i)]*NA)));
    }

    // Update the state with the new diffusion grid.
    robertslab::pbuf::NDArraySerializer::serializeInto<double>(state->mutable_concentrations(column), *grid);
}

uint64_t PDETrajectoryList::findNextTrajectoryToRun() const
{
    uint64_t minId=std::numeric_limits<uint64_t>::max();
    double minTime=std::numeric_limits<double>::infinity();
    for (auto it=waitingTrajectories.begin(); it!=waitingTrajectories.end(); it++)
    {
        lm::trajectory::Trajectory* t = trajectories.at(*it);
        double time = t->getState().diffusion_pde_state().time();
        if (time < minTime)
        {
            minTime = time;
            minId = *it;
        }
    }

    if (minId == std::numeric_limits<uint64_t>::max())
    {
        for (auto it=waitingTrajectories.begin(); it!=waitingTrajectories.end(); it++)
        {
            trajectories.at(*it)->getState().PrintDebugString();
        }
        throw Exception("Consistency error in PDETrajectoryList, no next trajectory found",minId,waitingTrajectories.size());
    }

    return minId;
}

}
}
