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

#ifndef LM_MICROENV_PDETrajectoryList_H_
#define LM_MICROENV_PDETrajectoryList_H_

#include <map>
#include <string>

#include "hrtime.h"
#include "lm/Types.h"
#include "lm/input/Input.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/trajectory/TrajectoryList.h"
#include "robertslab/Types.h"

using std::map;
using lm::trajectory::TrajectoryList;

namespace lm {
namespace microenv {

class PDETrajectoryList : public lm::trajectory::TrajectoryList
{

public:
    PDETrajectoryList(const lm::input::Input& input, uint64_t replicate);
    virtual ~PDETrajectoryList();

    virtual void reconcileDiffusionGrid(ndarray<uint32_t>* cellGridPoints, ndarray<double>* cellVolumes, ndarray<int32_t>* cellCurrentCounts, ndarray<int32_t>* cellFlux, uint column);

protected:
    virtual void printTrajectoryStatistics() const;
    virtual uint64_t findNextTrajectoryToRun() const;

protected:
    uint64_t replicate;
    ndarray<double>* grid;
    double gridElementVolume;

private:
    mutable hrtime stats_lastPrintTime;
};

}
}

#endif
