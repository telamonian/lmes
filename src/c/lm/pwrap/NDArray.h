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
#ifndef LM_PWRAP_NDARRAY
#define LM_PWRAP_NDARRAY

#include <string>
#include <vector>
#include <zlib.h>

#include "lm/Types.h"
#include "lm/pwrap/Repeated.h"
#include "robertslab/pbuf/NDArray.pb.h"

namespace lm {
namespace pwrap {

typedef robertslab::pbuf::NDArray_ArrayOrder ArrayOrder;
typedef robertslab::pbuf::NDArray_ByteOrder ByteOrder;
typedef robertslab::pbuf::NDArray_DataType DataType;

template <typename T>
class NDArray
{
public:
    NDArray(): buf(NULL) {}
    NDArray(robertslab::pbuf::NDArray* newBuf): buf(NULL) {setBuf(newBuf);}
    ~NDArray() {}

// pass throughs
// accessors
    ArrayOrder array_order() const {return buf->array_order();}
    ByteOrder byte_order() const {return buf->byte_order();}
    DataType data_type() const {return buf->data_type();}
    Repeated<int32_t>& shape() {return shape_;}
    const Repeated<int32_t>& shape(int index) const {return shape_.Get(index);}
    const std::string& data() const {return buf->data();}
    bool compressed_deflate() const {return buf->compressed_deflate();}

    Repeated<int32_t>* mutable_shape() {return &shape_;}
    std::string* mutable_data() {return buf->mutable_data();}

// mutators
    void set_array_order(ArrayOrder value) {buf->set_array_order(value);}
    void set_byte_order(ByteOrder value) {buf->set_byte_order(value);}
    void set_data_type(DataType value) {buf->set_data_type(value);}
    void set_shape(int index, const int32_t& value) {shape_.Set(index, value);}
//    void set_data(std::string& value) {buf->set_data(value);}
    void set_compressed_deflate(bool value) {buf->set_compressed_deflate(value);}


// wrapper functions
// accessors

// mutators
    inline void set_data(std::vector<T>& value, DataType dtype, bool compressed=true)
    {
        set_data_type(dtype);
        size_t dataSizeEstimate=compressBound(value.size()*sizeof(T));
        mutable_data()->resize(dataSizeEstimate);
        ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*mutable_data())[0]), &dataSizeEstimate, (unsigned char*)value.data(), value.size()*sizeof(T)));
        mutable_data()->resize(dataSizeEstimate);
    }

    inline void setBuf(robertslab::pbuf::NDArray* newBuf) {buf=newBuf;
        shape_.setRepFieldPtr(buf->mutable_shape());}

public:
    robertslab::pbuf::NDArray* buf;
protected:
    Repeated<int32_t> shape_;
};

}
}

#endif /* LM_PWRAP_NDARRAY */
