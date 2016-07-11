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
#include "lm/Iterator.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/protowrap/FFluxPhaseOutputWrap.h"
#include "lm/protowrap/FFluxStageOutputWrap.h"
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
    virtual lm::fflux::input::FFluxStage* buildProductionStage(lm::fflux::input::FFluxStage* productionStage, const lm::tiling::Tiling& tiling, int basinIndex);
    virtual lm::fflux::input::FFluxStage* addPilotStage(lm::fflux::input::FFluxStage* productionStage);
    virtual void addFFluxPhases(lm::fflux::input::FFluxStage* stage, FFPhaseEnums::TrajectoryGeneration trajGeneration, FFPhaseEnums::TrajectoryDuplication trajDuplication);

    // setup methods that run at the start of every fflux stage
    virtual void startSimulationStage();
    template <typename ValT> lm::fflux::input::FFluxPhaseLimit* buildFFluxPhaseLimit(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, FFPhaseLimEnums::StopCondition stopCondition, ValT value);
    template <typename ValT> void addFFluxPhaseLimits(lm::fflux::input::FFluxStage* stage, FFPhaseLimEnums::StopCondition stopCondition, ValT value);
    // TODO: spin this function off as part of an FFluxPhase wrapper
    static void buildFFluxPhaseLimitTrajectoriesToRun(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, const lm::fflux::input::FFluxPhase& ffluxPhase, uint simultaneousWorkUnits);
    void repeatFFluxPhaseLimits(lm::fflux::input::FFluxStage* stage, const lm::fflux::input::FFluxPhaseLimit& limitToRepeat);
//    template <typename ValueT> void repeatFFluxPhaseLimits(lm::protowrap::Repeated<lm::fflux::input::FFluxPhaseLimit>::iterator begin, const lm::fflux::input::FFluxPhaseLimit& limitToRepeat);
    virtual void addFFluxPhaseLimitsFromInput(lm::fflux::input::FFluxStage* productionStage);
    virtual void addFFluxPhaseLimitsFromStageOutput(lm::fflux::input::FFluxStage* productionStage, const lm::protowrap::FFluxStageOutputWrap& stageOutput, bool minimizeCost = true);

    // the functions where all the computational cost minimization magic happens
    inline static std::vector<uint64_t> optimizeTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const lm::protowrap::FFluxStageOutputWrap& stageOutput, bool minimizeCost = true);
    inline static std::vector<uint64_t> minimizeCostTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const std::vector<double>& probabilities, const std::vector<double>& costs);
    inline static std::vector<uint64_t> minimizeCountTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const std::vector<double>& probabilities);
    inline static std::valarray<double> getConstantFactors(const std::vector<double>& probabilities);

    // setup methods that run at the start of every fflux phase
    virtual void startSimulationPhase();
    virtual void buildTrajectoryList();

    // methods that control what happens at the end of a ffluxPhase
    virtual bool terminateSimulationPhase();
    virtual void finishSimulationPhase();
    virtual bool performAnotherSimulationPhase() {return not isCurrentPhaseLast();}
    virtual void incrementSimulationPhase();
    virtual void addFFluxPhaseOutput();

    // methods that control what happens at the end of a ffluxStage
    virtual void finishSimulationStage();
    virtual bool performAnotherSimulationStage() {return not isCurrentStageLast();}
    virtual void incrementSimulationStage();
    virtual void addFFluxStageOutput();

    // methods that handle FinishedWorkUnit messages
    virtual void receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg);
    virtual void receivedFinishedWorkUnitPart(const lm::message::WorkUnitStatus& wusMsg);
    virtual void receivedFinishedWorkUnitPartPhaseZero(const lm::message::WorkUnitStatus& wusMsg);

    // accessors
    // figure out how many flux events we need to observe per trajectory. Useful only during phase zero
