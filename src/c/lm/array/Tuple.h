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
#ifndef LM_ARRAY_TUPLE_H
#define LM_ARRAY_TUPLE_H

#include <list>
#include <vector>

#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Types.h"

namespace lm {
namespace array {

template <typename T> struct Tuple
{
public:
    Tuple(const Tuple& t)
    :len(t.len),values(new T[t.len]())
    {
        memcpy(values, t.values, sizeof(T)*len);
    }

    Tuple(const T v1)
    :len(1),values(new T[len]())
    {
        values[0] = v1;
    }

    Tuple(const T v1, const T v2)
    :len(2),values(new T[len]())
    {
        values[0] = v1;
        values[1] = v2;
    }

    Tuple(const T v1, const T v2, const T v3)
    :len(3),values(new T[len]())
    {
        values[0] = v1;
        values[1] = v2;
        values[2] = v3;
    }

    Tuple(uint len, const T* valuesArray)
    :len(len),values(new T[len]())
    {
        memcpy(values, valuesArray, sizeof(T)*len);
    }

    Tuple(const std::list<T>& valuesList)
    :len(valuesList.size()),values(new T[len]())
    {
        int i=0;
        for (typename std::list<T>::const_iterator it = valuesList.begin(); it != valuesList.end(); it++)
            values[i++] = *it;
    }

    Tuple(const std::vector<T>& valuesVector)
    :len(valuesVector.size()),values(new T[len]())
    {
        for (uint i=0; i<valuesVector.size(); i++)
            values[i] = valuesVector[i];
    }

    Tuple& operator=(const Tuple<T>& t)
    {
        if (len != t.len)
            throw lm::InvalidArgException("t","both tuples during assigment must be of the same length");
        memcpy(values, t.values, sizeof(T)*len);
        return *this;
    }

    bool operator!=(const Tuple<T>& t) const
    {
        if (len!=t.len)
        {
            return false;
        }
        for (uint i=0; i<len; i++)
        {
            if (values[i]!=t.values[i])
            {
                return false;
            }
        }
        return true;
    }

    virtual ~Tuple()
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
        else throw lm::InvalidArgException("index","index exceeded length of Tuple");
    }

    void print(const char* suffix="") const
    {
        printf("(");
        for (uint i=0; i<len; i++)
        {
            if (i > 0) printf(",");
            printNumeric(values[i]);
//            printf(printf_format_string<T>(),values[i]);
        }
        printf(")%s",suffix);
    }

public:
    const uint len;

private:
    T* values;
};

// Unlike the Tuple constructor, the Tup class factories can use type inference.
// For example, if x and y are uints, instead of Tuple<uint>(x,y) you can write Tup(x,y)
template <typename T> Tuple<T> Tup(T v1) {return Tuple<T>(v1);}
template <typename T> Tuple<T> Tup(T v1, T v2) {return Tuple<T>(v1, v2);}
template <typename T> Tuple<T> Tup(T v1, T v2, T v3) {return Tuple<T>(v1, v2, v3);}

}
}

// export to top-level namespace
using lm::array::Tuple;
typedef Tuple<uint> UTuple;

#endif /* LM_ARRAY_TUPLE_H */
