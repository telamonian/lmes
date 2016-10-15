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
    template <typename Value>
    inline void setInitialWriteInterval(double interval, double* nextWriteTime, Value* valueArray, int valueSize, std::vector<Value>* valueVector, std::vector<double>* timeVector)
    {
//        // if this is the start of the trajectory's first work unit...
//        if (not trajectoryStarted)
//        {
//            // and if we're specifically writing out initial states, do that then set the next write time. If the next write time happens to be the current time, skip that since we just wrote it out
//            if (writeInitialTrajectoryState)
//            {
//                *nextWriteTime = (floor(time/interval) + 1)*interval;
//                for (uint i=0; i<valueSize; i++) valueVector->push_back(valueArray[i]);
//                timeVector->push_back(time);
//            }
//            // otherwise, just set the next write time. If the next write time happens to be the current time, use that
//            else
//            {
//                *nextWriteTime = ceil(time/interval)*interval;
//            }
//        }
//        // otherwise, just set the next write time. If the next write time happens to be the current time, skip that since we already wrote it out in the previous work unit
//        else
//        {
//            *nextWriteTime = (floor(time/interval) + 1)*interval;
//        }
        // if this is the start of the trajectory's first work unit...
        if (not trajectoryStarted and writeInitialTrajectoryState)
        {
            // and if we're specifically writing out initial states, do that then set the next write time. If the next write time happens to be the current time, skip that since we just wrote it out
            for (uint i=0; i<valueSize; i++) valueVector->push_back(valueArray[i]);
            timeVector->push_back(time);
        }
        // otherwise, just set the next write time. If the next write time happens to be the current time, skip that since we already wrote it out in the previous work unit
        *nextWriteTime = (floor(time/interval) + 1)*interval;
    }
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
