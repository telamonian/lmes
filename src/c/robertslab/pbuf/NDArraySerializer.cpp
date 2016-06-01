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

#include <zlib.h>

#include "robertslab/Exceptions.h"
#include "robertslab/Types.h"
#include "robertslab/pbuf/NDArraySerializer.h"

namespace robertslab {
namespace pbuf{

template<> robertslab::pbuf::NDArray_DataType NDArray_datatype_code<int32_t>() {return robertslab::pbuf::NDArray_DataType_int32;}
template<> robertslab::pbuf::NDArray_DataType NDArray_datatype_code<int64_t>() {return robertslab::pbuf::NDArray_DataType_int64;}
template<> robertslab::pbuf::NDArray_DataType NDArray_datatype_code<float>() {return robertslab::pbuf::NDArray_DataType_float32;}
template<> robertslab::pbuf::NDArray_DataType NDArray_datatype_code<double>() {return robertslab::pbuf::NDArray_DataType_float64;}


template <typename T> robertslab::pbuf::NDArray* NDArraySerializer::serialize(ndarray<T> array, bool compressDeflate, bool compressSnappy)
{
    robertslab::pbuf::NDArray* msg = new robertslab::pbuf::NDArray();
    serializeInto(msg, array, compressDeflate, compressSnappy);
    return msg;
}

template <typename T> void NDArraySerializer::serializeInto(robertslab::pbuf::NDArray* msg, T* data, utuple shape, bool compressDeflate, bool compressSnappy)
{
    serializeInto(msg, ndarray<T>(data, shape, false), compressDeflate, compressSnappy);
}

template <typename T> void NDArraySerializer::serializeInto(robertslab::pbuf::NDArray* msg, ndarray<T> array, bool compressDeflate, bool compressSnappy)
{
    // Set the data type.
    msg->set_data_type(NDArray_datatype_code<T>());

    // Set the shape.
    for (uint i=0; i<array.shape.len; i++)
        msg->add_shape(array.shape[i]);

    // See if we need to compress the data.
    if (compressDeflate)
    {
        // Set the deflate flag.
        msg->set_compressed_deflate(true);

        std::string* data = msg->mutable_data();
        size_t dataSizeEstimate=compressBound(array.size*sizeof(T));
        data->resize(dataSizeEstimate);
        ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*data)[0]), &dataSizeEstimate, (const unsigned char*)array.values, array.size*sizeof(T)));
        data->resize(dataSizeEstimate);
    }
    if (compressSnappy)
    {
        msg->set_compressed_snappy(true);
    }
    else
    {
        std::string* data = msg->mutable_data();
        data->resize(array.size*sizeof(T));
        memcpy((unsigned char*)&((*data)[0]), (const unsigned char*)array.values, array.size*sizeof(T));
    }
}





}
}
