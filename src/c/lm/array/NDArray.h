/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
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
#ifndef LM_ARRAY_NDARRAY_H
#define LM_ARRAY_NDARRAY_H

#include "lm/array/Tuple.h"
#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Types.h"

namespace lm {
namespace array {

template <typename T> struct NDArray
{
public:
    NDArray(const Tuple<uint>& shape)
    :shape(shape),numberValues(calculateNumberValues(shape)),values(new T[numberValues]())
    {
    }

    NDArray(const Tuple<uint>& shape, const T* valuesArray)
    :shape(shape),numberValues(calculateNumberValues(shape)),values(new T[numberValues]())
    {
        memcpy(values, valuesArray, sizeof(T)*numberValues);
    }

    NDArray(const NDArray& a)
    :shape(a.shape),numberValues(a.numberValues),values(new T[numberValues]())
    {
        memcpy(values, a.values, sizeof(T)*numberValues);
    }

    NDArray& operator=(const NDArray& a)
    {
        if (shape != a.shape || numberValues != a.numberValues)
            throw lm::InvalidArgException("t","both ndarrays during assigment must be of the same shape");
        memcpy(values, a.values, sizeof(T)*numberValues);
        return *this;
    }

    virtual ~NDArray()
    {
        if (values != NULL) delete[] values; values = NULL;
    }

    const T& operator[](const Tuple<uint>& index) const
    {
        return const_cast<NDArray *>(this)->get(index);
    }

    T& operator[](const Tuple<uint>& index)
    {
        return get(index);
    }

    T& get(const Tuple<uint>& index)
    {
        // Validate the index.
        if (index.len != shape.len) throw lm::InvalidArgException("index","index Tuple must have the same length as the shape of an NDArray");
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
                printNumeric((*this)[Tup(i)]);
//                printf(printf_format_string<T>(),(*this)[Tuple<uint>(i)]);
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
                    printNumeric((*this)[Tup(i,j)]);
//                    printf(printf_format_string<T>(),(*this)[Tuple<uint>(i,j)]);
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
                        printNumeric((*this)[Tup(i,j,k)]);
//                        printf(printf_format_string<T>(),(*this)[Tuple<uint>(i,j,k)]);
                    }
                    printf("]\n");
                }
                printf(" ]\n");
            }
            printf("]%s",suffix);
        }
        else
        {
            printf("[%d dimensional NDArray: %d entries]%s",shape.len,numberValues,suffix);
        }
    }

private:
    uint calculateNumberValues(Tuple<uint> s)
    {
        uint r = 1U;
        for (uint i=0; i<s.len; i++)
            r *= s[i];
        return r;
    }

public:
    const Tuple<uint> shape;

private:
    uint numberValues;
    T* values;
};

}
}

// export to top-level namespace
using lm::array::NDArray;

#endif /* LM_ARRAY_NDARRAY */
