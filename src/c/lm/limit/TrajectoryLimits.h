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
#ifndef LM_LIMIT_TRAJECTORYLIMITS
#define LM_LIMIT_TRAJECTORYLIMITS

#include <limits>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

#include "lm/EnumHelper.h"
#include "lm/io/LimitTracking.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/SimulationParameters.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/limit/LimitCheckMacros.h"
#include "lm/limit/LimitTrackingListWrap.h"
#include "lm/limit/TrajectoryLimit.h"
#include "lm/protowrap/Repeated.h"
#include "lm/tiling/Tiling.h"
#include "lm/Types.h"

namespace lm {
namespace limit {

typedef lm::input::TrajectoryLimits TrajectoryLimitsMsg;
typedef lm::input::TrajectoryLimit TrajectoryLimitMsg;

// main template for LimitType->ValueType type generator
template <TrajLimEnums::LimitType LT> struct LimitElement;
// NB: if any new LimitType enum values are added in the future, add a template specialization below
template <> struct LimitElement<TrajLimEnums::TIME> {typedef double type;};
template <> struct LimitElement<TrajLimEnums::DEGREE_ADVANCEMENT> {typedef uint64_t type;};
template <> struct LimitElement<TrajLimEnums::ORDER_PARAMETER> {typedef double type;};
template <> struct LimitElement<TrajLimEnums::SPECIES> {typedef int32_t type;};

template <typename ValueT, typename ContainerT> inline ValueT _getLimitValue(const ContainerT& tl);
template <> inline double _getLimitValue<double, TrajectoryLimitMsg>(const TrajectoryLimitMsg& tl) {return tl.dvalue();}
template <> inline int32_t _getLimitValue<int32_t, TrajectoryLimitMsg>(const TrajectoryLimitMsg& tl) {return tl.ivalue();}
template <> inline uint64_t _getLimitValue<uint64_t, TrajectoryLimitMsg>(const TrajectoryLimitMsg& tl) {return tl.uvalue();}
template <> inline double _getLimitValue<double, TrajectoryLimit>(const TrajectoryLimit& tl) {return tl.dvalue;}
template <> inline int32_t _getLimitValue<int32_t, TrajectoryLimit>(const TrajectoryLimit& tl) {return tl.ivalue;}
template <> inline uint64_t _getLimitValue<uint64_t, TrajectoryLimit>(const TrajectoryLimit& tl) {return tl.uvalue;}
template <TrajLimEnums::LimitType LT, typename ContainerT> inline typename LimitElement<LT>::type getLimitValue(const ContainerT& tl) {return _getLimitValue<typename LimitElement<LT>::type, ContainerT>(tl);}

template <typename ContainerT, typename ValueT> inline void setLimitValue(ContainerT& tl, ValueT val);
template <> inline void setLimitValue<TrajectoryLimitMsg, double>(TrajectoryLimitMsg& tl, double val) {tl.set_dvalue(val);}
template <> inline void setLimitValue<TrajectoryLimitMsg, int32_t>(TrajectoryLimitMsg& tl, int32_t val) {tl.set_ivalue(val);}
template <> inline void setLimitValue<TrajectoryLimitMsg, uint64_t>(TrajectoryLimitMsg& tl, uint64_t val) {tl.set_uvalue(val);}
template <> inline void setLimitValue<TrajectoryLimit, double>(TrajectoryLimit& tl, double val) {tl.dvalue = val;}
template <> inline void setLimitValue<TrajectoryLimit, int32_t>(TrajectoryLimit& tl, int32_t val) {tl.ivalue = val;}
template <> inline void setLimitValue<TrajectoryLimit, uint64_t>(TrajectoryLimit& tl, uint64_t val) {tl.uvalue = val;}
        
class TrajectoryLimits
{
public:
// constants
    static const int32_t TIME_LIMIT_ID = -1;
    static const int32_t DEFAULT_LIMIT_ID = -2;

// typedefs
    typedef lm::protowrap::Repeated<TrajectoryLimitMsg> RepeatedType;
    typedef std::vector<TrajectoryLimit> VectorType;
    typedef VectorType::iterator iterator;
    typedef VectorType::const_iterator const_iterator;
    
// constructors/destructors
    TrajectoryLimits(): nextID(0) {seatRepeated(_msg);}
    TrajectoryLimits(const TrajectoryLimitsMsg& inBuf): nextID(0) {rFB(inBuf);}
    //    TrajectoryLimits(const lm::io::hdf5::Hdf5File& file) {rFF(file);}
    ~TrajectoryLimits() {}

// accessors
    int ByteSize() const {return _msg.ByteSize();}
    bool hasMsg(int32_t id) const;
    bool hasMsg(TrajLimEnums::LimitType lt) const;
    const TrajectoryLimitMsg& findMsg(int32_t id) const;
    const TrajectoryLimitMsg& findMsg(TrajLimEnums::LimitType lt) const;
    const TrajectoryLimitMsg& getTimeLimitMsg() const {return _msg.time_limit();}
    double getTimeLimitValue() const {return _msg.has_time_limit() ? _msg.time_limit().dvalue() : std::numeric_limits<double>::infinity();}
    bool hasDegreeAdvancementLimit() const {return hasMsg(TrajLimEnums::DEGREE_ADVANCEMENT);}

