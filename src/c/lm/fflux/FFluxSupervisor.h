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

#include <valarray>

#include "lm/EnumHelper.h"
#include "lm/fflux/input/FFluxPhase.pb.h"
#include "lm/fflux/input/FFluxStage.pb.h"
#include "lm/fflux/io/FFluxPhaseOutput.pb.h"
#include "lm/fflux/io/FFluxStageOutput.pb.h"
#include "lm/fflux/input/FFluxInput.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/protowrap/FFluxPhaseOutput.h"
#include "lm/protowrap/FFluxStageOutput.h"
#include "lm/protowrap/Repeated.h"
#include "lm/trajectory/TrajectoryList.h"

namespace lm {
namespace fflux {

class FFluxSupervisor : public lm::main::SimulationSupervisor
{
public:
    typedef lm::protowrap::Repeated<lm::fflux::input::FFluxPhase> FFluxPhases;
    typedef lm::protowrap::Repeated<lm::fflux::input::FFluxPhaseLimit> FFluxPhaseLimits;
    typedef lm::protowrap::Repeated<lm::fflux::io::FFluxPhaseOutput> FFluxPhaseOutputs;
    typedef lm::protowrap::Repeated<lm::fflux::io::FFluxStageOutput> FFluxStageOutputs;
    typedef std::vector<lm::fflux::input::FFluxStage*> FFluxStageVector;

    static bool registered;
    static bool registerClass();
    static void* allocateObject();

    virtual int getRecvSleepMilliseconds();

public:
    FFluxSupervisor();
    virtual ~FFluxSupervisor();
    virtual void init();

protected:
    // setup methods that run once at the beginning of the simulation
    virtual void startSimulation();
    virtual void initSimulationStageList();
    virtual lm::fflux::input::FFluxStage* buildProductionStage(lm::fflux::input::FFluxStage* productionStage, const lm::input::Tiling& tiling, int basinIndex);
    virtual lm::fflux::input::FFluxStage* addPilotStage(lm::fflux::input::FFluxStage* productionStage);
    virtual void addFFluxPhases(lm::fflux::input::FFluxStage* stage, FFPhaseEnums::TrajectoryGeneration trajGeneration, FFPhaseEnums::TrajectoryDuplication trajDuplication);

    // setup methods that run at the start of every fflux stage
    virtual void startSimulationStage();
    template <typename ValT> void buildFFluxPhaseLimit(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, FFPhaseLimEnums::StopCondition stopCondition, ValT value);
    template <typename ValT> void addFFluxPhaseLimits(lm::fflux::input::FFluxStage* stage, FFPhaseLimEnums::StopCondition stopCondition, ValT value);
    template <typename ValT> void repeatFFluxPhaseLimits(lm::fflux::input::FFluxStage* stage, const lm::fflux::input::FFluxPhaseLimit& limitToRepeat);
//    template <typename ValT> void repeatFFluxPhaseLimits(lm::protowrap::Repeated<lm::fflux::input::FFluxPhaseLimit>::iterator begin, const lm::fflux::input::FFluxPhaseLimit& limitToRepeat);
    virtual void addFFluxPhaseLimitsFromInput(lm::fflux::input::FFluxStage* productionStage);
    virtual void addFFluxPhaseLimitsFromStageOutput(lm::fflux::input::FFluxStage* productionStage, const lm::protowrap::FFluxStageOutput& stageOutput, bool minimizeCost = true);