//    virtual uint requiredFluxesPerTrajectory() const {return (uint)(ceil((double)(currentPhaseLimit().uvalue())/lm::fflux::FFluxTrajectoryList::getTrajectoriesToStart(currentPhase(), currentPhaseLimit(), slots.getSimultaneousWorkUnits())));}

    virtual const lm::fflux::input::FFluxPhase& currentPhase() const {return *currentFFluxPhaseIter;}
    virtual int64_t currentFFluxPhaseIndex() const {return currentPhase().fflux_phase_index();}
    virtual const lm::fflux::input::FFluxPhaseLimit& currentPhaseLimit() const {return currentStage().fflux_phase_limits(currentFFluxPhaseIndex());}
    virtual const lm::protowrap::FFluxPhaseOutputWrap& currentPhaseOutput() const {return *currentFFluxPhaseOutputWrapPtr;}
    virtual int64_t finalFFluxPhaseIndex() const {return currentStage().fflux_phases_size() - 1;}
    virtual bool isCurrentPhaseLast() const {return is_last(currentFFluxPhaseIter, currentStage().fflux_phases());} //{return currentStage().fflux_phases().end()==currentFFluxPhaseIter;}
    virtual const lm::protowrap::FFluxPhaseOutputWrap& previousPhaseOutput() const {return *previousFFluxPhaseOutputWrapPtr;}

    virtual const lm::fflux::input::FFluxStage& currentStage() const {return **currentFFluxStageIter;}
    virtual int64_t currentStageIndex() const {return currentFFluxStageIter - ffluxStageExecutionOrder.begin();}
    virtual const lm::protowrap::FFluxStageOutputWrap& currentStageOutput() const {return currentFFluxStageOutputWrap;}
    virtual int getStageCount() const {return ffluxStageExecutionOrder.size();}
    virtual bool isCurrentStageLast() const {return is_last(currentFFluxStageIter, ffluxStageExecutionOrder);}  //{return currentFFluxStageIter==ffluxStageExecutionOrder.end();}

    virtual const lm::tiling::Tiling& currentTiling() const {return currentTilingWrap;}

    // mutators
    virtual lm::fflux::input::FFluxPhase* mutableCurrentPhase() {return &*currentFFluxPhaseIter;}
    virtual lm::fflux::input::FFluxPhaseLimit* mutableCurrentPhaseLimit() {return mutableCurrentStage()->mutable_fflux_phase_limits(currentFFluxPhaseIndex());}
    virtual lm::protowrap::FFluxPhaseOutputWrap* mutableCurrentPhaseOutput() {return currentFFluxPhaseOutputWrapPtr;}

    virtual lm::fflux::input::FFluxStage* mutableCurrentStage() {return *currentFFluxStageIter;}
    virtual lm::protowrap::FFluxStageOutputWrap* mutableCurrentStageOutput() {return &currentFFluxStageOutputWrap;}

    virtual lm::tiling::Tiling* mutableCurrentTiling() {return &currentTilingWrap;}
    virtual void setCurrentTiling(lm::input::Tiling* newCurrentTilingMsg) {currentTilingWrap.init(newCurrentTilingMsg, input->getOrderParameters());}

    // setters/destructors to help with shadowing pointers in the base class
    virtual void setInput(lm::input::Input* newInput);
    virtual void setTrajectoryList(lm::trajectory::TrajectoryList* newTrajectoryList);
    virtual void destructInput() {SimulationSupervisor::destructInput(); input = NULL;}
    virtual void destructTrajectory() {SimulationSupervisor::destructTrajectory(); trajectoryList = NULL;}

    // deprecated
//    virtual void finishSimulation();
//    virtual void receivedProcessWorkUnitOutput(lm::message::Message& msg);
//    virtual void receivedStartedOutputWriter(const lm::message::StartedOutputWriter& msg);

protected:
    lm::fflux::input::FFluxStageList ffluxStageListMsg;
    FFluxStageVector ffluxStageExecutionOrder;
    FFluxStageVector::iterator currentFFluxStageIter;
    FFluxPhases::iterator currentFFluxPhaseIter;

    lm::tiling::Tiling currentTilingWrap;

    FFluxPhaseOutputs::WrappedField ffluxPhaseOutputMsgs;
    FFluxPhaseOutputs ffluxPhaseOutputsWrap;
    lm::protowrap::FFluxPhaseOutputWrap _ffluxPhaseOutputWrap_0;
    lm::protowrap::FFluxPhaseOutputWrap _ffluxPhaseOutputWrap_1;
    lm::protowrap::FFluxPhaseOutputWrap* currentFFluxPhaseOutputWrapPtr;
    lm::protowrap::FFluxPhaseOutputWrap* previousFFluxPhaseOutputWrapPtr;

    FFluxStageOutputs::WrappedField ffluxStageOutputMsgs;
    FFluxStageOutputs ffluxStageOutputsWrap;
    lm::protowrap::FFluxStageOutputWrap currentFFluxStageOutputWrap;

    bool simulationPhaseTerminated;

    // shadowing ptrs from the base class
    lm::fflux::input::FFluxInput* input;
    lm::fflux::FFluxTrajectoryList* trajectoryList;
};

}
}

#endif /* FFLUXSUPERVISOR_H_ */
