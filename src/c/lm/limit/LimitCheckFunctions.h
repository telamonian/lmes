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
#ifndef LM_LIMIT_LIMITCHECKFUNCTIONS
#define LM_LIMIT_LIMITCHECKFUNCTIONS

#include "lm/EnumHelper.h"

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

template <TrajLimEnums::StoppingCondition sc, bool includeEndpoint, typename T, typename TIterator>
TIterator checkLimitRangeAdapter(TIterator (* rangeBasedFuncWithPredicate)(TIterator, TIterator, bool (* predicate)(T)), TIterator start, TIterator end, T limitVal)
{
    // closure that allows for presetting the limitVal and calling the check with a single argument
    struct checkLimitClosureLocal: public checkLimit<sc, includeEndpoint>
    {
        static bool closure(T val) {return call(val, limitVal);}
    };

    return rangeBasedFuncWithPredicate(start, end, checkLimitClosureLocal::closure);
};

// closure that allows for presetting the limitVal and calling the check with a single argument
template <TrajLimEnums::StoppingCondition sc, bool includeEndpoint, typename T> struct checkLimitClosure: public checkLimit<sc, includeEndpoint>
{
    checkLimitClosure() {}
    checkLimitClosure(T limitVal): limitVal(limitVal) {}

    bool closure(T val) {return call(val, limitVal);}
    T limitVal;
};



/*
 * some regexes to help convert the check template function to check macros
 * /
// template <> struct checkLimit<EH::(\w+), (\w+)>.+return (\(.+\);).+
// define check_limit_$1_$2(val, limitVal, checkBool) checkBool = $3
// define check_limit_$1_$2(prevVal, val, limitVal, checkBool) checkBool = $3

#endif /* LM_LIMIT_LIMITCHECKFUNCTIONS */