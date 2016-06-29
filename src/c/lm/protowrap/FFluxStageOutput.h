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
#include "lm/protowrap/NDArray.h"
#include "lm/protowrap/Repeated.h"
#include "lm/Types.h"

using lm::protowrap::Repeated;

namespace lm {
namespace protowrap {

class FFluxStageOutput
{
public:
    typedef lm::fflux::io::FFluxPhaseOutput Msg;

    Msg* getMsg()
    {
        return msgPtr;
    }

    void setMsg(Msg* newMsgMutablePtr)
    {
        msgPtr = newMsgMutablePtr;
    }


protected:
    Msg* msgPtr;

};

}
}


#endif /* LM_PROTOWRAP_FFLUXPHASEOUTPUT_H_ */
