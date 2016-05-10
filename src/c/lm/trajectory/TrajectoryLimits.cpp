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
#include "lm/EnumHelper.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/trajectory/TrajectoryLimits.h"
#include "lm/Types.h"

using lm::trajectory::LimitValueT;

namespace lm {
namespace trajectory {

TrajectoryLimits::repeatedType::const_iterator TrajectoryLimits::findBuf(int32_t id) const
{
    TrajectoryLimits::repeatedType::const_iterator it=repeated().begin();
    // if the .id() of a TrajectoryLimit buf matches, return it
    for (;it!=repeated().end();it++)
    {
        if (it->id()==id)
        {
            return it;
        }
    }
    // otherwise, return .end()
    return it;
}

TrajectoryLimits::repeatedType::const_iterator TrajectoryLimits::findBuf(EH::LimitType lt) const
{
    TrajectoryLimits::repeatedType::const_iterator it=repeated().begin();
    // if the .limit_type() of a TrajectoryLimit buf matches, return it
    for (;it!=repeated().end();it++)
    {
        if (it->limit_type()==lt)
        {
            return it;
        }
    }
    // otherwise, return .end()
    return it;
}

// rFB = read From Buf
void TrajectoryLimits::rFB(const TrajectoryLimitsBuf& inBuf)
{
    if (&inBuf!=&_buf) _buf.CopyFrom(inBuf);
    seatRepeated();
    wTV();
}

// rFB = write To Buf
void TrajectoryLimits::wTB(TrajectoryLimitsBuf& outBuf)
{
    outBuf.clear_trajectory_limits();
    for (TrajectoryLimits::const_iterator it=vec().begin(); it!=vec().end(); it++)
    {
        TrajectoryLimitBuf* limitBuf = outBuf.add_trajectory_limits();
        limitBuf->CopyFrom(structToBuf(*it));
    }
}

// wTV = write To Vec
void TrajectoryLimits::wTV(vectorType& outVec)
{
    outVec.clear();
    for (TrajectoryLimits::repeatedType::const_iterator it=repeated().begin(); it!=repeated().end(); it++)
    {
        outVec.push_back(bufToStruct(*it));
    }
}

TrajectoryLimit TrajectoryLimits::bufToStruct(const lm::io::TrajectoryLimits::TrajectoryLimit& inBuf)
{
    TrajectoryLimit limit;
    limit.type = inBuf.limit_type();
    limit.stoppingCondition = inBuf.stopping_condition();
    limit.limitID = inBuf.id();
    limit.includeEndpoint = inBuf.include_endpoint();

    limit.valueID = inBuf.value_id();
    limit.dvalue = inBuf.dvalue();
    limit.ivalue = inBuf.ivalue();
    limit.uvalue = inBuf.uvalue();

    return limit;
}
    
TrajectoryLimitBuf TrajectoryLimits::structToBuf(const TrajectoryLimit& inStruct)
{
    TrajectoryLimitBuf limitBuf;
    limitBuf.set_limit_type(inStruct.type);
    limitBuf.set_stopping_condition(inStruct.stoppingCondition);
    limitBuf.set_id(inStruct.limitID);
    limitBuf.set_include_endpoint(inStruct.includeEndpoint);

    limitBuf.set_value_id(inStruct.valueID);

    switch(inStruct.type)
    {
    case EH::TIME:
        setLimitValue(limitBuf, getLimitValue<EH::TIME>(inStruct));
        break;
    case EH::SPECIES:
        setLimitValue(limitBuf, getLimitValue<EH::SPECIES>(inStruct));
        break;
    case EH::ORDER_PARAMETER:
        setLimitValue(limitBuf, getLimitValue<EH::ORDER_PARAMETER>(inStruct));
        break;
    case EH::DEGREE_ADVANCEMENT:
        setLimitValue(limitBuf, getLimitValue<EH::DEGREE_ADVANCEMENT>(inStruct));
        break;
    default:
        throw Exception("When converting a TrajectoryLimit struct to a TrajectoryLimit buf, the TrajectoryLimit struct did not have a recognized type", inStruct.type, inStruct.stoppingCondition);
        break;
    }
    return limitBuf;
}
    
}
}
