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

#ifndef LM_MAIN_SOLVER_H
#define LM_MAIN_SOLVER_H

#include <string>
#include <vector>

#include "lm/Types.h"
#include "lm/input/OutputOptions.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"

using std::string;
using std::vector;

namespace lm {
namespace main {

class Solver
{
public:
    Solver();
    virtual ~Solver();
    virtual uint getSimultaneousTrajectories();
    virtual void setComputeResources(vector<int> cpus, vector<int> gpus);
    virtual void reset();
    virtual void setLimits(const lm::io::TrajectoryLimits& limits)=0;
    virtual void setOutputOptions(const lm::input::OutputOptions& outputOptions)=0;
    virtual void setState(const lm::io::TrajectoryState& state, uint trajectoryNumber=0)=0;
    virtual void getState(lm::io::TrajectoryState* state, uint trajectoryNumber=0)=0;
    virtual lm::message::WorkUnitOutput* getOutput(uint trajectoryNumber=0)=0;
    virtual lm::message::WorkUnitStatus::Status getStatus(uint trajectoryNumber=0)=0;
    virtual uint64_t generateTrajectory(uint64_t maxSteps)=0;

protected:
    bool isTrajectoryOutsideLimits();

protected:
    vector<int> cpus;
    vector<int> gpus;
};

}
}

#endif
