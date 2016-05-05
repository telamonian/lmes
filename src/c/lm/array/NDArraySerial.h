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

#ifndef LM_ARRAY_NDARRAYSERIAL_H
#define LM_ARRAY_NDARRAYSERIAL_H

#include <cstring>
#include <vector>

#include "lm/array/NDArray.h"
#include "lm/array/Tuple.h"
#include "lm/Types.h"

namespace lm {
namespace array {

template <typename T>
class NDArrayResizable : public NDArray<T>
{
public:
    NDArrayResizable()
    :NDArray<T>()
    {
    }

    NDArrayResizable(const UTuple& shape)
    :NDArray<T>(),_vector(calculateNumberValues(shape))
    {
        initBaseNDArray(shape);
    }

    NDArrayResizable(const UTuple& shape, const T* valuesArray)
    :NDArray<T>(),_vector(valuesArray, valuesArray + calculateNumberValues(shape))
    {
        initBaseNDArray(shape);
    }

    NDArrayResizable(const NDArrayResizable& other)
    :NDArray<T>(),_vector(other.vector())
    {
        initBaseNDArray(other.shape(););
    }

    virtual ~NDArrayResizable()
    {
    }

    void initBaseNDArray(const UTuple& shape)
    {
        _shape = shape;
        _size = _vector.size();
        _data = _vector.data();
    }

    void reshape(const UTuple& shape)
    {

    }

    const std::vector<T>& vector() const
    {
        return _vector;
    }

protected:
    std::vector<T> _vector;
};

}
}

#endif /* LM_ARRAY_NDARRAYSERIAL_H */