    const TrajectoryLimitsMsg& buf() const {return _msg;}
    const RepeatedType& repeated() const {return _repeated;}
    const VectorType& vec() const {return _vec;}

// mutators
    // general addLimitMsg
    template <TrajLimEnums::LimitType LT> inline TrajectoryLimitMsg*
    addLimitMsg(uint32_t valID, typename LimitElement<LT>::type val, TrajLimEnums::StoppingCondition sc, bool includeEndpoint = true, int32_t id = DEFAULT_LIMIT_ID)
    {
        lm::input::TrajectoryLimit* tlMsg;
        if (LT==TrajLimEnums::TIME)
        {
            tlMsg = _msg.mutable_time_limit();
            // for now, the expected behavior is that the id of the time limit will default to -1
            tlMsg->set_id(applyDefaultTimeLimitID(id));
        }
        else
        {
            tlMsg = _repeated.Add();
            // for now, the expected behavior is that the id of most limits (ie not TIME) will default to an incrementing counter
            tlMsg->set_id(applyDefaultID(id));
        }

        tlMsg->set_limit_type(LT);
        tlMsg->set_stopping_condition(sc);
        tlMsg->set_include_endpoint(includeEndpoint);

        tlMsg->set_value_id(valID);
        setLimitMsgValue(tlMsg, val);

        return tlMsg;
    }

    // version of addLimitMsg that adds the limits appropriate for tracking when a trajectory exits a bin (it helps to think of it as a bin on a histogram)
    template <TrajLimEnums::LimitType LT> inline void
    addBinExitLimitsMsg(uint32_t valID, typename LimitElement<LT>::type edge0Val, typename LimitElement<LT>::type edge1Val,
                        bool edge0Exists=true, bool edge1Exists=true, bool rightOpenBins=true,
                        int32_t edge0LimitID=DEFAULT_LIMIT_ID, int32_t edge1LimitID=DEFAULT_LIMIT_ID)
    {
        // if DEFAULT_LIMIT_ID is used for the ids, set the limitIDs using an incrementing counter. Skip if not edgeExists
        if (edge0Exists) edge0LimitID = applyDefaultID(edge0LimitID);
        if (edge1Exists) edge1LimitID = applyDefaultID(edge1LimitID);

        // declare positional variables
        typename LimitElement<LT>::type leftEdgeVal,rightEdgeVal;
        bool leftEdgeExists, rightEdgeExists, leftIncludeEndpoint, rightIncludeEndpoint;
        int32_t leftEdgeLimitID, rightEdgeLimitID;
        
        // determine the position of the bin edges relative to one another wrt a 1D number line
        bool edgesIncreasing = (edge1Val>=edge0Val);

        // based on this relative position, assign edge parameters to positional variables
        leftEdgeVal     = (edgesIncreasing ? edge0Val     : edge1Val);
        leftEdgeExists  = (edgesIncreasing ? edge0Exists  : edge1Exists);
        leftEdgeLimitID = (edgesIncreasing ? edge0LimitID : edge1LimitID);

        rightEdgeVal     = (edgesIncreasing ? edge1Val     : edge0Val);
        rightEdgeExists  = (edgesIncreasing ? edge1Exists  : edge0Exists);
        rightEdgeLimitID = (edgesIncreasing ? edge1LimitID : edge0LimitID);

        // - if rightOpenBins,
        //     - set a closed boundary condition (includeEndpoint==false) on the left edge
        //     - set an open boundary boundary condition (includeEndpoint==true) on the right edge
        // - otherwise, do the opposite
        leftIncludeEndpoint  = (rightOpenBins ? false : true);
        rightIncludeEndpoint = (rightOpenBins ? true  : false);

        // add left and right limits. Skip if not edgeExists (this gives a half-infinite bin)
        if (leftEdgeExists)  addLimitMsg<LT>(valID, leftEdgeVal,  TrajLimEnums::DECREASING, leftIncludeEndpoint,  leftEdgeLimitID);
        if (rightEdgeExists) addLimitMsg<LT>(valID, rightEdgeVal, TrajLimEnums::INCREASING, rightIncludeEndpoint, rightEdgeLimitID);
    }

