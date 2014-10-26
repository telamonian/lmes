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

#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/resource/Trajectory.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

namespace lm {
namespace fflux {

class FFluxTrajectory : public lm::resource::Trajectory
{
public:
    FFluxTrajectory(uint64_t id,uint ffluxPhase,const lm::io::ReactionModel& reactionModel,const lm::io::DiffusionModel& diffusionModel,std::map<std::string,std::string>& simulationParameters,lm::tiling::Tilings& tilings);
    FFluxTrajectory(uint64_t id,uint ffluxPhase,const lm::io::ReactionModel& reactionModel,const lm::io::DiffusionModel& diffusionModel,std::map<std::string,std::string>& simulationParameters,lm::tiling::Tilings& tilings,const lm::io::TrajectoryState& state);
    virtual ~FFluxTrajectory();

    virtual void initZerothTrajectory();

    // methods for detecting when a flux event has occurred
    virtual bool fluxedBackward();
    virtual bool fluxedForward();

    // accessors
    virtual lm::io::TrajectoryLimits::LimitType getFinalLimitType();
    virtual uint getSimSteps();
    virtual double getSimTime();
    virtual bool hasElapsed(double time);

    // methods for dealing with limits and the underlying tiling
    virtual void setLimits();

    //    uint getFFluxPhase() {return ffluxPhase;}
    //    void setFFluxPhase(uint newPhase) {ffluxPhase = newPhase;}

protected:
    // reference to supervisor's Tilings object for setting limits and such
    lm::tiling::Tilings& tilings;

public:
    uint ffluxPhase;
};

}
}

#endif
