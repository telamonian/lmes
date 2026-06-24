/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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
#ifdef OPT_AVX

#ifndef LM_AVX_AVXMATH_H_
#define LM_AVX_AVXMATH_H_

#include <immintrin.h>
#include "lm/Types.h"

// template versions

// pairwise comparison of the doubles contained in two avxd's (pointed to by leftPtr and rightPtr)
// currently, each avxd is expected to contain DOUBLES_PER_AVX (ie 4) doubles
// the type of comparison performed is determined by comparisonOp. See GillespieDSolverAVX::isTrajectoryOutsideLimitsAVX() for examples of different comparisonOps
// comparisonOp is passed as a template parameter since the _mm256_cmp_pd macro requires this to be known at compile-time
// the result of the comparison is stored as a bitmask in trueMask (0 for false, 1 for true)
template <int comparisonOp> inline void avxGetCompareMask(double* leftPtr, double* rightPtr, int* trueMask)
{
    avxd comp;

    // load the values from the left and right arrays into avx variables
    avxd left = _mm256_load_pd(leftPtr);
    avxd right = _mm256_load_pd(rightPtr);

    // compare the values in each avx variables in a pairwise fashion
    comp = _mm256_cmp_pd(left, right, comparisonOp);

    // Get a bitmask of all values that were true.
    *trueMask = _mm256_movemask_pd(comp);
}

// overloaded version of above function that does an extra check to determine if all the values compared to false
// stores the result of that check in allFalse
template <int comparisonOp> inline void avxGetCompareMask(double* leftPtr, double* rightPtr, int* trueMask, int* allFalse)
{
    avxd comp;

    // load the values from the left and right arrays into avx variables
    avxd left = _mm256_load_pd(leftPtr);
    avxd right = _mm256_load_pd(rightPtr);

    // compare the values in each avx variables in a pairwise fashion
    comp = _mm256_cmp_pd(left, right, comparisonOp);

    // set a return variable that tells us if all compared false
    *allFalse = _mm256_testz_pd(comp, comp);

    // Get a bitmask of all values that were true.
    *trueMask = _mm256_movemask_pd(comp);
}

// macro versions

#define AVX_COMP_MASK(comparisonOp, leftPtr, rightPtr, trueMask) \
{ \
    avxd comp,left,right; \
    left=_mm256_load_pd(leftPtr); \
    right=_mm256_load_pd(rightPtr); \
    comp=_mm256_cmp_pd(left, right, comparisonOp); \
    *trueMask=_mm256_movemask_pd(comp); \
}

#define AVX_COMP_MASK_ALLFALSE(comparisonOp, leftPtr, rightPtr, trueMask, allFalse) \
{ \
    avxd comp,left,right; \
    left=_mm256_load_pd(leftPtr); \
    right=_mm256_load_pd(rightPtr); \
    comp=_mm256_cmp_pd(left, right, comparisonOp); \
    *allFalse = _mm256_testz_pd(comp, comp); \
    *trueMask=_mm256_movemask_pd(comp); \
}

#endif /* LM_AVX_AVXMATH_H_ */
#endif /* OPT_AVX */