/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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

#ifndef LM_CME_GILLESPIEDSOLVER_H_
#define LM_CME_GILLESPIEDSOLVER_H_

#include <map>
#include <list>
#include <string>
#include "lm/ClassFactory.h"
#include "lm/cme/CMESolver.h"
#include "lm/io/DegreeAdvancementTimeSeries.pb.h"
#include "lm/io/OrderParameterTimeSeries.pb.h"
#include "lm/limit/LimitTracking.h"
#include "lm/protowrap/TimeSeries.h"
#include "lm/rng/RandomGenerator.h"

using std::map;
using std::list;
using std::string;
using lm::rng::RandomGenerator;

namespace lm {
namespace cme {

class GillespieDSolver : public CMESolver
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

public:
    GillespieDSolver();
    virtual ~GillespieDSolver();
    virtual void reset();
    virtual void getState(lm::io::TrajectoryState* state, uint trajectoryNumber=0);
    virtual void setState(const lm::io::TrajectoryState& state, uint trajectoryNumber=0);
    virtual long long generateTrajectory(long long maxSteps);

protected:
    virtual void updateAllPropensities();
    inline void updatePropensities(uint r);

protected:
    lm::protowrap::TimeSeries<lm::io::DegreeAdvancementTimeSeries> daTimeSeriesWrap;
    lm::protowrap::TimeSeries<lm::io::OrderParameterTimeSeries> opTimeSeriesWrap;
    double * propensities;
};

}
}

#endif