    // the functions where all the computational cost minimization magic happens
    inline static std::vector<uint64_t> optimizeTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const lm::protowrap::FFluxStageOutput& stageOutput, bool minimizeCost = true);
    inline static std::vector<uint64_t> minimizeCostTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const std::vector<double>& probabilities, const std::vector<double>& costs);
    inline static std::vector<uint64_t> minimizeCountTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const std::vector<double>& probabilities);
    inline static std::valarray<double> getConstantFactors(const std::vector<double>& probabilities);

    // setup methods that run at the start of every fflux phase
    virtual void startSimulationPhase();
    virtual void setTrajectoryLimits();
    virtual void setTrajectoryLimitsPhaseZero();
    virtual void buildTrajectoryList();

    // methods that control what happens at the end of a ffluxPhase
    virtual bool terminateSimulationPhase();
    virtual void finishSimulationPhase();
    virtual bool performAnotherSimulationPhase() {return isCurrentPhaseLast();}
    virtual void incrementSimulationPhase();
    virtual void addFFluxPhaseOutput();

    // methods that control what happens at the end of a ffluxStage
    virtual void finishSimulationStage();
    virtual bool performAnotherSimulationStage() {return isCurrentStageLast();}
    virtual void incrementSimulationStage();
    virtual void addFFluxStageOutput();

    // methods that handle FinishedWorkUnit messages
    virtual void receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg);
    virtual void receivedFinishedWorkUnitPart(const lm::message::WorkUnitStatus& wusMsg);
    virtual void receivedFinishedWorkUnitPartPhaseZero(const lm::message::WorkUnitStatus& wusMsg);

    // accessors
    virtual const lm::fflux::input::FFluxPhase& currentPhase() const {return *currentFFluxPhaseIter;}
    virtual int64_t currentFFluxPhaseIndex() const {return currentPhase().fflux_phase_index();}
    virtual const lm::fflux::input::FFluxPhaseLimit& currentPhaseLimit() const {return currentStage().fflux_phase_limits(currentFFluxPhaseIndex());}
    virtual const lm::protowrap::FFluxPhaseOutput& currentPhaseOutput() const {return *currentFFluxPhaseOutputPtr;}
    virtual int64_t finalFFluxPhaseIndex() const {return currentStage().fflux_phases_size() - 1;}
    virtual bool isCurrentPhaseLast() const {return currentFFluxPhaseIter==currentStage().fflux_phases().end();}
    virtual const lm::protowrap::FFluxPhaseOutput& previousPhaseOutput() const {return *previousFFluxPhaseOutputPtr;}

    virtual const lm::fflux::input::FFluxStage& currentStage() const {return **currentFFluxStageIter;}
    virtual const lm::protowrap::FFluxStageOutput& currentStageOutput() const {return currentFFluxStageOutput;}
    virtual int getStageCount() const {return ffluxStageExecutionOrder.size();}
    virtual bool isCurrentStageLast() const {return currentFFluxStageIter==ffluxStageExecutionOrder.end();}

    virtual const lm::tiling::Tiling& currentTiling() const {return *currentTilingPtr;}

    // mutators
    virtual lm::fflux::input::FFluxPhase* mutableCurrentPhase() {return &*currentFFluxPhaseIter;}
    virtual lm::fflux::input::FFluxPhaseLimit* mutableCurrentPhaseLimit() {return mutableCurrentStage()->mutable_fflux_phase_limits(currentFFluxPhaseIndex());}
    virtual lm::protowrap::FFluxPhaseOutput* mutableCurrentPhaseOutput() {return currentFFluxPhaseOutputPtr;}

    virtual lm::fflux::input::FFluxStage* mutableCurrentStage() {return *currentFFluxStageIter;}
    virtual lm::protowrap::FFluxStageOutput* mutableCurrentStageOutput() {return &currentFFluxStageOutput;}

    // setters to help with shadowing pointers in the base class
    virtual void setInput(lm::input::Input* newInput);
    virtual void setTrajectoryList(lm::trajectory::TrajectoryList* newTrajectoryList);

    // deprecated
//    virtual void finishSimulation();
//    virtual void receivedProcessWorkUnitOutput(lm::message::Message& msg);
//    virtual void receivedStartedOutputWriter(const lm::message::StartedOutputWriter& msg);

protected:
    lm::fflux::input::FFluxStageList ffluxStageList;
    FFluxStageVector ffluxStageExecutionOrder;
    FFluxStageVector::iterator currentFFluxStageIter;
    FFluxPhases::iterator currentFFluxPhaseIter;

    lm::limit::TrajectoryLimits trajectoryLimits;
    lm::tiling::Tiling* currentTilingPtr;

    FFluxPhaseOutputs ffluxPhaseOutputs;
    lm::protowrap::FFluxPhaseOutput _ffluxPhaseOutput_0;
    lm::protowrap::FFluxPhaseOutput _ffluxPhaseOutput_1;
    lm::protowrap::FFluxPhaseOutput* currentFFluxPhaseOutputPtr;
    lm::protowrap::FFluxPhaseOutput* previousFFluxPhaseOutputPtr;

    FFluxStageOutputs ffluxStageOutputs;
    lm::protowrap::FFluxStageOutput currentFFluxStageOutput;

    // shadowing ptrs from the base class
    lm::fflux::input::FFluxInput* input;
    lm::fflux::FFluxTrajectoryList* trajectoryList;
};

}
}

#endif /* FFLUXSUPERVISOR_H_ */
