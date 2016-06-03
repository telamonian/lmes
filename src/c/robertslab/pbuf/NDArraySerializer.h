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

#ifndef NDARRAYSERIALIZER_H
#define NDARRAYSERIALIZER_H

#include <zlib.h>

#include "robertslab/Exceptions.h"
#include "robertslab/Types.h"
#include "robertslab/pbuf/NDArray.pb.h"

namespace robertslab {
namespace pbuf {

template <typename T> robertslab::pbuf::NDArray_DataType NDArray_datatype_code();

class NDArraySerializer
{
public:
    template <typename T> static robertslab::pbuf::NDArray* serialize(const ndarray<T>& array, bool compressData=true)
    {
        robertslab::pbuf::NDArray* msg = new robertslab::pbuf::NDArray();
        serializeInto(msg, array, compress);
        return msg;
    }

    template <typename T> static void serializeInto(robertslab::pbuf::NDArray* msg, const T* data, utuple shape, bool compressData=true)
    {
        serializeInto(msg, ndarray<T>(data, shape, false), compress);
    }

    template <typename T> static void serializeInto(robertslab::pbuf::NDArray* msg, const ndarray<T>& array, bool compressData=true)
    {
        // Set the data type.
        msg->set_data_type(NDArray_datatype_code<T>());

        // Set the shape.
        for (uint i=0; i<array.shape.len; i++)
            msg->add_shape(array.shape[i]);

        // See if we need to compress the data.
        if (compressData)
        {
#ifdef OPT_SNAPPY
            // Store with snappy compression.
            msg->set_compressed_deflate(false);
            msg->set_compressed_snappy(true);
#else
            // Store deflated.
            msg->set_compressed_deflate(true);
            msg->set_compressed_snappy(false);
            std::string* data = msg->mutable_data();
            size_t dataSizeEstimate=compressBound(array.size*sizeof(T));
            data->resize(dataSizeEstimate);
            RL_ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*data)[0]), &dataSizeEstimate, (const unsigned char*)array.values, array.size*sizeof(T)));
            data->resize(dataSizeEstimate);
#endif
        }
        else
        {
            // Store without compression.
            msg->set_compressed_deflate(false);
            msg->set_compressed_snappy(false);
            std::string* data = msg->mutable_data();
            data->resize(array.size*sizeof(T));
            memcpy((unsigned char*)&((*data)[0]), (const unsigned char*)array.values, array.size*sizeof(T));
        }
    }

    template <typename T> static ndarray<T> deserialize(const robertslab::pbuf::NDArray& msg);
};

}
}

#endif // NDARRAYSERIALIZER_H
