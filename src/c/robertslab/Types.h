/*
 * Copyright 2016 Johns Hopkins University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Author(s): Elijah Roberts
 */

#ifndef ROBERTSLAB_TYPES_H
#define ROBERTSLAB_TYPES_H

#include <list>
#include <stdexcept>
#include <vector>

#include <cstdio>
#include <cstring>

using std::invalid_argument;
using std::list;
using std::vector;

typedef unsigned int uint;

/*
 *  Array types.
 */

template <typename T> const char* printf_format_string();

template <typename T> struct tuple
{
    tuple(const tuple& t)
    :len(t.len),values(new T[t.len])
    {
        memcpy(values, t.values, sizeof(T)*len);
    }

    tuple(const T v1)
    :len(1),values(new T[len])
    {
        values[0] = v1;
    }

    tuple(const T v1, const T v2)
    :len(2),values(new T[len])
    {
        values[0] = v1;
        values[1] = v2;
    }

    tuple(const T v1, const T v2, const T v3)
    :len(3),values(new T[len])
    {
        values[0] = v1;
        values[1] = v2;
        values[2] = v3;
    }

    tuple(uint len, const T* valuesArray)
    :len(len),values(new T[len])
    {
        memcpy(values, valuesArray, sizeof(T)*len);
    }

    tuple(const list<T>& valuesList)
    :len(valuesList.size()),values(new T[len])
    {
        int i=0;
        for (typename std::list<T>::iterator it = valuesList.begin(); it != valuesList.end(); it++)
            values[i++] = *it;
    }

    tuple(const vector<T>& valuesVector)
    :len(valuesVector.size()),values(new T[len])
    {
        for (uint i=0; i<valuesVector.size(); i++)
            values[i] = valuesVector[i];
    }

    tuple& operator=(const tuple<T>& t)
    {
        if (len != t.len)
           throw invalid_argument("t: both tuples during assigment must be of the same length");
        memcpy(values, t.values, sizeof(T)*len);
        return *this;
    }

    bool operator==(const tuple<T>& t) const
    {
        if (len != t.len)
        {
            return false;
        }
        for (uint i=0; i<len; i++)
        {
            if (values[i] != t.values[i])
            {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const tuple<T>& t) const
    {
        return !(*this== t);
    }

    virtual ~tuple()
    {
        if (values != NULL) delete[] values;
    }

    const T operator[](const uint index) const
    {
        return get(index);
    }

    const T get(const uint index) const
    {
        if (index < len) return values[index];
        else throw invalid_argument("index: index exceeded length of tuple");
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
    T* const values;
};

typedef tuple<uint> utuple;

template <typename T> struct ndarray
{
public:
    ndarray(const tuple<uint>& shape, size_t alignment=0)
    :shape(shape),size(calculateSize(shape)),alignment(alignment),values(allocateMemory(size,alignment)),allocatedValues(true)
    {
        memset(values, 0, sizeof(T)*size);
    }

    ndarray(const tuple<uint>& shape, const T* valuesArray, size_t alignment=0)
    :shape(shape),size(calculateSize(shape)),alignment(alignment),values(allocateMemory(size,alignment)),allocatedValues(true)
    {
        memcpy(values, valuesArray, sizeof(T)*size);
    }

    ndarray(const tuple<uint>& shape, T* valuesArray, size_t alignment=0, bool copyValues=true)
    :shape(shape),size(calculateSize(shape)),alignment(alignment),values(copyValues?allocateMemory(size,alignment):valuesArray),allocatedValues(copyValues)
    {
        if (allocatedValues)
            memcpy(values, valuesArray, sizeof(T)*size);
    }

    ndarray(const ndarray& a)
    :shape(a.shape),size(a.size),alignment(a.alignment),values(allocateMemory(size,alignment)),allocatedValues(true)
    {
        memcpy(values, a.values, sizeof(T)*size);
    }

    ndarray& operator=(const ndarray& a)
    {
        if (shape != a.shape) invalid_argument("a: both ndarrays during assigment must be of the same shape");
        if (size != a.size) invalid_argument("a: both ndarrays during assigment must have the same size");
        memcpy(values, a.values, sizeof(T)*size);
        return *this;
    }

    ndarray& operator=(const T& v)
    {
        for (uint i=0; i<size; i++)
            values[i] = v;
        return *this;
    }

    virtual ~ndarray()
    {
        if (allocatedValues && values != NULL) free(values);
    }

    const T& operator[](const tuple<uint>& index) const
    {
        return (const_cast<ndarray *>(this))->get(index);
    }

    T& operator[](const tuple<uint>& index)
    {
        return get(index);
    }

    T& get(const tuple<uint>& index)
    {
        // Validate the index.
        if (index.len != shape.len) throw invalid_argument("index: index tuple must have the same length as the shape of an ndarray");
        for (uint i=0; i<shape.len; i++)
            if (index[i] >= shape[i]) throw invalid_argument("index: value of index exceeded ndarry length for the dimension");

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
            printf("[%d dimensional ndarray: %d entries]%s",shape.len,size,suffix);
        }
    }

private:
    uint calculateSize(tuple<uint> s)
    {
        uint r = 1U;
        for (uint i=0; i<s.len; i++)
            r *= s[i];
        return r;
    }

    T* allocateMemory(uint size, size_t alignment)
    {
        if (alignment > 0)
        {
            T* tmp;
            int _posix_ret_=posix_memalign((void**)&tmp, alignment*sizeof(T), size*sizeof(T));
            if (_posix_ret_ != 0) throw invalid_argument("alignment: could not allocate aligned memory");
            return tmp;
        }
        return (T*)malloc(size*sizeof(T));
    }

public:
    const tuple<uint> shape;
    const size_t size;
    const size_t alignment;
    T* const values;

private:
    bool allocatedValues;
};

#endif // ROBERTSLAB_TYPES_H


