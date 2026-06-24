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
#ifndef LM_FFLUX_FFLUXSUPERVISOR_H_
#define LM_FFLUX_FFLUXSUPERVISOR_H_

#include <valarray>

#include "hrtime.h"
#include "lm/EnumHelper.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/fflux/input/FFluxPhase.pb.h"
#include "lm/fflux/input/FFluxStage.pb.h"
#include "lm/fflux/io/FFluxPhaseOutput.pb.h"
#include "lm/fflux/io/FFluxPhaseOutputWrap.h"
#include "lm/fflux/io/FFluxStageOutput.pb.h"
#include "lm/fflux/io/FFluxStageOutputWrap.h"
#include "lm/fflux/input/FFluxInput.h"
#include "lm/input/Options.pb.h"
#include "lm/input/OutputOptions.pb.h"
#include "lm/Iterator.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Message.pb.h"
#include "lm/protowrap/Repeated.h"
#include "lm/trajectory/TrajectoryList.h"

namespace lm {
namespace fflux {

class FFluxSupervisor : public lm::main::SimulationSupervisor
{
public:
    typedef lm::protowrap::Repeated<lm::fflux::input::FFluxPhase> FFluxPhasesWrap;
    typedef lm::protowrap::Repeated<lm::fflux::input::FFluxPhaseLimit> FFluxPhaseLimitsWrap;
    typedef lm::protowrap::Repeated<lm::fflux::io::FFluxPhaseOutput> FFluxPhaseOutputsWrap;
    typedef lm::protowrap::Repeated<lm::fflux::io::FFluxPhaseOutputList> FFluxPhaseOutputListsWrap;
    typedef lm::protowrap::Repeated<lm::fflux::io::FFluxStageOutput> FFluxStageOutputsWrap;
    typedef std::vector<lm::fflux::input::FFluxStage*> FFluxStageVector;
    typedef lm::protowrap::Repeated<lm::fflux::input::FFluxStage> FFluxStagesWrap;

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
    virtual void initSimulationStageListCustom();
    virtual void sanityCheckInput();
    virtual lm::fflux::input::FFluxStage* buildProductionStage(lm::fflux::input::FFluxStage* productionStage, uint64_t replicateID, const lm::tiling::Tiling& tiling, int64_t basinIndex);
    virtual void addTiling(lm::fflux::input::FFluxStage* stage, const lm::tiling::Tiling& tiling, int64_t basinIndex);
    virtual lm::fflux::input::FFluxStage* addPilotStage(lm::fflux::input::FFluxStage* productionStage);
    virtual void addFFluxPhases(lm::fflux::input::FFluxStage* stage, FFPhaseEnums::TrajectoryGeneration trajGeneration, FFPhaseEnums::TrajectoryDuplication trajDuplication);
    virtual void addOptions(lm::fflux::input::FFluxPhase* phase, const lm::fflux::input::FFluxStage& stage);

    // setup methods that run at the start of every fflux stage
    virtual void startSimulationStage();
    virtual void addFFluxStageOutput();
    template <typename Value> lm::fflux::input::FFluxPhaseLimit* buildFFluxPhaseLimit(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, const lm::fflux::input::FFluxPhase& ffluxPhase, FFPhaseLimEnums::StopCondition stopCondition, Value value);
    template <typename Value> void addFFluxPhaseLimitsForPilotStage(lm::fflux::input::FFluxStage* stage, FFPhaseLimEnums::StopCondition stopCondition, Value phaseZeroValue, Value value);
    // TODO: spin this function off as part of an FFluxPhase wrapper
    static void buildFFluxPhaseLimitEventsPerTrajectory(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, const lm::fflux::input::FFluxPhase& ffluxPhase, uint simultaneousWorkUnits);
    static void buildFFluxPhaseLimitTrajectoriesToRun(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, const lm::fflux::input::FFluxPhase& ffluxPhase, uint simultaneousWorkUnits);
    void repeatFFluxPhaseLimits(lm::fflux::input::FFluxStage* stage, const lm::fflux::input::FFluxPhaseLimit& limitToRepeat);
//    template <typename ValueT> void repeatFFluxPhaseLimits(lm::protowrap::Repeated<lm::fflux::input::FFluxPhaseLimit>::iterator begin, const lm::fflux::input::FFluxPhaseLimit& limitToRepeat);
    virtual void addFFluxPhaseLimitsFromInput(lm::fflux::input::FFluxStage* productionStage);
    virtual void addFFluxPhaseLimitsFromStageOutput(lm::fflux::input::FFluxStage* productionStage, const lm::protowrap::FFluxStageOutputWrap& stageOutput);

