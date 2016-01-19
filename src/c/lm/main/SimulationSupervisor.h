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

#include <map>
#include <string>

#include <google/protobuf/message.h>

#include "hrtime.h"
#include "lm/Exceptions.h"
#include "lm/input/Input.h"
#include "lm/io/BoundaryConditions.pb.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/io/Tilings.pb.h"
#include "lm/message/Communicator.h"
#include "lm/message/FinishedCheckpointing.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedCheckpointSignaler.pb.h"
#include "lm/message/StartedOutputWriter.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/resource/ResourceMap.h"
#include "lm/slot/SlotList.h"
#include "lm/trajectory/TrajectoryList.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lm/tiling/Tilings.h"

using std::map;
using std::string;

namespace lm {
namespace main {

typedef map<string,string> SimulationParametersMap;

class SimulationSupervisor : public lm::thread::Worker
{
public:
    static const int THREAD_ID = 0;

public:
    SimulationSupervisor();
    virtual ~SimulationSupervisor();
    virtual void init();
    void setOutputWriterClassName(string outputWriterClassName) {this->outputWriterClassName = outputWriterClassName;}
    void setResourceMap(lm::resource::ResourceMap* resourceMap) {this->resourceMap = resourceMap;}
    void setSimulationFilename(string simulationInputFilename, string simulationOutputFilename) {this->simulationInputFilename = simulationInputFilename; this->simulationOutputFilename = simulationOutputFilename;}
    void setSolverClassName(string solverClassName) {this->solverClassName = solverClassName;}
    void setUseCPUAffinity(bool useCPUAffinity) {this->useCPUAffinity = useCPUAffinity;}
    void wake() throw(lm::thread::PthreadException);

protected:
//    virtual void initLimits();
    virtual void startSimulation();
    virtual bool assignWork();
    virtual void finishSimulation();

    virtual int run();
    virtual void resourceAvailable(const lm::message::ResourcesAvailable& msg);
    virtual void allResourcesRegistered();
    virtual void startOutputWriter();
    virtual void receivedStartedOutputWriter(const lm::message::StartedOutputWriter& msg);
    virtual void startCheckpointSignaler();
    virtual void receivedStartedCheckpointSignaler(const lm::message::StartedCheckpointSignaler& msg);
    virtual void startWorkUnitRunners();
    virtual void receivedStartedWorkUnitRunner(const lm::message::StartedWorkUnitRunner & msg);
    virtual void startSimulationIfAllWorkersStarted();
    virtual void receivedStartedWorkUnit(const lm::message::StartedWorkUnit& msg);
    virtual void receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg);
    virtual void receivedPerformCheckpointing(const lm::message::PerformCheckpointing& msg);
    virtual void receivedFinishedCheckpointing(const lm::message::FinishedCheckpointing& msg);

private:
    bool parseBoundaryConditions(lm::io::BoundaryConditions* bc, std::string arg);
    void resetPerformanceStatistics();
    void printPerformanceStatistics(bool flush=false);

protected:
    bool simulationRunning;
    bool performingCheckpoint;
    lm::message::Communicator communicator;
    lm::resource::ResourceMap* resourceMap;
    std::string simulationInputFilename;
    std::string simulationOutputFilename;
    std::string outputWriterClassName;
    bool hasOutputWriterStarted;
    int outputWriterProcess;
    int outputWriterThread;
    bool hasCheckpointSignalerStarted;
    std::string solverClassName;
    bool useCPUAffinity;
    lm::input::Input* input;
    lm::io::SimulationParameters simulationParametersBuf;
    map<string,string> simulationParametersMap;
    bool hasReactionModel;
    lm::io::ReactionModel reactionModelBuf;
    bool hasDiffusionModel;
    lm::io::DiffusionModel diffusionModelBuf;
    bool hasOrderParameters;
    lm::io::OrderParameters orderParametersBuf;
    lm::oparam::OParams ops;
    bool hasTilings;
    lm::io::Tilings tilingsBuf;
    lm::tiling::Tilings tilings;
    lm::trajectory::TrajectoryList* trajectoryList;
    lm::slot::SlotList slots;
    bool haveAllWorkUnitRunnersStarted;
    long long workUnitCount;

private:
    hrtime stats_lastPrintTime;
    long long stats_workUnits;
    long long stats_minWorkUnitId;
    long long stats_maxWorkUnitId;
    long long stats_workUnitsSteps;
    double stats_workUnitTime;
};

}
}

#endif // LM_MAIN_SUPERVISOR_H
