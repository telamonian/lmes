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
#include <sstream>
#include <string>
#include <vector>
#include <google/protobuf/repeated_field.h>

#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Types.h"

namespace lm {
namespace array {

template <typename T> struct tuple
{
public:
    tuple()
    :len(0),_data(NULL)
    {
    }

    tuple(const tuple& t)
    :len(t.len),_data(new T[t.len]())
    {
        memcpy(_data, t._data, sizeof(T)*len);
    }

    tuple(const T v1)
    :len(1),_data(new T[len]())
    {
        _data[0] = v1;
    }

    tuple(const T v1, const T v2)
    :len(2),_data(new T[len]())
    {
        _data[0] = v1;
        _data[1] = v2;
    }

    tuple(const T v1, const T v2, const T v3)
    :len(3),_data(new T[len]())
    {
        _data[0] = v1;
        _data[1] = v2;
        _data[2] = v3;
    }

    tuple(uint len, const T* dataArray)
    :len(len),_data(new T[len]())
    {
        memcpy(_data, dataArray, sizeof(T)*len);
    }

    tuple(const std::list<T>& dataList)
    :len(dataList.size()),_data(new T[len]())
    {
        int i=0;
        for (typename std::list<T>::const_iterator it = dataList.begin(); it != dataList.end(); it++)
            _data[i++] = *it;
    }

    tuple(const std::vector<T>& dataVector)
    :len(dataVector.size()),_data(new T[len]())
    {
        for (uint i=0; i<dataVector.size(); i++)
            _data[i] = dataVector[i];
    }

    tuple(const google::protobuf::RepeatedField<T>* repFieldPtr)
    :len(repFieldPtr->size()),_data(new T[repFieldPtr->size()]())
    {
        memcpy(_data, repFieldPtr->data(), sizeof(T)*len);
    }

    virtual ~tuple()
    {
        if (_data != NULL) delete[] _data; _data = NULL;
    }
    
// operators
    tuple& operator=(const tuple<T>& t)
    {
        if (len != t.len)
            throw lm::InvalidArgException("t","both tuples during assigment must be of the same length");
        memcpy(_data, t._data, sizeof(T)*len);
        return *this;
    }

// const operators
    const T* data() const {return _data;}

    const T operator[](const uint index) const
    {
        return get(index);
    }

    bool operator!=(const tuple<T>& t) const
    {
        if (len!=t.len)
        {
            return false;
        }
        for (uint i=0; i<len; i++)
        {
            if (_data[i]!=t._data[i])
            {
                return false;
            }
        }
        return true;
    }

// accessors
    const T get(const uint index) const
    {
        if (index < len) return _data[index];
        else throw lm::InvalidArgException("index","index exceeded length of tuple");
    }

    // print contents to stdout
    void print(const char* suffix="") const
    {
        printf("(");
        for (uint i=0; i<len; i++)
        {
            if (i > 0) printf(",");
            printNumeric(_data[i]);
        }
        printf(")%s",suffix);
    }

    // print contents to a string
    std::string repr(const char* suffix="") const
    {
        std::stringstream reprStream("(");
        for (uint i=0; i<len; i++)
        {
            if (i > 0) reprStream << ',';
            reprStream << _data[i];
        }
        reprStream << ")" << suffix;
        return reprStream.str();
    }

// mutators
    // copy data from a protobuf RepeatedField to a TupleResizable
    void fromRepeated(const google::protobuf::RepeatedField<T>* repFieldPtr)
    {
        // if _data exists, deallocate it
        if (_data != NULL) delete[] _data; _data = NULL;

        // allocate _data according to the size of repFieldPtr
        _data(new T[repFieldPtr->size()]());

        // reassign .len (via a const_cast)
        const_cast<uint&>(len) = repFieldPtr->size();

        //memcpy(_data, repFieldPtr->data(), sizeof(T)*len);

        // copy the data over (using a loop instead of memcpy allows for implicit conversion of numerical types (ie int -> uint))
        for (int i=0;i<len;i++)
        {
            _data[i] = repFieldPtr->Get(i);
        }
    }

public:
    const uint len;

private:
    T* _data;
};

// Unlike the tuple constructor, the tup class factories can use type inference.
// For example, if x and y are uints, instead of tuple<uint>(x,y) you can write tup(x,y)
template <typename T> tuple<T> tup(T v1) {return tuple<T>(v1);}
template <typename T> tuple<T> tup(T v1, T v2) {return tuple<T>(v1, v2);}
template <typename T> tuple<T> tup(T v1, T v2, T v3) {return tuple<T>(v1, v2, v3);}

}
}

// export to top-level namespace
using lm::array::tuple;
typedef tuple<uint> utuple;

#endif /* LM_ARRAY_TUPLE_H */
