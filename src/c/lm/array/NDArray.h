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

#include <stdlib.h>

#include "lm/array/Tuple.h"
#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/protowrap/NDArray.h"
#include "lm/Types.h"
#include "robertslab/pbuf/NDArray.pb.h"

namespace lm {
namespace array {

// names of these functions taken from the numpy equivalents
uint ravelMultiIndex(const UTuple& multiIndex, const UTuple& shape);
UTuple unravelIndex(uint index, const UTuple& shape);

template <typename T> struct NDArray
{
public:
    NDArray()
    :_shape(),_size(0),_data(NULL)
    {
    }
    
    NDArray(const Tuple<uint>& shape)
    :_shape(shape),_size(calculateNumberValues(shape)),_data(new T[_size]())
    {
    }

    NDArray(const Tuple<uint>& shape, const T* valuesArray)
    :_shape(shape),_size(calculateNumberValues(shape)),_data(new T[_size]())
    {
        memcpy(_data, valuesArray, sizeof(T)*_size);
    }

    NDArray(const NDArray& a)
    :_shape(a._shape),_size(a._size),_data(new T[_size]())
    {
        memcpy(_data, a._data, sizeof(T)*_size);
    }

    virtual ~NDArray()
    {
        if (_data != NULL) delete[] _data; _data = NULL;
    }

// operators
    NDArray& operator=(const NDArray& a)
    {
        if (_shape != a._shape || _size != a._size)
            throw lm::InvalidArgException("t","both ndarrays during assigment must be of the same shape");
        memcpy(_data, a._data, sizeof(T)*_size);
        return *this;
    }

    // NDArray can be indexed with a Tuple of the appropriate length..
    const T& operator[](const Tuple<uint>& index) const
    {
        return get(index);
    }

    T& operator[](const Tuple<uint>& index)
    {
        return get(index);
    }

    // ...or with a single index (this version flattens the array)...
    const T& operator[](uint i1) const {return _data[i1];}
    T& operator[](uint i1) {return _data[i1];}

    // ...or with multiple indices (although now we have to use operator() because operator[] complains about too many arguments)
    const T& operator()(uint i1) const {return get(i1);}
    const T& operator()(uint i1, uint i2) const {return get(i1, i2);}
    const T& operator()(uint i1, uint i2, uint i3) const {return get(i1, i2, i3);}

    T& operator()(uint i1) {return get(i1);}
    T& operator()(uint i1, uint i2) {return get(i1, i2);}
    T& operator()(uint i1, uint i2, uint i3) {return get(i1, i2, i3);}

// accessors
    const T* data() {return _data;}

    const T& get(const Tuple<uint>& index) const
    {
        // Validate the index.
        if (index.len != _shape.len) throw lm::InvalidArgException("index","index Tuple must have the same length as the shape of an NDArray");
        for (uint i=0; i<_shape.len; i++)
            if (index[i] >= _shape[i]) throw lm::InvalidArgException("index","value of index exceeded ndarry length for dimension",i,index[i],_shape[i]);

        // Calculate the position.
        uint position=0;
        for (uint i=0; i<_shape.len; i++)
        {
            uint offset=1;
            for (uint j=i+1; j<_shape.len; j++)
                offset *= _shape[j];
            position += index[i]*offset;
        }

        // Return a reference to the element.
        return _data[position];
    }
    const T& get(uint i1) const {return get(Tup(i1));}
    const T& get(uint i1, uint i2) const {return get(Tup(i1,i2));}
    const T& get(uint i1, uint i2, uint i3) const {return get(Tup(i1,i2,i3));}

    robertslab::pbuf::NDArray_DataType inferDType() const
    {
        return lm::protowrap::NDType<T>::T;
    }

    void print(const char* suffix="") const
    {
        if (_shape.len == 1)
        {
            printf("[");
            for (uint i=0; i<_shape[0]; i++)
            {
                if (i > 0) printf (",");
                printNumeric((*this)[Tup(i)]);
//                printf(printf_format_string<T>(),(*this)[Tuple<uint>(i)]);
            }
            printf("]%s",suffix);
        }
        else if (_shape.len == 2)
        {
            printf("[[");
            for (uint i=0; i<_shape[0]; i++)
            {
                if (i > 0) printf (" [");
                for (uint j=0; j<_shape[1]; j++)
                {
                    if (j > 0) printf (",");
                    printNumeric((*this)[Tup(i,j)]);
//                    printf(printf_format_string<T>(),(*this)[Tuple<uint>(i,j)]);
                }
                printf("]\n");
            }
            printf("]%s",suffix);
        }
        else if (_shape.len == 3)
        {
            printf("[[[");
            for (uint k=0; k<_shape[2]; k++)
            {
                if (k > 0) printf (" [[");
                for (uint i=0; i<_shape[0]; i++)
                {
                    if (i > 0) printf ("  [");
                    for (uint j=0; j<_shape[1]; j++)
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
            printf("[%d dimensional NDArray: %d entries]%s",_shape.len,_size,suffix);
        }
    }

    uint rank() const
    {
        return _shape.len;
    }

    uint ravelMultiIndex(const UTuple& multiIndex) const
    {
        return lm::array::ravelMultiIndex(multiIndex, _shape);
    }

    void serialize(robertslab::pbuf::NDArray* ndArrMsg, bool compressed=true) const
    {
        lm::protowrap::NDArray<T> ndArrWrap(ndArrMsg);
        ndArrWrap.set_array(_shape, _data, inferDType(), compressed);
    }

    const UTuple& shape() const {
        return _shape;
    }

    uint shape(uint index) const {
        return _shape[index];
    }

    UTuple unravelIndex(uint index) const
    {
        return lm::array::unravelIndex(index, _shape);
    }

// mutators
    void deserialize(robertslab::pbuf::NDArray* ndArrMsg)
    {
        // set new shape
        _shape.fromRepeated(&ndArrMsg->shape());
        _size = calculateNumberValues(_shape);

        // deallocate main array memory, if set

        // reallocate main array memory

        // deallocate main array memory, if set
        if (_data != NULL) delete[] _data; _data = NULL;

        // reallocate main array according to new shape
        _data(new T[_size]());

        // actual deserialization step
        lm::protowrap::NDArray<T> ndArrWrap(ndArrMsg);
        ndArrWrap.get_data(_data);
    }

    T& get(const Tuple<uint>& index)
    {
        return const_cast<T&>(const_cast<const NDArray*>(this)->get(index));
    }
    T& get(uint i1) {return get(Tup(i1));}
    T& get(uint i1, uint i2) {return get(Tup(i1,i2));}
    T& get(uint i1, uint i2, uint i3) {return get(Tup(i1,i2,i3));}

    T* mutable_data() {return _data;}

protected:
    uint calculateNumberValues(Tuple<uint> s)
    {
        uint r = 1U;
        for (uint i=0; i<s.len; i++)
            r *= s[i];
        return r;
    }

protected:
    Tuple<uint> _shape;
    uint _size;
    T* _data;
};

}
}

// export to top-level namespace
using lm::array::NDArray;

#endif /* LM_ARRAY_NDARRAY */
