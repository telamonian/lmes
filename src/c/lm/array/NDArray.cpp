/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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
#include <cstdlib>
#include <functional>
#include <numeric>

#include "lm/array/NDArray.h"
#include "lm/array/Tuple.h"

namespace lm {
namespace array {

// names of these functions taken from the numpy equivalents
uint ravelMultiIndex(const utuple& multiIndex, const utuple& shape)
{
    uint position=0;
    for (uint i=0; i<shape.len; i++)
    {
        uint offset=1;
        for (uint j=i+1; j<shape.len; j++)
            offset *= shape[j];
        position += multiIndex[i]*offset;
    }

    return position;
}

utuple unravelIndex(uint index, const utuple& shape)
{
    std::vector<uint> multiIndex(shape.len, 0);
    std::vector<uint> minorShapes(shape.data() + 1, shape.data() + shape.len + 1);
    // for shape->(x, y, z), the partial_sum will store (y*z, z, 0) in multiIndex
    std::partial_sum (minorShapes.rbegin(), minorShapes.rend(), multiIndex.rbegin() + 1, std::multiplies<int>());

    div_t divmod;
    for (uint i=0; i<shape.len - 1; i++)
    {
        // need static_cast<int> or else the compiler confuses the int and long versions of div
        divmod = std::div(static_cast<int>(index), static_cast<int>(multiIndex[i]));
        index = divmod.rem;
        multiIndex[i] = divmod.quot;
    }
    multiIndex.back() = index;

    return utuple(multiIndex);
}
}
}