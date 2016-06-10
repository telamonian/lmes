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
#include "lm/microenv/PDETrajectoryList.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/trajectory/TrajectoryList.h"

using std::map;
using std::string;



namespace lm {
namespace microenv {

PDETrajectoryList::PDETrajectoryList(const lm::input::Input& input, uint64_t replicate)
:replicate(replicate)
{
    trajectories[replicate] = new lm::trajectory::Trajectory(replicate, getSimulationPhase(), input, false, false, false, true);
    waitingTrajectories[replicate] = trajectories[replicate];
}

PDETrajectoryList::~PDETrajectoryList()
{
}

}
}
