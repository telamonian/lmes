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
#ifndef LM_TRAJECTORY_TRAJECTORYLIMITS
#define LM_TRAJECTORY_TRAJECTORYLIMITS

#include <limits>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

#include "lm/EnumHelper.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/SimulationParameters.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/option/SimulationParameters.h"
#include "lm/protowrap/Repeated.h"
#include "lm/tiling/Tiling.h"
#include "lm/Types.h"

namespace lm {
namespace trajectory {

typedef lm::input::TrajectoryLimits TrajectoryLimitsMsg;
typedef lm::input::TrajectoryLimit TrajectoryLimitMsg;

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

// main template for LimitType->ValueType type generator
template <TrajLimEnums::LimitType LT> struct LimitValueT;
// NB: if any new LimitType enum values are added in the future, add a template specialization below
template <> struct LimitValueT<TrajLimEnums::TIME> {typedef double type;};
template <> struct LimitValueT<TrajLimEnums::DEGREE_ADVANCEMENT> {typedef uint64_t type;};
template <> struct LimitValueT<TrajLimEnums::ORDER_PARAMETER> {typedef double type;};
template <> struct LimitValueT<TrajLimEnums::SPECIES> {typedef int32_t type;};

template <typename ValueT, typename ContainerT> inline ValueT _getLimitValue(const ContainerT& tl);
template <> inline double _getLimitValue<double, TrajectoryLimitMsg>(const TrajectoryLimitMsg& tl) {return tl.dvalue();}
template <> inline int32_t _getLimitValue<int32_t, TrajectoryLimitMsg>(const TrajectoryLimitMsg& tl) {return tl.ivalue();}
template <> inline uint64_t _getLimitValue<uint64_t, TrajectoryLimitMsg>(const TrajectoryLimitMsg& tl) {return tl.uvalue();}
template <> inline double _getLimitValue<double, TrajectoryLimit>(const TrajectoryLimit& tl) {return tl.dvalue;}
template <> inline int32_t _getLimitValue<int32_t, TrajectoryLimit>(const TrajectoryLimit& tl) {return tl.ivalue;}
template <> inline uint64_t _getLimitValue<uint64_t, TrajectoryLimit>(const TrajectoryLimit& tl) {return tl.uvalue;}
template <TrajLimEnums::LimitType LT, typename ContainerT> inline typename LimitValueT<LT>::type getLimitValue(const ContainerT& tl) {return _getLimitValue<typename LimitValueT<LT>::type, ContainerT>(tl);}

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
    typedef lm::protowrap::Repeated<TrajectoryLimitMsg> repeatedType;
    typedef vector<TrajectoryLimit> vectorType;
    typedef vectorType::iterator iterator;
    typedef vectorType::const_iterator const_iterator;
    
// constructors/destructors
    TrajectoryLimits(): nextID(0) {seatRepeated(_msg);}
    TrajectoryLimits(const TrajectoryLimitsMsg& inBuf): nextID(0) {rFB(inBuf);}
    //    TrajectoryLimits(const lm::io::hdf5::Hdf5File& file) {rFF(file);}
    ~TrajectoryLimits() {}

// accessors
    repeatedType::const_iterator findMsg(int32_t id) const;
    repeatedType::const_iterator findMsg(TrajLimEnums::LimitType lt) const;
    const TrajectoryLimitMsg& getTimeBuf() const {return _msg.time_limit();}
    double getTimeLimitValue() const {return _msg.has_time_limit() ? _msg.time_limit().dvalue() : std::numeric_limits<double>::infinity();}
    bool hasDegreeAdvancementLimit() const {return (findMsg(TrajLimEnums::DEGREE_ADVANCEMENT)!=repeated().end());}

