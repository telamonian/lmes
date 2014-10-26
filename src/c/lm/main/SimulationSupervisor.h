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

#include "lm/Exceptions.h"
#include "lm/io/BoundaryConditions.pb.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/io/Tilings.pb.h"
#include "lm/message/Communicator.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedOutputWriter.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"
#include "lm/resource/ResourceMap.h"
#include "lm/resource/SlotList.h"
#include "lm/resource/TrajectoryList.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lm/tiling/Tilings.h"

using std::map;
using std::string;

namespace lm {
namespace main {

class SimulationSupervisor : public lm::thread::Worker
{
public:
    static const int THREAD_ID = 0;

public:
    SimulationSupervisor();
    virtual ~SimulationSupervisor();
    void setOutputWriterClassName(string outputWriterClassName) {this->outputWriterClassName = outputWriterClassName;}
    void setResourceMap(lm::resource::ResourceMap* resourceMap) {this->resourceMap = resourceMap;}
    void setSimulationFilename(string simulationInputFilename, string simulationOutputFilename) {this->simulationInputFilename = simulationInputFilename; this->simulationOutputFilename = simulationOutputFilename;}
    void setSolverClassName(string solverClassName) {this->solverClassName = solverClassName;}
    void setUseCPUAffinity(bool useCPUAffinity) {this->useCPUAffinity = useCPUAffinity;}
    virtual void initialize();
    void wake() throw(lm::thread::PthreadException);

protected:
    virtual void initLimits();
    virtual void startSimulation();
    virtual bool assignWork();
    virtual void workUnitStarted(const lm::message::StartedWorkUnit& msg);
    virtual void workUnitFinished(const lm::message::FinishedWorkUnit& msg);
    virtual void outputWriterStarted(const lm::message::StartedOutputWriter& msg)=0;
    virtual void finishSimulation();

    virtual int run();
    virtual void resourceAvailable(const lm::message::ResourcesAvailable& msg);
    virtual void markWorkUnitRunnerStarted(const lm::message::StartedWorkUnitRunner & msg);
    virtual void allResourcesRegistered();
    virtual void allWorkUnitRunnersStarted();


protected:
    long long workUnitCount;
    lm::resource::TrajectoryList* trajectories;
    lm::message::Communicator communicator;
    lm::resource::ResourceMap* resourceMap;
    std::string simulationInputFilename;
    std::string simulationOutputFilename;
    std::string outputWriterClassName;
    std::string solverClassName;
    bool useCPUAffinity;
    lm::io::SimulationParameters simulationParameters;
    map<string,string> simulationParameterMap;
    bool hasReactionModel;
    lm::io::ReactionModel reactionModel;
    bool hasDiffusionModel;
    lm::io::DiffusionModel diffusionModel;
//    bool hasFFluxParameters;
//    lm::io::FFluxParameters ffluxParameters;
    bool hasOrderParameters;
    lm::io::OrderParameters orderParameters;
    bool hasTilings;
    lm::io::Tilings tilingsBuf;
    lm::tiling::Tilings tilings;
    lm::resource::SlotList slots;

private:
    bool parseBoundaryConditions(lm::io::BoundaryConditions* bc, std::string arg);
};

}
}

#endif // LM_MAIN_SUPERVISOR_H
