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
 * Author(s): Elijah Roberts
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <cstring>
#include <list>
#include <vector>

#include <stdint.h>
#define __STDC_LIMIT_MACROS
#include <limits.h>

#include "lm/Exceptions.h"

using std::list;
using std::vector;

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

#if defined(OPT_AVX) && !defined(__FMA__)
#define _mm256_fmadd_pd(a,b,c) _mm256_add_pd(_mm256_mul_pd(a,b),c)
#endif

/*
 *  Array types.
 */

template <typename T> const char* printf_format_string();

template <typename T> struct tuple
{
    tuple(const tuple& t)
    :len(t.len),values(new T[t.len]())
    {
        memcpy(values, t.values, sizeof(T)*len);
    }

    tuple(const T v1)
    :len(1),values(new T[len]())
    {
        values[0] = v1;
    }

    tuple(const T v1, const T v2)
    :len(2),values(new T[len]())
    {
        values[0] = v1;
        values[1] = v2;
    }

    tuple(const T v1, const T v2, const T v3)
    :len(3),values(new T[len]())
    {
        values[0] = v1;
        values[1] = v2;
        values[2] = v3;
    }

    tuple(uint len, const T* valuesArray)
    :len(len),values(new T[len]())
    {
        memcpy(values, valuesArray, sizeof(T)*len);
    }

    tuple(const list<T>& valuesList)
    :len(valuesList.size()),values(new T[len]())
    {
        int i=0;
        for (typename std::list<T>::iterator it = valuesList.begin(); it != valuesList.end(); it++)
            values[i++] = *it;
    }

    tuple(const vector<T>& valuesVector)
    :len(valuesVector.size()),values(new T[len]())
    {
        for (uint i=0; i<valuesVector.size(); i++)
            values[i] = valuesVector[i];
    }

    tuple& operator=(const tuple& t)
    {
        if (len != t.len)
           throw lm::InvalidArgException("t","both tuples during assigment must be of the same length");
        memcpy(values, t.values, sizeof(T)*len);
        return *this;
    }

    virtual ~tuple()
    {
        if (values != NULL) delete[] values; values = NULL;
    }

    const T operator[](const uint index) const
    {
        return get(index);
    }

    const T get(const uint index) const
    {
        if (index < len) return values[index];
        else throw lm::InvalidArgException("index","index exceeded length of tuple");
    }

    void print(const char* suffix="") const
    {
        printf("(");
        for (uint i=0; i<len; i++)
        {
            if (i > 0) printf(",");
            printf(printf_format_string<T>(),values[i]);
        }
        printf(")%s",suffix);
    }


public:
    const uint len;

private:
    T* values;
};

typedef tuple<uint> utuple;

template <typename T> struct ndarray
{
public:
    ndarray(const tuple<uint>& shape)
    :shape(shape),numberValues(calculateNumberValues(shape)),values(new T[numberValues]())
    {
    }

    ndarray(const tuple<uint>& shape, const T* valuesArray)
    :shape(shape),numberValues(calculateNumberValues(shape)),values(new T[numberValues]())
    {
        memcpy(values, valuesArray, sizeof(T)*numberValues);
    }

    ndarray(const ndarray& a)
    :shape(a.shape),numberValues(a.numberValues),values(new T[numberValues]())
    {
        memcpy(values, a.values, sizeof(T)*numberValues);
    }

    ndarray& operator=(const ndarray& a)
    {
        if (shape != a.shape || numberValues != a.numberValues)
           throw lm::InvalidArgException("t","both ndarrays during assigment must be of the same shape");
        memcpy(values, a.values, sizeof(T)*numberValues);
        return *this;
    }

    virtual ~ndarray()
    {
        if (values != NULL) delete[] values; values = NULL;
    }

    const T& operator[](const tuple<uint>& index) const
    {
        return const_cast<ndarray *>(this)->get(index);
    }

    T& operator[](const tuple<uint>& index)
    {
        return get(index);
    }

    T& get(const tuple<uint>& index)
    {
        // Validate the index.
        if (index.len != shape.len) throw lm::InvalidArgException("index","index tuple must have the same length as the shape of an ndarray");
        for (uint i=0; i<shape.len; i++)
            if (index[i] >= shape[i]) throw lm::InvalidArgException("index","value of index exceeded ndarry length for dimension",i,index[i],shape[i]);

        // Calculate the position.
        uint position=0;
        for (uint i=0; i<shape.len; i++)
        {
            uint offset=1;
            for (uint j=i+1; j<shape.len; j++)
                offset *= shape[j];
            position += index[i]*offset;
        }

        // Return a reference to the element.
        return values[position];
    }

    void print(const char* suffix="") const
    {
        if (shape.len == 1)
        {
            printf("[");
            for (uint i=0; i<shape[0]; i++)
            {
                if (i > 0) printf (",");
                printf(printf_format_string<T>(),(*this)[tuple<uint>(i)]);
            }
            printf("]%s",suffix);
        }
        else if (shape.len == 2)
        {
            printf("[[");
            for (uint i=0; i<shape[0]; i++)
            {
                if (i > 0) printf (" [");
                for (uint j=0; j<shape[1]; j++)
                {
                    if (j > 0) printf (",");
                    printf(printf_format_string<T>(),(*this)[tuple<uint>(i,j)]);
                }
                printf("]\n");
            }
            printf("]%s",suffix);
        }
        else if (shape.len == 3)
        {
            printf("[[[");
            for (uint k=0; k<shape[2]; k++)
            {
                if (k > 0) printf (" [[");
                for (uint i=0; i<shape[0]; i++)
                {
                    if (i > 0) printf ("  [");
                    for (uint j=0; j<shape[1]; j++)
                    {
                        if (j > 0) printf (",");
                        printf(printf_format_string<T>(),(*this)[tuple<uint>(i,j,k)]);
                    }
                    printf("]\n");
                }
                printf(" ]\n");
            }
            printf("]%s",suffix);
        }
        else
        {
            printf("[%d dimensional ndarray: %d entries]%s",shape.len,numberValues,suffix);
        }
    }

private:
    uint calculateNumberValues(tuple<uint> s)
    {
        uint r = 1U;
        for (uint i=0; i<s.len; i++)
            r *= s[i];
        return r;
    }

public:
    const tuple<uint> shape;

private:
    uint numberValues;
    T* values;
};

#endif
