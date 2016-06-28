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
#ifndef FFLUXSUPERVISOR_H_
#define FFLUXSUPERVISOR_H_

#include <deque>
#include <valarray>

#include "lm/fflux/input/FFluxPhase.pb.h"
#include "lm/fflux/input/FFluxStage.pb.h"
#include "lm/fflux/io/FFluxPhaseOutput.pb.h"
#include "lm/fflux/io/FFluxStageOutput.pb.h"
#include "lm/fflux/FFluxInput.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/protowrap/FFluxPhaseOutput.h"
#include "lm/protowrap/Repeated.h"
#include "lm/trajectory/TrajectoryList.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/thread/Worker.h"

namespace lm {
namespace fflux {

class FFluxSupervisor : public lm::main::SimulationSupervisor
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

    virtual int getRecvSleepMilliseconds();

public:
    FFluxSupervisor();
    virtual ~FFluxSupervisor();
    virtual void init();

protected:
    // setup methods run once at the beginning of the simulation
    virtual void startSimulation();
    virtual void initSimulationStageList();
    virtual void addProductionStage(lm::fflux::input::FFluxStage* productionStage, const lm::input::Tiling& tiling, uint basinIndex);
    virtual void addPilotStage(lm::fflux::input::FFluxStage* productionStage);
    virtual void addFFluxPhases(lm::fflux::input::FFluxStage* stage);

    // setup methods run at the start of every fflux stage
    virtual void startSimulationStage();
    template <typename ValT> void buildFFluxPhaseLimit(lm::fflux::input::FFluxPhase* phase, FFPhaseLimEnums::StopCondition stopCondition, ValT value);
    template <typename ValT> void buildFFluxPhaseLimits(lm::fflux::input::FFluxStage* stage, FFPhaseLimEnums::StopCondition stopCondition, ValT value);
    virtual void buildFFluxPhaseLimitsFromInput(lm::fflux::input::FFluxStage* productionStage);
    virtual void buildFFluxPhaseLimitsFromStageOutput(lm::fflux::input::FFluxStage* productionStage, const lm::fflux::io::FFluxStageOutput& stageOutput, bool minimizeCost=true);

    // the functions where all the computational cost minimization magic happens
    inline static std::vector<uint64_t> optimizeTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const lm::fflux::io::FFluxStageOutput& stageOutput, bool minimizeCost = true);
    inline static std::vector<uint64_t> minimizeCostTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const std::vector<double>& probabilities, const std::vector<double>& costs);
    inline static std::vector<uint64_t> minimizeCountTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const std::vector<double>& probabilities);
    inline static std::valarray<double> getConstantFactors(const std::vector<double>& probabilities);

    // setup methods run at the start of every fflux phase
    virtual void startSimulationPhase();
    virtual void setLimits();
    virtual void setLimitsPhaseZero();
    virtual void buildTrajectoryList();
    virtual void buildTrajectoryListPhaseZero();

    // methods that control what happens at the end of a phase/stage
    virtual bool performAnotherSimulationPhase();
    virtual bool terminateSimulationPhase();
    virtual bool performAnotherSimulationStage();

    virtual void receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg);
    virtual void receivedFinishedWorkUnitPart(const lm::message::WorkUnitStatus& wusMsg);
    virtual void receivedFinishedWorkUnitPartPhaseZero(const lm::message::WorkUnitStatus& wusMsg);

    virtual void setTrajectoryList(lm::trajectory::TrajectoryList* newTrajectoryList);

    // getters
    virtual lm::fflux::input::FFluxPhaseLimit* getCurrentFFluxPhaseLimit() {return getCurrentStage()->mutable_fflux_phase_limits(ffluxPhaseIndex);}
    virtual lm::fflux::input::FFluxStage* getCurrentStage() {return *currentFFluxStage;}
    virtual const lm::fflux::input::FFluxStage& getCurrentStage() const {return **currentFFluxStage;}
    virtual int getStageCount() const {return ffluxStageExecutionOrder.size();}

    // deprecated
//    virtual void finishSimulation();
//    virtual void receivedProcessWorkUnitOutput(lm::message::Message& msg);
//    virtual void receivedStartedOutputWriter(const lm::message::StartedOutputWriter& msg);

protected:
    lm::fflux::input::FFluxStageList ffluxStageList;
    std::vector<lm::fflux::input::FFluxStage*> ffluxStageExecutionOrder;
    std::vector<lm::fflux::input::FFluxStage*>::iterator currentFFluxStage;
    uint64_t ffluxPhaseIndex;

    lm::fflux::FFluxInput* input;

    // shadow trajectoryList from base class with a trajectoryList with a fflux appropriate type
    lm::fflux::FFluxTrajectoryList* trajectoryList;
    lm::limit::TrajectoryLimits trajectoryLimits;
    lm::tiling::Tiling* currentTiling;

    lm::protowrap::Repeated<lm::fflux::io::FFluxPhaseOutput> ffluxPhaseOutputs;
    lm::protowrap::FFluxPhaseOutput* currentFFluxPhaseOutput;

    //    int realOutputWriterProcess;
    //    int realOutputWriterThread;
};

}
}

#endif /* FFLUXSUPERVISOR_H_ */
