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
#include "lm/input/ReactionModel.pb.h"
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
:trajectoryMultiplier(0),numberCells(0)
{
    if (!input.hasMicroenvironmentModel()) throw RuntimeException("METrajectoryList requires a MicroenvironmentModel as input");
    if (replicate == 0) throw RuntimeException("Replicate number cannot be 0.");
    trajectoryMultiplier = replicate-1;

    // Go through each cell in the microenvironment.
    numberCells = input.getMicroenvironmentModel().number_cells();
    if (numberCells > 0)
    {
        ndarray<uint32_t>* initialCounts = robertslab::pbuf::NDArraySerializer::deserializeAllocate<uint32_t>(input.getMicroenvironmentModel().cell_initial_species_counts());
        uint numberSpecies = initialCounts->shape[1];
        for (uint i=0; i<numberCells; i++)
        {
            // Create a new trajectory for the cell.
            uint64_t id = trajectoryMultiplier*numberCells+i;
            trajectories[id] = new lm::trajectory::Trajectory(id, getSimulationPhase(), input, false, true, true, false);
            waitingTrajectories.insert(id);

            // Set the initial species counts for the cell.
            lm::io::TrajectoryState* state = trajectories[id]->getMutableState();
            if (state->cme_state().species_counts().number_species() != (int)numberSpecies) throw RuntimeException("inconsistent number of species", state->cme_state().species_counts().number_species(), numberSpecies);
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
    for (map<uint64_t,lm::trajectory::Trajectory*>::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        (*counts)[utuple(it->first-numberCells*trajectoryMultiplier,column)] = it->second->getState().cme_state().species_counts().species_count(speciesId);
    }
}

void METrajectoryList::copySpeciesCountFrom(const ndarray<int32_t>& counts, uint32_t column, uint32_t speciesId)
{
    for (map<uint64_t,lm::trajectory::Trajectory*>::iterator it=trajectories.begin(); it!=trajectories.end(); it++)
    {
        it->second->getMutableState()->mutable_cme_state()->mutable_species_counts()->set_species_count(speciesId, counts[utuple(it->first-numberCells*trajectoryMultiplier,column)]);
    }
}

}
}