    // the functions where all the computational cost minimization magic happens
    inline static std::vector<double> estimateBernoulliProbabilities(const lm::protowrap::FFluxStageOutputWrap& stageOutput, double confidence=.99, double minimum=1e-4);
    inline static std::vector<uint64_t> optimizeTrajectoryCounts(double errorGoal, double errorGoalConfidence, const lm::protowrap::FFluxStageOutputWrap& stageOutput, uint64_t minimumCount, double phaseZeroSamplingMultiplier, bool minimizeCost);
    inline static std::vector<uint64_t> minimizeCostTrajectoryCounts(double errorGoal, double errorGoalConfidence, const std::vector<double>& weights, const std::vector<double>& variances, const std::vector<double>& costs);
    inline static std::vector<uint64_t> minimizeCountTrajectoryCounts(double errorGoal, double errorGoalConfidence, const std::vector<double>& weights, const std::vector<double>& variances);
    inline static std::valarray<double> getConstantFactors(const std::vector<double>& weights, const std::vector<double>& variances);

    // TODO: refactor the static functions used to implement the optimization equation to allow for easier unit testing. Below is a first pass at new function headers
//    // the functions where all the computational cost minimization magic happens
//    // static encapsulations of parts of the optimization routine. Broken out this way for unit testing purposes
//    inline static std::vector<double> estimateBernoulliProbabilities(const lm::protowrap::FFluxStageOutputWrap& stageOutput, double confidence=.9999);
//    inline static std::vector<uint64_t> minimizeTrajectoryCounts(double errorGoal, double errorGoalConfidence, const vector<uint64_t>& floorCounts, const vector<uint64_t>& countMultipliers, const vector<double>& probabilities);
//    inline static std::vector<uint64_t> minimizeTrajectoryCounts(double errorGoal, double errorGoalConfidence, const vector<uint64_t>& floorCounts, const vector<uint64_t>& countMultipliers, const vector<double>& probabilities, const vector<double>& costs);
//    inline static std::vector<uint64_t> _minimizeTrajectoryCounts(double errorGoal, double errorGoalConfidence, const std::vector<double>& probabilities, const std::vector<double>& costs);
//    inline static std::vector<uint64_t> _minimizeTrajectoryCounts(double errorGoal, double errorGoalConfidence, const std::vector<double>& probabilities);
//    inline static std::vector<uint64_t> optimizationEquation(double errorGoal, double errorGoalConfidence, const std::vector<double>& gFactors, const std::vector<double>& costs);
//    inline static std::valarray<double> getConstantFactors(const std::vector<double>& probabilities);

    // setup methods that run at the start of every fflux phase
    virtual void startSimulationPhase();
    virtual void addFFluxPhaseOutput();
    virtual void buildTrajectoryList();

    // methods that control what happens at the end of a ffluxPhase
    virtual bool _terminateSimulationPhase();
    virtual void printFFluxLimitProgress();
    virtual void finishSimulationPhase();
    virtual void sendSimulationPhaseOutput();
    virtual bool incrementSimulationPhase();

    // methods that control what happens at the end of a ffluxStage
    virtual void finishSimulationStage();
    virtual void sendSimulationStageOutput();
    virtual bool incrementSimulationStage();

    // methods that run at the end of the entire simulation
    virtual void finishSimulation();

    // methods that handle setting up RunWorkUnit messages
    virtual const lm::input::Options& getOptions() {return currentPhase().options();}
    virtual const lm::input::OutputOptions& getOutputOptions() {return currentPhase().output_options();}

    // methods that handle FinishedWorkUnit messages
    virtual void receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg);
    virtual void receivedFinishedWorkUnitPart(const lm::message::WorkUnitStatus& wusMsg);
    virtual void receivedFinishedWorkUnitPartPhaseZero(const lm::message::WorkUnitStatus& wusMsg);

    // accessors
    virtual const lm::fflux::input::FFluxPhase& currentPhase() const {return *currentFFluxPhaseIter;}
    virtual int64_t currentFFluxPhaseID() const {return currentPhase().phase_id();}
    virtual std::string phaseInfo(bool path = false, const lm::fflux::input::FFluxPhase* phase = NULL, const lm::fflux::input::FFluxStage* stage = NULL) const;
    virtual const lm::fflux::input::FFluxPhaseLimit& currentPhaseLimit() const {return currentPhase().has_fflux_phase_limit() ? currentPhase().fflux_phase_limit() : currentStage().fflux_phase_limits(currentFFluxPhaseID());}
    virtual const lm::protowrap::FFluxPhaseOutputWrap& currentPhaseOutput() const {return *currentFFluxPhaseOutputWrapPtr;}
    virtual int64_t finalFFluxPhaseID() const {return currentStage().fflux_phases_size() - 1;}
    virtual bool isCurrentPhaseLast() const {return isLast(currentFFluxPhaseIter, currentStage().fflux_phases());} //{return currentStage().fflux_phases().end()==currentFFluxPhaseIter;}
    virtual const lm::protowrap::FFluxPhaseOutputWrap& previousPhaseOutput() const {return *previousFFluxPhaseOutputWrapPtr;}

