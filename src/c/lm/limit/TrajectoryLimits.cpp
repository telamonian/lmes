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
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/tiling/Tiling.h"
#include "lm/limit/TrajectoryLimit.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/Types.h"

using lm::limit::LimitValueT;

namespace lm {
namespace limit {

TrajectoryLimits::repeatedType::const_iterator TrajectoryLimits::findMsg(int32_t id) const
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

TrajectoryLimits::repeatedType::const_iterator TrajectoryLimits::findMsg(TrajLimEnums::LimitType lt) const
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

void TrajectoryLimits::addTileExitLimitsMsg(lm::tiling::Tiling& tiling, int edge0Index, int edge1Index, bool edge0Exists=true, bool edge1Exists=true,
                                            bool rightOpenBins=true, int32_t edge0LimitID=DEFAULT_LIMIT_ID, int32_t edge1LimitID=DEFAULT_LIMIT_ID)
{
    // if an edgeIndex is less than 0 or greater than tiling.edges().lastIndex(), pretend that it's an extra edge one unit past the last edge (useful in conjunction with edgeExists for setting half-infinite bins)
    double edge0Value = tiling.getEdgeFixBounds(edge0Index);
    double edge1Value = tiling.getEdgeFixBounds(edge1Index);

    addBinExitLimitsMsg<TrajLimEnums::ORDER_PARAMETER>(tiling.getOrderParameterID(), edge0Value, edge1Value, edge0Exists, edge1Exists, rightOpenBins, edge0LimitID, edge1LimitID);
}

// rFB = read From Buf
void TrajectoryLimits::rFB(const TrajectoryLimitsMsg& inBuf)
{
    if (&inBuf!=&_msg) _msg.CopyFrom(inBuf);
    seatRepeated();
    wTV();
}

// rFB = write To Buf
void TrajectoryLimits::wTB(TrajectoryLimitsMsg& outBuf)
{
    outBuf.clear_trajectory_limits();
    for (TrajectoryLimits::const_iterator it=vec().begin(); it!=vec().end(); it++)
    {
        TrajectoryLimitMsg* limitBuf = outBuf.add_trajectory_limits();
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

TrajectoryLimit TrajectoryLimits::bufToStruct(const lm::input::TrajectoryLimit& inBuf)
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

    limit.terminate = inBuf.terminate();
    limit.addTrackingToCMEState = inBuf.add_tracking_to_cme_state();
    limit.addTrackingToOutput = inBuf.add_tracking_to_output();
    limit.trackCount = inBuf.track_count();

    return limit;
}
    
TrajectoryLimitMsg TrajectoryLimits::structToBuf(const TrajectoryLimit& inStruct)
{
    TrajectoryLimitMsg limitBuf;
    limitBuf.set_limit_type(inStruct.type);
    limitBuf.set_stopping_condition(inStruct.stoppingCondition);
    limitBuf.set_id(inStruct.limitID);
    limitBuf.set_include_endpoint(inStruct.includeEndpoint);

    limitBuf.set_value_id(inStruct.valueID);
    switch(inStruct.type)
    {
    case TrajLimEnums::TIME:
        setLimitValue(limitBuf, getLimitValue<TrajLimEnums::TIME>(inStruct));
        break;
    case TrajLimEnums::SPECIES:
        setLimitValue(limitBuf, getLimitValue<TrajLimEnums::SPECIES>(inStruct));
        break;
    case TrajLimEnums::ORDER_PARAMETER:
        setLimitValue(limitBuf, getLimitValue<TrajLimEnums::ORDER_PARAMETER>(inStruct));
        break;
    case TrajLimEnums::DEGREE_ADVANCEMENT:
        setLimitValue(limitBuf, getLimitValue<TrajLimEnums::DEGREE_ADVANCEMENT>(inStruct));
        break;
    default:
        throw Exception("When converting a TrajectoryLimit struct to a TrajectoryLimit buf, the TrajectoryLimit struct did not have a recognized type", inStruct.type, inStruct.stoppingCondition);
        break;
    }

    limitBuf.set_terminate(inStruct.terminate);
    limitBuf.set_add_tracking_to_cme_state(inStruct.addTrackingToCMEState);
    limitBuf.set_add_tracking_to_output(inStruct.addTrackingToOutput);
    limitBuf.set_track_count(inStruct.trackCount);

    return limitBuf;
}
    
}
}
