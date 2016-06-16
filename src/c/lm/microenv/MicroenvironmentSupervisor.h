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

#ifndef LM_MICROENV_MICROENVIRONMENTSUPERVISOR_H_
#define LM_MICROENV_MICROENVIRONMENTSUPERVISOR_H_

#include <list>
#include <map>
#include <deque>
#include <string>
#include <vector>

#include "hrtime.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/microenv/PDETrajectoryList.h"
#include "robertslab/Types.h"

namespace lm {
namespace microenv {

using std::deque;
using std::list;
using std::map;
using std::string;
using std::vector;

class MicroenvironmentSupervisor : public lm::main::SimulationSupervisor
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

public:
    MicroenvironmentSupervisor();
    virtual ~MicroenvironmentSupervisor();
    virtual void init();

protected:
    virtual void startWorkUnitRunners();
    virtual void receivedStartedWorkUnitRunner(const lm::message::StartedWorkUnitRunner & msg);
    virtual void startSimulation();
    virtual void startSimulationPhase();
    virtual void buildTrajectoryList();
    virtual void receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg);
    virtual bool assignWork();
    //virtual void finishSimulationPhase();
    virtual bool incrementSimulationPhase();
    virtual void finishSimulation();

protected:
    virtual void startNewReplicate();
    virtual void continueCurrentReplicate();
    virtual void buildRunWorkUnit(lm::message::RunWorkUnit* msg, bool me=true);

protected:
    virtual void printPerformanceStatistics(bool flush=false);
    virtual void resetPerformanceStatistics();

protected:
    hrtime simulationStartTime;
    uint numberReplicates;
    uint currentReplicateIndex;
    uint numberTimesteps;
    uint currentTimestep;
    double tau;
    double maxTime;
    lm::slot::SlotList pdeSlots;
    std::string pdeSolverClassName;
    lm::microenv::PDETrajectoryList* pdeTrajectoryList;
    double gridSpacing;
    uint32_t numberCells;
    ndarray<double>* cellCoordinates;
    ndarray<uint32_t>* cellGridPoints;
    ndarray<double>* cellVolumes;
    ndarray<int32_t>* cellPreviousCounts;
    ndarray<int32_t>* cellCurrentCounts;
    ndarray<int32_t>* cellFlux;

private:
    long long stats_pdeWorkUnitsSteps;
    double stats_pdeWorkUnitsTime;
    long long stats_timesteps;
    hrtime stats_timestepStartTime;
    hrtime stats_timestepTotalTime;
    hrtime stats_timestepReconcileTime;
};

}
}

#endif