    const TrajectoryLimitsMsg& buf() const {return _msg;}
    const repeatedType& repeated() const {return _repeated;}
    const vectorType& vec() const {return _vec;}

// mutators
    // general addLimitMsg
    template <TrajLimEnums::LimitType LT> inline TrajectoryLimitMsg*
    addLimitMsg(uint32_t valID, typename LimitValueT<LT>::type val, TrajLimEnums::StoppingCondition sc, bool includeEndpoint = true, int32_t id = DEFAULT_LIMIT_ID)
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
    addBinExitLimitsMsg(uint32_t valID, typename LimitValueT<LT>::type edge0Val, typename LimitValueT<LT>::type edge1Val,
                        bool edge0Exists=true, bool edge1Exists=true, bool rightOpenBins=true,
                        int32_t edge0LimitID=DEFAULT_LIMIT_ID, int32_t edge1LimitID=DEFAULT_LIMIT_ID)
    {
        // if DEFAULT_LIMIT_ID is used for the ids, set the limitIDs using an incrementing counter. Skip if not edgeExists
        if (edge0Exists) edge0LimitID = applyDefaultID(edge0LimitID);
        if (edge1Exists) edge1LimitID = applyDefaultID(edge1LimitID);

        // declare positional variables
        typename LimitValueT<LT>::type leftEdgeVal,rightEdgeVal;
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
    void addTileExitLimitsMsg(lm::tiling::Tiling& tiling, int edge0Index, int edge1Index, bool edge0Exists=true, bool edge1Exists=true,
                              bool rightOpenBins=true, int32_t edge0LimitID=DEFAULT_LIMIT_ID, int32_t edge1LimitID=DEFAULT_LIMIT_ID);

    void clear(bool resetNextID=true) {_msg.Clear(); _vec.clear(); seatRepeated(); if (resetNextID) nextID=0;}
    void seatRepeated(TrajectoryLimitsMsg& inMsg) {_repeated.setRepFieldPtr(inMsg.mutable_trajectory_limits());}
    void seatRepeated() {seatRepeated(_msg);}
    void setMsg(const TrajectoryLimitsMsg& inMsg) {_msg.CopyFrom(inMsg);}
    void setVector(vectorType& inVec) {_vec = inVec;}

    // specializing assignment to the TrajectoryLimit buffer oneof_value field via polymorphism
    TrajectoryLimitMsg* setLimitMsgValue(TrajectoryLimitMsg* limitMsg, double val) {limitMsg->set_dvalue(val); return limitMsg;}
    TrajectoryLimitMsg* setLimitMsgValue(TrajectoryLimitMsg* limitMsg, int32_t val) {limitMsg->set_ivalue(val); return limitMsg;}
    TrajectoryLimitMsg* setLimitMsgValue(TrajectoryLimitMsg* limitMsg, uint64_t val) {limitMsg->set_uvalue(val); return limitMsg;}

// protobuf and stl container IO
    void rFB(const TrajectoryLimitsMsg& inBuf);     // rFB = read From Buf
    void wTB(TrajectoryLimitsMsg& outBuf);          // wTB = write To Buf
    void wTV(vectorType& outVec);                   // wTV = write To Vec
    //void rFF(const lm::io::hdf5::Hdf5File& file); // rFF = read From File

    void rFB() {return rFB(_msg);}
    void wTB() {return wTB(_msg);}
    void wTV() {return wTV(_vec);}

// const qualified pass-throughs to the underlying buf and stl container
    vectorType::size_type size() const {return _vec.size();}

// static functions to do TrajectoryLimit buf <-> TrajectoryLimit struct conversion
    static TrajectoryLimit bufToStruct(const TrajectoryLimitMsg& inBuf);
    static TrajectoryLimitMsg structToBuf(const TrajectoryLimit& inStruct);

protected:
    int32_t applyDefaultID(int32_t id) {return (id==DEFAULT_LIMIT_ID ? nextID++ : id);}
    int32_t applyDefaultTimeLimitID(int32_t id) {return (id==DEFAULT_LIMIT_ID ? TIME_LIMIT_ID : id);}

protected:
    int32_t nextID;

    TrajectoryLimitsMsg _msg;
    repeatedType _repeated;
    vectorType _vec;
};

//// the main checkLimit template. Call this function when checking any values against any limits
//template <EH::StoppingCondition sc, bool includeEndpoint> struct checkLimit;
//
//// specializations of checkLimit with regards to stoppingCondition and includeEndpoint for the basic min/max limits
//template <> struct checkLimit<EH::MIN, false> {template <typename T> static bool call(T val, T limitVal) {return (val < limitVal);}};
//template <> struct checkLimit<EH::MIN, true> {template <typename T> static bool call(T val, T limitVal) {return (val <= limitVal);}};
//template <> struct checkLimit<EH::MAX, false> {template <typename T> static bool call(T val, T limitVal) {return (val > limitVal);}};
//template <> struct checkLimit<EH::MAX, true> {template <typename T> static bool call(T val, T limitVal) {return (val >= limitVal);}};
//
//// specializations of checkLimit with regards to stoppingCondition and includeEndpoint for the slightly more complex decreasing/increasing limits
//template <> struct checkLimit<EH::DECREASING, false> {template <typename T> static bool call(T prevVal, T val, T limitVal) {return (prevVal > limitVal && val <= limitVal);}};
//template <> struct checkLimit<EH::DECREASING, true> {template <typename T> static bool call(T prevVal, T val, T limitVal) {return (prevVal >= limitVal && val < limitVal);}};
//template <> struct checkLimit<EH::INCREASING, false> {template <typename T> static bool call(T prevVal, T val, T limitVal) {return (prevVal < limitVal && val >= limitVal);}};
//template <> struct checkLimit<EH::INCREASING, true> {template <typename T> static bool call(T prevVal, T val, T limitVal) {return (prevVal <= limitVal && val > limitVal);}};

// template conversion regexes
// template <> struct checkLimit<EH::(\w+), (\w+)>.+return (\(.+\);).+
// define check_limit_$1_$2(val, limitVal, checkBool) checkBool = $3
// define check_limit_$1_$2(prevVal, val, limitVal, checkBool) checkBool = $3

// non-standard compliant check macro, similar to the avx check macros
//define check_limit_MIN_false(val, limitVal) __extension__ ({ (val < limitVal); })

// specializations of check_limit with regards to stoppingCondition and includeEndpoint for the basic min/max limits
#define check_limit_MIN_false(val, limitVal, checkBool) checkBool = (val <  limitVal);
#define check_limit_MIN_true( val, limitVal, checkBool) checkBool = (val <= limitVal);
#define check_limit_MAX_false(val, limitVal, checkBool) checkBool = (val >  limitVal);
#define check_limit_MAX_true( val, limitVal, checkBool) checkBool = (val >= limitVal);

// specializations of check_limit with regards to stoppingCondition and includeEndpoint for second degree limits (limits that depend on both previous and present value)
#define check_limit_DECREASING_false(prevVal, val, limitVal, checkBool) checkBool = (prevVal >= limitVal && val <  limitVal);
#define check_limit_DECREASING_true( prevVal, val, limitVal, checkBool) checkBool = (prevVal >  limitVal && val <= limitVal);
#define check_limit_INCREASING_false(prevVal, val, limitVal, checkBool) checkBool = (prevVal <= limitVal && val >  limitVal);
#define check_limit_INCREASING_true( prevVal, val, limitVal, checkBool) checkBool = (prevVal <  limitVal && val >= limitVal);

// AVX versions
// specializations of check_limit with regards to stoppingCondition and includeEndpoint for the basic min/max limits
#define check_simple_limit_avx(valueArr, valueID, limitValue, tmpBool, checkBool, opCode) \
    tmpBool = _mm256_cmp_pd(_mm256_load_pd(&valueArr[valueID*DOUBLES_PER_AVX]), limitValue, opCode); \
    checkBool = _mm256_movemask_pd(tmpBool);

#define check_limit_avx_MIN_false(valueArr, valueID, limitValue, tmpBool, checkBool) check_simple_limit_avx(valueArr, valueID, limitValue, tmpBool, checkBool, _CMP_LT_OQ)
#define check_limit_avx_MIN_true( valueArr, valueID, limitValue, tmpBool, checkBool) check_simple_limit_avx(valueArr, valueID, limitValue, tmpBool, checkBool, _CMP_LE_OQ)
#define check_limit_avx_MAX_false(valueArr, valueID, limitValue, tmpBool, checkBool) check_simple_limit_avx(valueArr, valueID, limitValue, tmpBool, checkBool, _CMP_GT_OQ)
#define check_limit_avx_MAX_true( valueArr, valueID, limitValue, tmpBool, checkBool) check_simple_limit_avx(valueArr, valueID, limitValue, tmpBool, checkBool, _CMP_GE_OQ)

// specializations of check_limit with regards to stoppingCondition and includeEndpoint for second degree limits (limits that depend on both previous and present value)
#define check_second_degree_limit_avx(previousValueArr, valueArr, valueID, limitValue, previousTmpBool, tmpBool, checkBool, previousOpCode, opCode) \
    previousTmpBool = _mm256_cmp_pd(_mm256_load_pd(&previousValueArr[valueID*DOUBLES_PER_AVX]), limitValue, previousOpCode); \
    tmpBool         = _mm256_cmp_pd(_mm256_load_pd(&valueArr[valueID*DOUBLES_PER_AVX]), limitValue, opCode); \
    checkBool = _mm256_movemask_pd(previousTmpBool)&_mm256_movemask_pd(tmpBool);

#define check_limit_avx_DECREASING_false(previousValueArr, valueArr, valueID, limitValue, previousTmpBool, tmpBool, checkBool) check_second_degree_limit_avx(previousValueArr, valueArr, valueID, limitValue, previousTmpBool, tmpBool, checkBool, _CMP_GE_OQ, _CMP_LT_OQ)
#define check_limit_avx_DECREASING_true( previousValueArr, valueArr, valueID, limitValue, previousTmpBool, tmpBool, checkBool) check_second_degree_limit_avx(previousValueArr, valueArr, valueID, limitValue, previousTmpBool, tmpBool, checkBool, _CMP_GT_OQ, _CMP_LE_OQ)
#define check_limit_avx_INCREASING_false(previousValueArr, valueArr, valueID, limitValue, previousTmpBool, tmpBool, checkBool) check_second_degree_limit_avx(previousValueArr, valueArr, valueID, limitValue, previousTmpBool, tmpBool, checkBool, _CMP_LE_OQ, _CMP_GT_OQ)
#define check_limit_avx_INCREASING_true( previousValueArr, valueArr, valueID, limitValue, previousTmpBool, tmpBool, checkBool) check_second_degree_limit_avx(previousValueArr, valueArr, valueID, limitValue, previousTmpBool, tmpBool, checkBool, _CMP_LT_OQ, _CMP_GE_OQ)

}
}

