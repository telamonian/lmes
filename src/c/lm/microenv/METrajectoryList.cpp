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
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/microenv/METrajectoryList.h"
#include "lm/trajectory/Trajectory.h"
#include "robertslab/Types.h"
#include "robertslab/pbuf/NDArraySerializer.h"

using std::map;
using std::string;

namespace lm {
namespace microenv {

METrajectoryList::METrajectoryList(const lm::input::Input& input, uint64_t replicate)
:replicate(replicate),numberCells(0)
{
    if (!input.hasMicroenvironmentModel()) throw RuntimeException("METrajectoryList requires a MicroenvironmentModel as input");

    // Go through each cell in the microenvironment.
    numberCells = input.getMicroenvironmentModel().number_cells();
    if (numberCells > 0)
    {
        ndarray<uint32_t>* initialCounts = robertslab::pbuf::NDArraySerializer::deserialize<uint32_t>(input.getMicroenvironmentModel().cell_initial_species_counts());
        uint numberSpecies = initialCounts->shape[1];
        for (uint i=0; i<numberCells; i++)
        {
            // Create a new trajectory for the cell.
            uint64_t id = replicate*numberCells+i;
            trajectories[id] = new lm::trajectory::Trajectory(id, getSimulationPhase(), input, false, true, true, false);
            waitingTrajectories[id] = trajectories[id];

            // Set the initial species counts for the cell.
            lm::io::TrajectoryState* state = trajectories[id]->getMutableState();
            if (state->cme_state().species_counts().number_species() != numberSpecies) throw RuntimeException("inconsistent number of species", state->cme_state().species_counts().number_species(), numberSpecies);
            for (uint j=0; j<numberSpecies; j++)
                state->mutable_cme_state()->mutable_species_counts()->set_species_count(j, (*initialCounts)[utuple(i,j)]);
        }
        delete initialCounts;
    }
}

METrajectoryList::~METrajectoryList()
{
}

void METrajectoryList::copySpeciesCountInto(ndarray<int32_t>* counts, uint32_t column, uint32_t speciesId)
{
    for (TrajectoryMap::const_iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        (*counts)[utuple(it->first-numberCells*replicate,column)] = it->second->getState().cme_state().species_counts().species_count(speciesId);
    }
}

void METrajectoryList::copySpeciesCountFrom(const ndarray<int32_t>& counts, uint32_t column, uint32_t speciesId)
{
    for (TrajectoryMap::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        it->second->getMutableState()->mutable_cme_state()->mutable_species_counts()->set_species_count(speciesId, counts[utuple(it->first-numberCells*replicate,column)]);
    }
}


uint64_t METrajectoryList::findNextTrajectoryToRun() const
{
    uint64_t minId=UINT64_MAX;
    double minTime=std::numeric_limits<double>::infinity();
    for (TrajectoryMap::const_iterator it=waitingTrajectories.begin(); it!=waitingTrajectories.end(); it++)
    {
        lm::trajectory::Trajectory* t = it->second;
        double time = t->getState().cme_state().species_counts().time(0);
        if (time < minTime)
        {
            minTime = time;
            minId = it->first;
        }
    }

    if (minId == UINT64_MAX)
    {
        for (TrajectoryMap::const_iterator it=waitingTrajectories.begin(); it!=waitingTrajectories.end(); it++)
        {
            it->second->getState().PrintDebugString();
        }
        throw Exception("Consistency error in ReplicateTrajectoryList, no next trajectory found",minId,waitingTrajectories.size());
    }

    return minId;
}

}
}
