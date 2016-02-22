/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *                  Johns Hopkins University
 *                  http://biophysics.jhu.edu/roberts/
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
#ifndef LM_FFLUX_FFLUXTRAJECTORY_H_
#define LM_FFLUX_FFLUXTRAJECTORY_H_

#include "lm/Types.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/input/Input.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/tiling/Tilings.h"

namespace lm {
namespace fflux {

class FFluxTrajectory : public lm::trajectory::Trajectory
{
public:
    FFluxTrajectory(uint64_t id,uint ffluxPhase,const lm::input::Input& input,bool reversed=false);
    FFluxTrajectory(uint64_t id,uint ffluxPhase,const lm::input::Input& input,lm::io::TrajectoryState* state);
    virtual ~FFluxTrajectory();
    //virtual void initZerothTrajectory();
    virtual void initLimits();

    // methods for detecting when a flux event has occurred
    virtual bool fluxedBackward();
    virtual bool fluxedForward();

    // accessors
    virtual uint getFFluxPhase();
    virtual lm::io::TrajectoryLimits::LimitType getFinalLimitType();
    virtual double getLastLimitTime();
    virtual void getLastSpeciesCounts(lm::io::FFluxOutput::TrajectoryOutput* trajectoryOutputBuf);
    virtual bool hasElapsed(double time);

    // mutators
    virtual void setLastLimitTime(double lLT);

    //    uint getFFluxPhase() {return ffluxPhase;}
    //    void setFFluxPhase(uint newPhase) {ffluxPhase = newPhase;}

protected:
    const lm::input::Input& input;
    uint ffluxPhase;
    // the simulation time when the trajectory last hit a limit (or 0.0)
    double lastLimitTime;
};

}
}

#endif
