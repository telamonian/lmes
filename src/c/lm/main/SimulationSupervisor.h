/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *              Johns Hopkins University
 *              http://biophysics.jhu.edu/roberts/
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
#ifndef LM_MAIN_SUPERVISOR_H
#define LM_MAIN_SUPERVISOR_H

#include <google/protobuf/message.h>
#include <map>
#include <string>

#include "hrtime.h"
#include "lm/Exceptions.h"
#include "lm/input/Input.h"
#include "lm/types/BoundaryConditions.pb.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/input/OrderParameters.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/Tilings.pb.h"
#include "lm/message/Communicator.h"
#include "lm/message/FinishedCheckpointing.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedCheckpointSignaler.pb.h"
#include "lm/message/StartedOutputWriter.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"
#include "lm/message/WorkUnit.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/resource/ResourceMap.h"
#include "lm/slot/SlotList.h"
#include "lm/trajectory/TrajectoryList.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lm/tiling/Tilings.h"

namespace lm {
namespace main {

typedef std::map<std::string,std::string> SimulationParametersMap;

class SimulationSupervisor : public lm::thread::Worker
{
public:
    static const int THREAD_ID = 0;
    virtual int getRecvSleepMilliseconds();

public:
    SimulationSupervisor();
    virtual ~SimulationSupervisor();
    virtual void init();
    void setOutputWriterClassName(string outputWriterClassName) {this->outputWriterClassName = outputWriterClassName;}
    void setResourceMap(lm::resource::ResourceMap* resourceMap) {this->resourceMap = resourceMap;}
    void setSimulationFilename(std::vector<std::string> simulationInputFilenames, std::string simulationOutputFilename) {this->simulationInputFilenames = simulationInputFilenames; this->simulationOutputFilename = simulationOutputFilename;}
    void setSolverClassName(std::string solverClassName) {this->solverClassName = solverClassName;}
    void setUseCPUAffinity(bool useCPUAffinity) {this->useCPUAffinity = useCPUAffinity;}
    void wake() throw(lm::thread::PthreadException);

protected:
    virtual int run();

// the functions below are listed in (very) roughly the order they are first called during simulation execution
    // resource setup
    virtual void receivedResourceAvailable(const lm::message::ResourcesAvailable& msg);
    virtual void allResourcesRegistered();

    // output setup
    virtual void startOutputWriter();

    // checkpointing setup
    virtual void startCheckpointSignaler();

    // work unit runner setup
    virtual void startWorkUnitRunners();
    virtual void receivedStartedOutputWriter(const lm::message::StartedOutputWriter& msg);
    virtual void receivedStartedCheckpointSignaler(const lm::message::StartedCheckpointSignaler& msg);
    virtual void receivedStartedWorkUnitRunner(const lm::message::StartedWorkUnitRunner & msg);

    // simulation kickoff
    virtual void startSimulationIfAllWorkersStarted();
    virtual void startSimulation();

    // simulation phase setup
    virtual void startSimulationPhase();
    virtual void buildTrajectoryList()=0;

    // work unit setup/finalization
    virtual void receivedStartedWorkUnit(const lm::message::StartedWorkUnit& msg);
    virtual void receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg);
    virtual bool assignWork();
    virtual const lm::input::Options& getOptions() {return input->getOptions();}
    virtual const lm::input::OutputOptions& getOutputOptions() {return input->getOutputOptionsMsg();}
    virtual void buildRunWorkUnitHeader(lm::message::RunWorkUnit* msg);
    virtual void buildRunWorkUnitParts(lm::message::RunWorkUnit* msg, uint minWorkUnits);

    // simulation phase finalization
    virtual bool _terminateSimulationPhase();
    virtual bool terminateSimulationPhase();
    virtual void finishSimulationPhase();
    virtual bool performAnotherSimulationPhase();
    virtual void incrementSimulationPhase();

    // simulation finalization
    virtual void finishSimulation();

    // msg handling
    virtual void receivedPerformCheckpointing(const lm::message::PerformCheckpointing& msg);
    virtual void receivedFinishedCheckpointing(const lm::message::FinishedCheckpointing& msg);
    virtual void receivedProcessWorkUnitOutput(lm::message::Message& msg);
    virtual bool receivedOther(lm::message::Message& msg);

    // setters/destructors for attributes that may be shadowed by derived class attributes
    virtual void setInput(lm::input::Input* newInput);
    virtual void setTrajectoryList(lm::trajectory::TrajectoryList* newTrajectoryList);
    virtual void destructInput() {if (input != NULL) delete input; input = NULL;}
    virtual void destructTrajectoryList() {if (trajectoryList != NULL) delete trajectoryList; trajectoryList = NULL;}

    // other
    virtual double timeElapsed() {return convertHrToSeconds(getHrTime() - simulationStartTime);}

private:
    void printPerformanceStatistics(bool flush=false);
    void resetPerformanceStatistics();

protected:
    lm::message::Communicator communicator;
    bool hasCheckpointSignalerStarted;
    bool hasOutputWriterStarted;
    bool haveAllWorkUnitRunnersStarted;
    lm::input::Input* input;
    std::string outputWriterClassName;
    int outputWriterProcess;
    int outputWriterThread;
    bool performingCheckpoint;
    lm::resource::ResourceMap* resourceMap;
    std::vector<std::string> simulationInputFilenames;
    std::string simulationOutputFilename;
    int64_t simulationPhaseID;
    bool simulationRunning;
    bool simulationPhaseAborted;
    lm::slot::SlotList slots;
    std::string solverClassName;
    lm::trajectory::TrajectoryList* trajectoryList;
    bool useCPUAffinity;
    long long workUnitCount;

protected:
    hrtime simulationStartTime;

    hrtime stats_lastPrintTime;
    long long stats_workUnits;
    long long stats_workUnitsParts;
    long long stats_minWorkUnitId;
    long long stats_maxWorkUnitId;
    long long stats_workUnitsSteps;
    double stats_workUnitTime;
};

}
}

#endif // LM_MAIN_SUPERVISOR_H