    // addLimitMsg version for tilings.
    void addTileExitLimitsMsg(const lm::tiling::Tiling& tiling, int edge0Index, int edge1Index, bool edge0Exists=true, bool edge1Exists=true,
                              bool rightOpenBins=true, int32_t edge0LimitID=DEFAULT_LIMIT_ID, int32_t edge1LimitID=DEFAULT_LIMIT_ID);

    TrajectoryLimitMsg* findMsg(int32_t id) {return const_cast<TrajectoryLimitMsg*>(&const_cast<const TrajectoryLimits*>(this)->findMsg(id));}
    TrajectoryLimitMsg* findMsg(TrajLimEnums::LimitType lt) {return const_cast<TrajectoryLimitMsg*>(&const_cast<const TrajectoryLimits*>(this)->findMsg(lt));}
    void Clear(bool resetNextID=true) {_msg.Clear(); _vec.clear(); seatRepeated(); if (resetNextID) nextID=0;}
    void seatRepeated(TrajectoryLimitsMsg& inMsg) {_repeated.setWrappedField(inMsg.mutable_trajectory_limits());}
    void seatRepeated() {seatRepeated(_msg);}
    void setMsg(const TrajectoryLimitsMsg& inMsg) {_msg.CopyFrom(inMsg);}
    void setVector(VectorType& inVec) {_vec = inVec;}

    // specializing assignment to the TrajectoryLimit buffer oneof_value field via polymorphism
    TrajectoryLimitMsg* setLimitMsgValue(TrajectoryLimitMsg* limitMsg, double val) {limitMsg->set_dvalue(val); return limitMsg;}
    TrajectoryLimitMsg* setLimitMsgValue(TrajectoryLimitMsg* limitMsg, int32_t val) {limitMsg->set_ivalue(val); return limitMsg;}
    TrajectoryLimitMsg* setLimitMsgValue(TrajectoryLimitMsg* limitMsg, uint64_t val) {limitMsg->set_uvalue(val); return limitMsg;}

    // methods for working with the tracking messages associated with the limit messages
//    LimitTrackingWrap::WrappedMsg* addTrackingMsg(int32_t limitID, bool addToOutput=true, bool addToCMEState=false, int64_t count=1, bool terminate=true);
//    LimitTrackingWrap::WrappedMsg* addTrackingMsgNonterminating(int32_t limitID, bool addToOutput=true, bool addToCMEState=false, int64_t count=-1);
//    void set_all_trajectory_id(uint64_t trajectoryID) {_limitTrackings.SetAll(trajectoryID, &lm::io::LimitTracking::set_trajectory_id);}

// protobuf and stl container IO
    void rFB(const TrajectoryLimitsMsg& inBuf);     // rFB = read From Buf
    void wTB(TrajectoryLimitsMsg& outBuf);          // wTB = write To Buf
    void wTV(VectorType& outVec);                   // wTV = write To Vec
    //void rFF(const lm::io::hdf5::Hdf5File& file); // rFF = read From File

    void rFB() {return rFB(_msg);}
    void wTB() {return wTB(_msg);}
    void wTV() {return wTV(_vec);}

// const qualified pass-throughs to the underlying buf and stl container
    VectorType::size_type size() const {return _vec.size();}

// static functions to do TrajectoryLimit buf <-> TrajectoryLimit struct conversion
    static TrajectoryLimit bufToStruct(const TrajectoryLimitMsg& inBuf);
    static TrajectoryLimitMsg structToBuf(const TrajectoryLimit& inStruct);

protected:
    int32_t applyDefaultID(int32_t id) {return (id==DEFAULT_LIMIT_ID ? nextID++ : id);}
    int32_t applyDefaultTimeLimitID(int32_t id) {return (id==DEFAULT_LIMIT_ID ? TIME_LIMIT_ID : id);}

protected:
    int32_t nextID;

    TrajectoryLimitsMsg _msg;

    RepeatedType _repeated;
    VectorType _vec;
};

}
}

#endif /* LM_LIMIT_TRAJECTORYLIMITS */