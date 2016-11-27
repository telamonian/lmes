/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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
 * Author(s): Elijah Roberts
 */

#ifndef LM_ME_MESOLVER_H
#define LM_ME_MESOLVER_H

#include <map>
#include <string>
#include <vector>

#include "lm/Types.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/OutputOptions.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Communicator.h"
#include "lm/message/Endpoint.pb.h"
#include "lm/message/RunWorkUnit.pb.h"

using std::map;
using std::string;
using std::vector;

namespace lm {

namespace me {

class MESolver
{
public:
    MESolver();
    virtual ~MESolver();
    virtual void setComputeResources(vector<int> cpus, vector<int> gpus);
    virtual uint getSimultaneousTrajectories();
    virtual void setCommunicator(lm::message::Communicator* communicator, lm::message::Endpoint outputAddress, int64_t workUnitId);
    virtual bool needsReactionModel()=0;
    virtual void setReactionModel(const lm::io::ReactionModel& rm)=0;
    virtual bool needsDiffusionModel()=0;
    virtual void setDiffusionModel(const lm::io::DiffusionModel& dm)=0;
    virtual void setOrderParameters(const lm::io::OrderParameters& ops)=0;
    virtual void setTilings(const lm::io::Tilings& tilings)=0;
    virtual void setLimits(const lm::io::TrajectoryLimits& limits)=0;
    virtual void setOutputOptions(const lm::io::OutputOptions& outputOptions)=0;
    virtual void reset();
    virtual void getState(lm::io::TrajectoryState* state, uint trajectoryNumber=0)=0;
    virtual void setState(const lm::io::TrajectoryState& state, uint trajectoryNumber=0)=0;
    virtual long long generateTrajectory(long long maxSteps)=0;
    virtual lm::message::WorkUnitStatus::Status getStatus(uint trajectoryNumber=0)=0;

protected:
    virtual bool isTrajectoryOutsideLimits()=0;

protected:
    vector<int> cpus;
    vector<int> gpus;
    lm::message::Communicator* communicator;
    lm::message::Endpoint outputAddress;
    int64_t workUnitId;
};

}
}

#endif
