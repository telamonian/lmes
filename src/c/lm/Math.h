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
#ifndef LM_MATH_H_
#define LM_MATH_H_

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>

#include "lm/Types.h"

/*
 * constant definitions
 */
#define TWOPI       6.283185307179586476925287
#define PI          3.141592653589793238462643
#define PID2        1.570796326794896619231322
#define PID4        0.7853981634
#define PIOVER180   0.0174532925
#define NA          6.02214179e23
#define EPS         1e-9

/*
 * prefix definitions
 */
#define KIBI        1024
#define MEBI        1048576

/*
 * unary operations
 */
inline bool isPower2(unsigned int x)
{
    return !(x&(x - 1)) && x;
}

inline bool isPower2(unsigned long x)
{
    return !(x&(x - 1)) && x;
}

inline bool isPower2(unsigned long long x)
{
    return !(x&(x - 1)) && x;
}

inline unsigned int log2(unsigned int x)
{
    unsigned int r = 0;
    while (x>>=1)
        r++;
    return r;
}

inline unsigned int log2(unsigned long x)
{
    unsigned int r = 0;
    while (x>>=1)
        r++;
    return r;
}

inline unsigned int log2(unsigned long long x)
{
    unsigned int r = 0;
    while (x>>=1)
        r++;
    return r;
}

//#include <cmath>
//// simple rounding function
//// for interesting corner cases where this function won't work, see http://stackoverflow.com/a/4572677/425458
//double round(double d)
//{
//    return (d >= 0.0) ? floor(d + 0.5) : ceil(d - 0.5);
//}

/*
 * binary operations
 */
template <typename T1, typename T2> inline T1 add(T1 val1, T2 val2)
{
    return val1 + val2;
};


template <typename T1, typename T2> inline T1 mul(T1 val1, T2 val2)
{
    return val1 * val2;
};

template <typename IntType> inline IntType ceilDiv(IntType val0, IntType val1)
{
    return (IntType)ceil(val0/(double)val1);
}

/*
 * operations on containers
 */
// product functor. ProductFunctor<T>::call(first, last) will return the product of an iterator range if T is a numeric type (returns 0 if the range is empty)
template <typename T, typename=typename EnableIf<IsNumeric<T>::value>::type> struct ProductFunctor
{
    template <typename iterT> static T call(iterT first, iterT last)
    {
        if (first==last) return static_cast<T>(0);
        else             return std::accumulate(first, last, static_cast<T>(1), std::multiplies<T>());
    }
};

//template <typename T> struct _ProductFunctor<T, true> {template <typename iterT> static T call(iterT first, iterT last) {return std::accumulate(first, last, static_cast<T>(1), std::multiplies<T>());}};   //mul);}};
//template <typename T> struct _ProductFunctor<T, false> {template <typename iterT> static T call(iterT first, iterT last) {return *first;}};
//template <typename T> struct ProductFunctor {template <typename iterT> static T call(iterT first, iterT last) {return _ProductFunctor<T, IsNumeric<T>::value>::call(first, last);}};

#ifndef __cuda_cuda_h__
using std::min;
using std::max;
/*
template <class T> inline T min(T x, T y) {return x<y?x:y;}
template <class T> inline T min(T x, T y, T z) {return x<y?(x<z?x:z):(y<z?y:z);}
template <class T> inline T max(T x, T y) {return x>y?x:y;}
template <class T> inline T max(Tt x, T y, T z) {return x>y?(x>z?x:z):(y>z?y:z);}
*/
#endif

#endif