#endif /* LM_TRAJECTORY_TRAJECTORYLIMITS */

//// standalone version of the limit checking code
//// for investigating the assembly produced by various compilers

//#include <exception>
//using std::exception;
//
//enum LimitType {NONE,
//    SPECIES,
//    ORDER_PARAMETER};
//
//enum StoppingCondition {MIN,
//    MAX};
//
//enum Endpoint {EXCLUDED,
//    INCLUDED};
//
//// specializations of check_limit with regards to stoppingCondition and includeEndpoint for the basic min/max limits
//#define check_limit_MIN_false(val, limitVal, checkBool) checkBool = (val < limitVal);
//#define check_limit_MIN_true(val, limitVal, checkBool) checkBool = (val <= limitVal);
//#define check_limit_MAX_false(val, limitVal, checkBool) checkBool = (val > limitVal);
//#define check_limit_MAX_true(val, limitVal, checkBool) checkBool = (val >= limitVal);
//
//struct TrajectoryLimit
//{
//    LimitType type;
//    StoppingCondition stoppingCondition;
//    bool includeEndpoint;
//    int limitID;
//
//    int valueID;
//    int ivalue;
//    double dvalue;
//    int uvalue;
//};
//
//// globals
//TrajectoryLimit* tl;
//int* speciesCounts;
//double* orderParameterValues;
//
//TrajectoryLimit* setup()
//{
//    TrajectoryLimit* tl = new TrajectoryLimit[2];
//
//    tl[0].type = SPECIES;
//    tl[0].stoppingCondition = MIN;
//    tl[0].includeEndpoint = true;
//    tl[0].limitID = 0;
//    tl[0].valueID = 0;
//    tl[0].ivalue = 19;
//
//    tl[1].type = ORDER_PARAMETER;
//    tl[1].stoppingCondition = MAX;
//    tl[1].includeEndpoint = false;
//    tl[1].limitID = 1;
//    tl[1].valueID = 0;
//    tl[1].dvalue = 2.9;
//
//    speciesCounts = new int[2];
//    speciesCounts[0] = 234;
//    speciesCounts[1] = 4;
//
//    orderParameterValues = new double[1];
//    orderParameterValues[0] = 1.23;
//}
//
//bool isTrajectoryOutsideLimits()
//{
//    bool limitReached;
//    for (int i=0; i<2; i++)
//    {
//        TrajectoryLimit* limits = setup();
//        TrajectoryLimit& l = limits[i];
//        limitReached = false;
//
//        switch (l.type)
//        {
//        case NONE: throw exception(); break;
//
//        case SPECIES:
//            switch (l.stoppingCondition)
//            {
//            case MIN:
//                if (l.includeEndpoint)
//                {
//                    check_limit_MIN_true(speciesCounts[l.valueID], l.ivalue, limitReached)
//                }
//                else
//                {
//                    check_limit_MIN_false(speciesCounts[l.valueID], l.ivalue, limitReached)
//                }
//                break;
//            case MAX:
//                if (l.includeEndpoint)
//                {
//                    check_limit_MAX_true(speciesCounts[l.valueID], l.ivalue, limitReached)
//                }
//                else
//                {
//                    check_limit_MAX_false(speciesCounts[l.valueID], l.ivalue, limitReached)
//                }
//                break;
//            } break;
//
//        case ORDER_PARAMETER:
//            switch (l.stoppingCondition)
//            {
//            case MIN:
//                if (l.includeEndpoint)
//                {
//                    check_limit_MIN_true(orderParameterValues[l.valueID], l.dvalue, limitReached)
//                }
//                else
//                {
//                    check_limit_MIN_false(orderParameterValues[l.valueID], l.dvalue, limitReached)
//                }
//                break;
//            case MAX:
//                if (l.includeEndpoint)
//                {
//                    check_limit_MAX_true(orderParameterValues[l.valueID], l.dvalue, limitReached)
//                }
//                else
//                {
//                    check_limit_MAX_false(orderParameterValues[l.valueID], l.dvalue, limitReached)
//                }
//                break;
//            }
//            break;
//        default:
//            break;
//        }
//    }
//    return limitReached;
//}