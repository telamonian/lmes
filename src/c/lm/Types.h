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
#ifndef TYPES_H_
#define TYPES_H_

#include <cstring>
#include <list>
#include <map>
#include <utility>
#include <vector>

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <limits>

#include "lm/Exceptions.h"

/*
 * General types.
 */

typedef unsigned char       uchar;
typedef unsigned int        uint;
typedef unsigned long       ulong;

typedef long                intv_t;
typedef unsigned long       uintv_t;

typedef uint8_t 		    byte;

/*
 * Physical types.
 */
typedef double              si_dist_t;
typedef double              si_time_t;

/*
 * AVX types.
 */
#ifdef OPT_AVX
#include <immintrin.h>
#define DOUBLES_PER_AVX 4
#define INT32S_PER_AVX 8
#define avxd __m256d
#define avxi __m256i
#endif

#if defined(OPT_AVX) && !defined(OPT_FMA)
#define _mm256_fmadd_pd(a,b,c) _mm256_add_pd(_mm256_mul_pd(a,b),c)
#endif

/*
 *  Array types.
 */

template<typename T0, typename T1> struct PairVector
{
    typedef std::pair<T0, T1> Pair;
    typedef std::vector<Pair> T;
    typedef typename T::iterator iterator;
    typedef typename T::const_iterator const_iterator;

    T vec;
};

template<typename Key0, typename Key1, typename Value> struct PairMap
{
    typedef std::pair<Key0, Key1> Key;
    typedef std::map<Key, Value> T;
    typedef typename T::iterator iterator;
    typedef typename T::const_iterator const_iterator;

    T map;
};

/*
 * type traits
 */
// template for IsNumeric type testing utility. For numeric types, IsNumeric<T>::value will be true
template <typename T> struct IsNumeric {static const bool value = std::numeric_limits<T>::is_specialized;};

template<bool cond, class T=int> struct EnableIf { typedef T type; };
template<class T> struct EnableIf<false, T> {};

template<typename B, typename D>
struct IsBaseOf {
    typedef char (&yes)[1];
    typedef char (&no)[2];

#if defined(MACOSX)
#undef check
#endif

    static yes check(const B*);
    static no check(const void*);

    enum {
        value = sizeof(check(static_cast<const D*>(NULL))) == sizeof(yes),
    };
};

/*
template<typename T> struct disable_if_void {template <typename This, typename Func> static T* call(This* _this, Func func) {return (*_this.*func)();}};
template<> struct disable_if_void<void> {template <typename This, typename Func> static void* call(This* _this, Func func) {return NULL;}};

template<typename T, typename U> struct IsSame {static const bool value = false;};
template<typename T> struct IsSame<T, T> {static const bool value = true;};
 */

/*
 * misc
 */
#ifndef CPP98
#define CPP98 __cplusplus <= 199711L
#endif

#ifndef CPP11
#define CPP11 __cplusplus > 199711L
#endif

#endif