    virtual const lm::fflux::input::FFluxStage& currentStage() const {return **currentFFluxStageIter;}
    virtual int64_t currentStageIndex() const {return currentFFluxStageIter - ffluxStageExecutionOrder.begin();}
    virtual std::string currentStageInfo(bool path=false, const lm::fflux::input::FFluxStage* stage=NULL) const;
    virtual const lm::protowrap::FFluxStageOutputWrap& currentStageOutput() const {return currentFFluxStageOutputWrap;}
    virtual int getStageCount() const {return ffluxStageExecutionOrder.size();}
    virtual bool isCurrentStageLast() const {return isLast(currentFFluxStageIter, ffluxStageExecutionOrder);}  //{return currentFFluxStageIter==ffluxStageExecutionOrder.end();}

    virtual const lm::tiling::Tiling& currentTiling() const {return currentTilingWrap;}

    virtual std::string stageLogPilot(const lm::protowrap::FFluxStageOutputWrap& stageOutput, double errorGoal, double errorGoalConfidence, vector<uint64_t>& trajectoryCounts) const;
    virtual std::string stageLogProduction(const lm::protowrap::FFluxStageOutputWrap& stageOutput) const;

    // mutators
    virtual lm::fflux::input::FFluxPhase* mutableCurrentPhase() {return &*currentFFluxPhaseIter;}
    virtual lm::fflux::input::FFluxPhaseLimit* mutableCurrentPhaseLimit() {return currentPhase().has_fflux_phase_limit() ? mutableCurrentPhase()->mutable_fflux_phase_limit() : mutableCurrentStage()->mutable_fflux_phase_limits(currentFFluxPhaseID());}
    virtual lm::protowrap::FFluxPhaseOutputWrap* mutableCurrentPhaseOutput() {return currentFFluxPhaseOutputWrapPtr;}

    virtual lm::fflux::input::FFluxStage* mutableCurrentStage() {return *currentFFluxStageIter;}
    virtual lm::protowrap::FFluxStageOutputWrap* mutableCurrentStageOutput() {return &currentFFluxStageOutputWrap;}

    virtual lm::tiling::Tiling* mutableCurrentTiling() {return &currentTilingWrap;}
    virtual void setCurrentTiling(lm::input::Tiling* newCurrentTilingMsg) {currentTilingWrap.init(newCurrentTilingMsg, input->getOrderParameters());}

    // setters/destructors to help with shadowing pointers in the base class
    virtual void setInput(lm::input::Input* newInput);
    virtual void setTrajectoryList(lm::trajectory::TrajectoryList* newTrajectoryList);
    virtual void destructInput() {lm::main::SimulationSupervisor::destructInput(); input = NULL;}
    virtual void destructTrajectoryList() {lm::main::SimulationSupervisor::destructTrajectoryList(); trajectoryList = NULL;}

protected:
    lm::fflux::input::FFluxStageList ffluxStageListMsg;
    FFluxStageVector ffluxStageExecutionOrder;
    FFluxStageVector::iterator currentFFluxStageIter;
    FFluxPhasesWrap::iterator currentFFluxPhaseIter;

    lm::tiling::Tiling currentTilingWrap;

    // phase output messages
    FFluxPhaseOutputListsWrap::WrappedField ffluxPhaseOutputListsMsg;
    FFluxPhaseOutputListsWrap ffluxPhaseOutputListsWrap;
    FFluxPhaseOutputsWrap currentFFluxPhaseOutputsWrap;

    lm::message::Message ffluxPhaseOutputContainingMsg;
    lm::protowrap::FFluxPhaseOutputWrap _ffluxPhaseOutputWrap_0;
    lm::protowrap::FFluxPhaseOutputWrap _ffluxPhaseOutputWrap_1;
    lm::protowrap::FFluxPhaseOutputWrap* previousFFluxPhaseOutputWrapPtr;
    lm::protowrap::FFluxPhaseOutputWrap* currentFFluxPhaseOutputWrapPtr;

    // stage output messages
    lm::message::Message ffluxStageOutputRawContainingMsg;
    lm::message::Message ffluxStageOutputSummaryContainingMsg;
    FFluxStageOutputsWrap::WrappedField ffluxStageOutputsMsg;
    FFluxStageOutputsWrap ffluxStageOutputsWrap;
    lm::protowrap::FFluxStageOutputWrap currentFFluxStageOutputWrap;

    // flag that indicates that a phase has ended
    bool ffluxPhaseTerminated;

    /*
     * - flags that prevent output from being sent multiple times
     *     - this is important when the supervisor has to cycle through the finishSimulationPhase() and finishSimulationStage multiple times, as at program end
     */
    bool simulationPhaseOutputSent;
    bool simulationStageOutputSent;

    // shadowing ptrs from the base class
    lm::fflux::input::FFluxInput* input;
    lm::fflux::FFluxTrajectoryList* trajectoryList;

    hrtime ffluxProgress_lastPrintTime;
};

}
}

#endif /* LM_FFLUX_FFLUXSUPERVISOR_H_ */
