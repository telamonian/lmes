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

using lm::limit::LimitElement;

namespace lm {
namespace limit {

bool TrajectoryLimits::hasMsg(int32_t id) const
{
    // if a TrajectoryLimitMsg with a matching id is found, return true
    for (TrajectoryLimits::RepeatedType::const_iterator it=repeated().begin();it!=repeated().end();it++)
    {
        if (it->id()==id) return true;
    }
    // otherwise, return false
    return false;
}

bool TrajectoryLimits::hasMsg(TrajLimEnums::LimitType lt) const
{
    // if a TrajectoryLimitMsg with a matching limit type is found, return true
    for (TrajectoryLimits::RepeatedType::const_iterator it=repeated().begin();it!=repeated().end();it++)
    {
        if (it->limit_type()==lt) return true;
    }
    // otherwise, return false
    return false;
}

const TrajectoryLimitMsg& TrajectoryLimits::findMsg(int32_t id) const
{
    // return the first TrajectoryLimitMsg with a matching id
    for (TrajectoryLimits::RepeatedType::const_iterator it=repeated().begin();it!=repeated().end();it++)
    {
        if (it->id()==id) return *it;
    }

    // in no TrajectoryLimitMsgs are found with a matching id, throw an exception
    throw NotFoundException("limit with id %d was not found in TrajectoryLimits instance", id);
}

const TrajectoryLimitMsg& TrajectoryLimits::findMsg(TrajLimEnums::LimitType lt) const
{
    // return the first TrajectoryLimitMsg with a matching limit type
    for (TrajectoryLimits::RepeatedType::const_iterator it=repeated().begin();it!=repeated().end();it++)
    {
        if (it->limit_type()==lt) return *it;
    }

    // if no TrajectoryLimitMsgs are found with the appropriate limit type, throw an exception
    throw NotFoundException("no limits with LimitType %s found in TrajectoryLimits instance", TrajLimEnums::LimitType_Name(lt).c_str());
}

void TrajectoryLimits::addTileExitLimitsMsg(const lm::tiling::Tiling& tiling, int edge0Index, int edge1Index, bool edge0Exists, bool edge1Exists,
                                            bool rightOpenBins, int32_t edge0LimitID, int32_t edge1LimitID)
{
    // if an edgeIndex is less than 0 or greater than tiling.edges().lastIndex(), pretend that it's an extra edge one unit past the last edge (useful in conjunction with edgeExists for setting half-infinite bins)
    double edge0Value = tiling.getEdgeFixBounds(edge0Index);
    double edge1Value = tiling.getEdgeFixBounds(edge1Index);

    addBinExitLimitsMsg<TrajLimEnums::ORDER_PARAMETER>(tiling.getOrderParameterID(), edge0Value, edge1Value, edge0Exists, edge1Exists, rightOpenBins, edge0LimitID, edge1LimitID);
}

//LimitTrackingWrap::WrappedMsg* TrajectoryLimits::addTrackingMsg(int32_t limitID, bool addToOutput, bool addToCMEState, int64_t count, bool terminate)
//{
//    // set the tracking options on the limit of interest
//    TrajectoryLimitMsg* trackedLimitMsg = findMsg(limitID);
//
//    trackedLimitMsg->set_terminate(terminate);
//
//    trackedLimitMsg->set_add_tracking_to_output(addToOutput);
//    trackedLimitMsg->set_add_tracking_to_cme_state(addToCMEState);
//
//    // if count < 0, unset track_count. When unset, tracking data will be collected every time the limit is triggered and the limit will never trigger trajectory termination
//    if (count < 0) trackedLimitMsg->clear_track_count();
//    else           trackedLimitMsg->set_track_count(static_cast<uint>(count));
//
//    // initialize the actual tracking message
//    LimitTrackingWrap::WrappedMsg* trackingMsg = _limitTrackings.Add();
//    trackingMsg->set_limit_id(limitID);
//    return trackingMsg;
//}
//
//// version of addTracking message that allow for setting non terminating tracking without necessarily filling in every default value in the signature
//LimitTrackingWrap::WrappedMsg* TrajectoryLimits::addTrackingMsgNonterminating(int32_t limitID, bool addToOutput, bool addToCMEState, int64_t count)
//{
//    return addTrackingMsg(limitID, addToOutput, addToCMEState, count, false);
//}

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
void TrajectoryLimits::wTV(VectorType& outVec)
{
    outVec.clear();
    for (TrajectoryLimits::RepeatedType::const_iterator it=repeated().begin(); it!=repeated().end(); it++)
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
