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
#include "lm/io/SimulationParameters.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/option/SimulationParameters.h"
#include "lm/pwrap/Repeated.h"
#include "lm/Types.h"

namespace lm {
namespace trajectory {

typedef lm::io::TrajectoryLimits TrajectoryLimitsBuf;
typedef lm::io::TrajectoryLimits::TrajectoryLimit TrajectoryLimitBuf;

struct TrajectoryLimit
{
    EH::LimitType type;
    EH::StoppingCondition stoppingCondition;
    bool includeEndpoint;
    int32_t limitID;

    uint32_t valueID;
    int32_t ivalue;
    double dvalue;
    uint64_t uvalue;
};

// main template for LimitType->ValueType type generator
template <EH::LimitType LT> struct LimitValueT;
// NB: if any new LimitType enum values are added in the future, add a template specialization below
template <> struct LimitValueT<EH::TIME> {typedef double type;};
template <> struct LimitValueT<EH::DEGREE_ADVANCEMENT> {typedef uint64_t type;};
template <> struct LimitValueT<EH::ORDER_PARAMETER> {typedef double type;};
template <> struct LimitValueT<EH::SPECIES> {typedef int32_t type;};

template <typename ValueT, typename ContainerT> ValueT _getLimitValue(const ContainerT& tl);
template <> double _getLimitValue<double, TrajectoryLimitBuf>(const TrajectoryLimitBuf& tl) {return tl.dvalue();}
template <> int32_t _getLimitValue<int32_t, TrajectoryLimitBuf>(const TrajectoryLimitBuf& tl) {return tl.ivalue();}
template <> uint64_t _getLimitValue<uint64_t, TrajectoryLimitBuf>(const TrajectoryLimitBuf& tl) {return tl.uvalue();}
template <> double _getLimitValue<double, TrajectoryLimit>(const TrajectoryLimit& tl) {return tl.dvalue;}
template <> int32_t _getLimitValue<int32_t, TrajectoryLimit>(const TrajectoryLimit& tl) {return tl.ivalue;}
template <> uint64_t _getLimitValue<uint64_t, TrajectoryLimit>(const TrajectoryLimit& tl) {return tl.uvalue;}
template <EH::LimitType LT, typename ContainerT> typename LimitValueT<LT>::type getLimitValue(const ContainerT& tl) {return _getLimitValue<LimitValueT<LT>::type, ContainerT>(tl);}

template <typename ValueT, typename ContainerT> void _setLimitValue(ContainerT& tl, ValueT val);
template <> void _setLimitValue<double, TrajectoryLimitBuf>(TrajectoryLimitBuf& tl, double val) {tl.set_dvalue(val);}
template <> void _setLimitValue<int32_t, TrajectoryLimitBuf>(TrajectoryLimitBuf& tl, int32_t val) {tl.set_ivalue(val);}
template <> void _setLimitValue<uint64_t, TrajectoryLimitBuf>(TrajectoryLimitBuf& tl, uint64_t val) {tl.set_uvalue(val);}
template <> void _setLimitValue<double, TrajectoryLimit>(TrajectoryLimit& tl, double val) {tl.dvalue = val;}
template <> void _setLimitValue<int32_t, TrajectoryLimit>(TrajectoryLimit& tl, int32_t val) {tl.ivalue = val;}
template <> void _setLimitValue<uint64_t, TrajectoryLimit>(TrajectoryLimit& tl, uint64_t val) {tl.uvalue = val;}
template <EH::LimitType LT, typename ContainerT> void setLimitValue(ContainerT& tl, typename LimitValueT<LT>::type val) {_setLimitValue<LimitValueT<LT>::type, ContainerT>(tl, val);}
        
class TrajectoryLimits
{
public:
// constants
    static const int32_t TIME_LIMIT_ID = -1;
    static const int32_t DEFAULT_LIMIT_ID = -2;

// typedefs
    typedef lm::pwrap::Repeated<TrajectoryLimitBuf> repeatedType;
    typedef vector<TrajectoryLimit> vectorType;
    typedef vectorType::iterator iterator;
    typedef vectorType::const_iterator const_iterator;
    
// constructors/destructors
    TrajectoryLimits(): nextID(0) {seatRepeated(_buf);}
    TrajectoryLimits(const TrajectoryLimitsBuf& inBuf): nextID(0) {rFB(inBuf);}
    //    TrajectoryLimits(const lm::io::hdf5::Hdf5File& file) {rFF(file);}
    ~TrajectoryLimits() {}

// accessors
    const TrajectoryLimitBuf& getTimeBuf() const {return _buf.time_limit();}
    double getTimeLimitValue() const {return _buf.has_time_limit() ? _buf.time_limit().dvalue() : std::numeric_limits<double>::infinity();}
    repeatedType::const_iterator findBuf(int32_t id) const;

    const TrajectoryLimitsBuf& buf() const {return _buf;}
    const repeatedType& repeated() const {return _repeated;}
    const vectorType& vec() const {return _vec;}

// mutators
    template <EH::LimitType LT> TrajectoryLimitBuf* addLimitBuf(uint32_t valID, typename LimitValueT<LT>::type val, EH::StoppingCondition sc, bool includeEndpoint=true, int32_t id=DEFAULT_LIMIT_ID);

    void seatRepeated(TrajectoryLimitsBuf& inBuf) {_repeated.setRepFieldPtr(inBuf.mutable_trajectory_limits());}
    void seatRepeated() {seatRepeated(_buf);}
    void setBuf(const TrajectoryLimitsBuf& inBuf) {_buf.CopyFrom(inBuf);}
    void setVector(vectorType& inVec) {_vec = inVec;}

    // specializing assignment to the TrajectoryLimit buffer oneof_value field via polymorphism
    TrajectoryLimitBuf* setLimitBufValue(TrajectoryLimitBuf* limitBuf, double val) {limitBuf->set_dvalue(val); return limitBuf;}
    TrajectoryLimitBuf* setLimitBufValue(TrajectoryLimitBuf* limitBuf, int32_t val) {limitBuf->set_ivalue(val); return limitBuf;}
    TrajectoryLimitBuf* setLimitBufValue(TrajectoryLimitBuf* limitBuf, uint64_t val) {limitBuf->set_uvalue(val); return limitBuf;}

// protobuf and stl container IO
    void rFB(const TrajectoryLimitsBuf& inBuf);     // rFB = read From Buf
    void wTB(TrajectoryLimitsBuf& outBuf);          // wTB = write To Buf
    void wTV(vectorType& outVec);                   // wTV = write To Vec
    //void rFF(const lm::io::hdf5::Hdf5File& file); // rFF = read From File

    void rFB() {return rFB(_buf);}
    void wTB() {return wTB(_buf);}
    void wTV() {return wTV(_vec);}

// const qualified pass-throughs to the underlying buf and stl container
    vectorType::size_type size() const {return _vec.size();}

// static functions to do TrajectoryLimit buf <-> TrajectoryLimit struct conversion
    static TrajectoryLimit bufToStruct(const TrajectoryLimitBuf& inBuf);
    static TrajectoryLimitBuf structToBuf(const TrajectoryLimit& inStruct);

protected:
    int32_t nextID;

    TrajectoryLimitsBuf _buf;
    repeatedType _repeated;
    vectorType _vec;
};

}
}

#endif /* LM_TRAJECTORY_TRAJECTORYLIMITS */
