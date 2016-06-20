/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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
#ifndef LM_LIMIT_TRAJECTORYLIMIT
#define LM_LIMIT_TRAJECTORYLIMIT

#include <limits>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

#include "lm/EnumHelper.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/SimulationParameters.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/limit/LimitTracking.h"
#include "lm/option/SimulationParameters.h"
#include "lm/protowrap/Repeated.h"
#include "lm/tiling/Tiling.h"
#include "lm/Types.h"

namespace lm {
namespace limit {


struct TrajectoryLimit
{
    TrajLimEnums::LimitType type;
    TrajLimEnums::StoppingCondition stoppingCondition;
    bool includeEndpoint;
    int32_t limitID;

    uint32_t valueID;
    int32_t ivalue;
    double dvalue;
    uint64_t uvalue;

    // variables related to limit tracking
    bool terminate;
    bool addTrackingToCMEState;
    bool addTrackingToOutput;
    uint64_t trackCount;
};

}
}

#endif /* LM_LIMIT_TRAJECTORYLIMIT */