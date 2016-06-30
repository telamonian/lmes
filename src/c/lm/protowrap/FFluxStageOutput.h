/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
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
#ifndef LM_PROTWRAP_FFLUXSTAGEOUTPUT_H_
#define LM_PROTWRAP_FFLUXSTAGEOUTPUT_H_

#include <algorithm>
#include <limits>
#include <map>
#include <vector>

#include "lm/EnumHelper.h"
#include "lm/fflux/io/FFluxPhaseOutput.pb.h"
#include "lm/fflux/io/FFluxStageOutput.pb.h"
#include "lm/limit/LimitCheckFunctions.h"
#include "lm/io/LimitTracking.pb.h"
#include "lm/protowrap/MessageWrap.h"
#include "lm/protowrap/NDArray.h"
#include "lm/protowrap/Repeated.h"
#include "lm/Types.h"

using lm::protowrap::MessageWrap;
using lm::protowrap::Repeated;

namespace lm {
namespace protowrap {

class FFluxStageOutputRaw : public MessageWrap<lm::fflux::io::FFluxStageOuputRaw>
{
public:
    typedef MessageWrap::Msg Msg;

    // accessors
    const Repeated<uint64_t>& sucessful_trajectory_counts() const {return *_sucessful_trajectory_counts;}
    const Repeated<double>& sucessful_trajectory_total_times() const {return *_sucessful_trajectory_total_times;}
    const Repeated<uint64_t>& failed_trajectory_counts() const {return *_failed_trajectory_counts;}
    const Repeated<double>& failed_trajectory_total_times() const {return *_failed_trajectory_total_times;}

    // mutators
    Repeated<uint64_t>* sucessful_trajectory_counts() {return _sucessful_trajectory_counts;}
    Repeated<double>* sucessful_trajectory_total_times() {return _sucessful_trajectory_total_times;}
    Repeated<uint64_t>* failed_trajectory_counts() {return _failed_trajectory_counts;}
    Repeated<double>* failed_trajectory_total_times() {return _failed_trajectory_total_times;}

    // pass through accessors
    // pass through mutators

    Repeated<uint64_t>* _sucessful_trajectory_counts;
    Repeated<double>* _sucessful_trajectory_total_times;

    Repeated<uint64_t>* _failed_trajectory_counts;
    Repeated<double>* _failed_trajectory_total_times;
};

class FFluxStageOutput : public MessageWrap<lm::fflux::io::FFluxStageOutput>
{
public:
    typedef MessageWrap::Msg Msg;
    typedef FFluxStageOutputRaw::Msg MsgRaw;

    virtual void setMsg(Msg* newMsgMutablePtr)
    {
        msgPtr = newMsgMutablePtr;
    }

    // accessors
    const Repeated<double>& switching_time_per_tile() const {return *_switching_time_per_tile;}
    const Repeated<double>& fluxes() const {return *_fluxes;}
    const Repeated<double>& probabilities() const {return *_probabilities;}
    const FFluxStageOutputRaw& fflux_stage_output_raw() const {return *_fflux_stage_output_raw;}

    // mutators
    Repeated<double>* mutable_switching_time_per_tile() {return _switching_time_per_tile;}
    Repeated<double>* mutable_fluxes() {return _fluxes;}
    Repeated<double>* mutable_probabilities() {return _probabilities;}
    FFluxStageOutputRaw* mutable_fflux_stage_output_raw() {return _fflux_stage_output_raw;}

    // pass through accessors
    // pass through mutators

protected:
    Repeated<double>* _switching_time_per_tile;
    Repeated<double>* _fluxes;
    Repeated<double>* _probabilities;
    FFluxStageOutputRaw* _fflux_stage_output_raw;
};

}
}


#endif /* LM_PROTOWRAP_FFLUXSTAGEOUTPUT_H_ */
