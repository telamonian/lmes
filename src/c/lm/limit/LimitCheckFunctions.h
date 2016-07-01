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
#ifndef LM_LIMIT_LIMITCHECKFUNCTIONS_H_
#define LM_LIMIT_LIMITCHECKFUNCTIONS_H_

#include <functional>

#include "lm/EnumHelper.h"

namespace lm {
namespace limit {

template <TrajLimEnums::StoppingCondition sc> struct isStoppingConditionFirstOrder {static const bool value = false;};
template <> struct isStoppingConditionFirstOrder<TrajLimEnums::MIN> {static const bool value = true;};
template <> struct isStoppingConditionFirstOrder<TrajLimEnums::MAX> {static const bool value = true;};

template <TrajLimEnums::StoppingCondition sc> struct isStoppingConditionSecondOrder {static const bool value = false;};
template <> struct isStoppingConditionSecondOrder<TrajLimEnums::DECREASING> {static const bool value = true;};
template <> struct isStoppingConditionSecondOrder<TrajLimEnums::INCREASING> {static const bool value = true;};

}
}

// the main checkLimit template. Call this function when checking any values against any limits
template <TrajLimEnums::StoppingCondition sc, bool includeEndpoint> struct checkLimit;

// specializations of checkLimit with regards to stoppingCondition and includeEndpoint for the basic min/max limits
template <> struct checkLimit<TrajLimEnums::MIN, false> {template <typename T> static bool call(T val, T limitVal) {return (val <  limitVal);}};
template <> struct checkLimit<TrajLimEnums::MIN, true>  {template <typename T> static bool call(T val, T limitVal) {return (val <= limitVal);}};
template <> struct checkLimit<TrajLimEnums::MAX, false> {template <typename T> static bool call(T val, T limitVal) {return (val >  limitVal);}};
template <> struct checkLimit<TrajLimEnums::MAX, true>  {template <typename T> static bool call(T val, T limitVal) {return (val >= limitVal);}};

// specializations of checkLimit with regards to stoppingCondition and includeEndpoint for the slightly more complex decreasing/increasing limits
template <> struct checkLimit<TrajLimEnums::DECREASING, false> {template <typename T> static bool call(T prevVal, T val, T limitVal) {return (prevVal >= limitVal && val <  limitVal);}};
template <> struct checkLimit<TrajLimEnums::DECREASING, true>  {template <typename T> static bool call(T prevVal, T val, T limitVal) {return (prevVal >  limitVal && val <= limitVal);}};
template <> struct checkLimit<TrajLimEnums::INCREASING, false> {template <typename T> static bool call(T prevVal, T val, T limitVal) {return (prevVal <= limitVal && val >  limitVal);}};
template <> struct checkLimit<TrajLimEnums::INCREASING, true>  {template <typename T> static bool call(T prevVal, T val, T limitVal) {return (prevVal <  limitVal && val >= limitVal);}};

template <TrajLimEnums::StoppingCondition sc, bool includeEndpoint, typename T,
          bool isFirstOrder=lm::limit::isStoppingConditionFirstOrder<sc>::value,
          bool isSecondOrder=lm::limit::isStoppingConditionSecondOrder<sc>::value> class checkLimitCurry;

template <TrajLimEnums::StoppingCondition sc, bool includeEndpoint, typename T> class checkLimitCurry<sc, includeEndpoint, T, true, false>
{
public:
    checkLimitCurry(T limitVal): limitVal(limitVal) {}
    bool operator()(T val) {return checkLimit<sc, includeEndpoint>::call(val, limitVal);}

    checkLimitCurry<sc, includeEndpoint, T, true, false>& setLimitVal(T newLimitVal) {limitVal = newLimitVal; return *this;}

protected:
    T limitVal;
};

template <TrajLimEnums::StoppingCondition sc, bool includeEndpoint, typename T> class checkLimitCurry<sc, includeEndpoint, T, false, true>
{
public:
    checkLimitCurry(T limitVal): limitVal(limitVal) {}
    bool operator()(T prevVal, T val) {return checkLimit<sc, includeEndpoint>::call(prevVal, val, limitVal);}

    checkLimitCurry<sc, includeEndpoint, T, false, true>& setLimitVal(T newLimitVal) {limitVal = newLimitVal; return *this;}

protected:
    T limitVal;
};

template <TrajLimEnums::StoppingCondition sc, bool includeEndpoint, typename T>
std::binder2nd<bool(T, T)> _checkLimitCurry(T limitVal)
{
    return std::bind2nd(checkLimit<sc, includeEndpoint>::call, limitVal);
};

//template <TrajLimEnums::StoppingCondition sc, bool includeEndpoint, typename T, typename TIterator>
//TIterator checkLimitRangeAdapter(TIterator (*rangeBasedFuncWithPredicate)(TIterator, TIterator, bool (*predicate)(T)), TIterator start, TIterator end, T limitVal)
//{
//    // closure that allows for presetting the limitVal and calling the check with a single argument
////    struct CheckLimitClosureLocal: public checkLimit<sc, includeEndpoint>
////    {
////        static T staticLimitVal;
////
////        static bool closure(T val) {return call(val, staticLimitVal);}
////    };
////    CheckLimitClosureLocal::staticLimitVal = limitVal;
//
//    static bool localClosure(T val) {return checkLimit<sc, includeEndpoint>::call(val, limitVal);}
//
//    return (*rangeBasedFuncWithPredicate)(start, end, localClosure);
//};
//
//// closure that allows for presetting the limitVal and calling the check with a single argument
//template <TrajLimEnums::StoppingCondition sc, bool includeEndpoint, typename T> struct checkLimitClosure: public checkLimit<sc, includeEndpoint>
//{
//    checkLimitClosure() {}
//    checkLimitClosure(T limitVal): limitVal(limitVal) {}
//
//    bool closure(T val) {return call(val, limitVal);}
//    T limitVal;
//};



/*
 * some regexes to help convert the check template function to check macros
 */
// template <> struct checkLimit<EH::(\w+), (\w+)>.+return (\(.+\);).+
// define check_limit_$1_$2(val, limitVal, checkBool) checkBool = $3
// define check_limit_$1_$2(prevVal, val, limitVal, checkBool) checkBool = $3

#endif /* LM_LIMIT_LIMITCHECKFUNCTIONS_H_ */