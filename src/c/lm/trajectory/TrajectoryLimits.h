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

#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/option/SimulationParameters.h"
#include "lm/pwrap/Repeated.h"
#include "lm/types.h"

namespace lm {
namespace trajectory {

// main template for LimitType->ValueType type generator
template <lm::io::TrajectoryLimits::LimitType LT> struct LimitValueT;
// NB: if any new LimitType enum values are added in the future, add a template specialization below
template <> struct LimitValueT<lm::io::TrajectoryLimits::TIME> {typedef double type;};
template <> struct LimitValueT<lm::io::TrajectoryLimits::SPECIES> {typedef int32_t type;};
template <> struct LimitValueT<lm::io::TrajectoryLimits::ORDER_PARAMETER> {typedef double type;};
template <> struct LimitValueT<lm::io::TrajectoryLimits::DEGREE_ADVANCEMENT> {typedef uint32_t type;};

struct TrajectoryLimit
{
    lm::io::TrajectoryLimits::LimitType type;
    lm::io::TrajectoryLimits::StoppingCondition stoppingCondition;
    bool includeEndpoint;
    int32_t limitID;

    uint32_t valueID;
    int32_t ivalue;
    double dvalue;
    uint64_t uvalue;
};

class TrajectoryLimits
{
public:
// constants
    static const int32_t DEFAULT_LIMIT_ID = -2;

// typedefs
    typedef lm::io::TrajectoryLimits bufType;
    typedef lm::io::TrajectoryLimits::TrajectoryLimit subBufType;

    typedef bufType::LimitType LimitType;
    typedef bufType::StoppingCondition StoppingCondition;

    typedef lm::pwrap::Repeated<subBufType> repeatedType;
    typedef vector<TrajectoryLimit> vectorType;
    typedef typename vectorType::iterator iterator;
    typedef typename vectorType::const_iterator const_iterator;
    
// constructors/destructors
    TrajectoryLimits(): nextID(0) {seatRepeated(_buf);}
    TrajectoryLimits(const bufType& inBuf): nextID(0) {rFB(inBuf); seatRepeated(_buf);}
    //    TrajectoryLimits(const lm::io::hdf5::Hdf5File& file) {rFF(file);}
    ~TrajectoryLimits() {}

// accessors
    const subBufType& getTimeBuf() const {return _buf.time_limit();}
    repeatedType::const_iterator findBuf(int32_t id) const;

    const bufType& buf() const {return _buf;}
    const repeatedType& repeated() const {return _repeated;}
    const vectorType& vec() const {return _vec;}

// mutators
    template <LimitType LT> subBufType* addLimitBuf(uint32_t valID, LimitValueT<LT>::type val, StoppingCondition sc, int32_t id=DEFAULT_LIMIT_ID, bool includeEndpoint=true);
//    template <> subBufType* addLimitBuf<bufType::TIME>(uint32_t valID, double val, StoppingCondition sc, int32_t id=DEFAULT_LIMIT_ID, bool includeEndpoint=true) {return _addLimitBuf<>(valID, val, bufType::TIME, sc, id, includeEndpoint);}
//    template <> subBufType* addLimitBuf<bufType::SPECIES>(uint32_t valID, int32_t val, StoppingCondition sc, int32_t id=DEFAULT_LIMIT_ID, bool includeEndpoint=true) {return _addLimitBuf<>(valID, val, bufType::SPECIES, sc, id, includeEndpoint);}
//    template <> subBufType* addLimitBuf<bufType::ORDER_PARAMETER>(uint32_t valID, double val, StoppingCondition sc, int32_t id=DEFAULT_LIMIT_ID, bool includeEndpoint=true) {return _addLimitBuf<>(valID, val, bufType::ORDER_PARAMETER, sc, id, includeEndpoint);}
//    template <> subBufType* addLimitBuf<bufType::DEGREE_ADVANCEMENT>(uint32_t valID, uint64_t val, StoppingCondition sc, int32_t id=DEFAULT_LIMIT_ID, bool includeEndpoint=true) {return _addLimitBuf<>(valID, val, bufType::DEGREE_ADVANCEMENT, sc, id, includeEndpoint);}
//    template <typename T> subBufType* _addLimitBuf(uint32_t valID, T val, LimitType lt, StoppingCondition sc, int32_t id=DEFAULT_LIMIT_ID, bool includeEndpoint=true);

    void seatRepeated(bufType& inBuf) {_repeated.setRepFieldPtr(inBuf.mutable_trajectory_limits());}
    void seatRepeated() {seatRepeated(_buf);}
    void setBuf(const bufType& inBuf) {_buf.CopyFrom(inBuf);}
    void setVector(vectorType& inVec) {_vec = inVec;}

    // specializing assignment to the TrajectoryLimit buffer oneof_value field via polymorphism
    subBufType* setLimitBufValue(subBufType* limitBuf, double val) {limitBuf->set_dvalue(val); return limitBuf;}
    subBufType* setLimitBufValue(subBufType* limitBuf, int32_t val) {limitBuf->set_ivalue(val); return limitBuf;}
    subBufType* setLimitBufValue(subBufType* limitBuf, uint64_t val) {limitBuf->set_uvalue(val); return limitBuf;}

// protobuf and stl container IO
    bool rFB(const bufType& inBuf);                 // rFB = read From Buf
    bool wTB(bufType& outBuf);                      // wTB = write To Buf
    bool wTV(vectorType& outVec);                   // wTV = write To Vec
    //bool rFF(const lm::io::hdf5::Hdf5File& file); // rFF = read From File

    bool rFB() {return rFB(_buf);}
    bool wTB() {return wTB(_buf);}
    bool wTV() {return wTV(_vec);}

protected:
    int32_t nextID;

    bufType _buf;
    repeatedType _repeated;
    vectorType _vec;
};

}
}

#endif /* LM_TRAJECTORY_TRAJECTORYLIMITS */
