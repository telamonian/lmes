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
#ifndef LM_LIMIT_LIMITCHECKMACROS
#define LM_LIMIT_LIMITCHECKMACROS

// non-standard compliant check macro, similar to the inequality checking macros from the avx library
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

#ifdef OPT_AVX
#include <immintrin.h>
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
#endif /* OPT_AVX */

#endif /* LM_LIMIT_LIMITCHECKMACROS */