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

// Configure the default compression algorithm.
#ifdef OPT_SNAPPY
#define DEFAULT_COMPRESSION SNAPPY
#else
#define DEFAULT_COMPRESSION DEFLATE
#endif

class NDArraySerializer
{
public:
    enum CompressionType {DEFLATE,
#ifdef OPT_SNAPPY
                          SNAPPY,
#endif
                          NONE};

public:
    template <typename T> static robertslab::pbuf::NDArray* serialize(const ndarray<T>& array, CompressionType compressionType=DEFAULT_COMPRESSION)
    {
        robertslab::pbuf::NDArray* msg = new robertslab::pbuf::NDArray();
        serializeInto(msg, array, compressionType);
        return msg;
    }

    template <typename T> static void serializeInto(robertslab::pbuf::NDArray* msg, const T* data, utuple shape, CompressionType compressionType=DEFAULT_COMPRESSION)
    {
        serializeInto(msg, ndarray<T>(shape, data, false), compressionType);
    }

    template <typename T> static void serializeInto(robertslab::pbuf::NDArray* msg, const ndarray<T>& array, CompressionType compressionType=DEFAULT_COMPRESSION)
    {
        // Set the data type.
        msg->set_data_type(NDArray_datatype_code<T>());

        // Set the shape.
        for (uint i=0; i<array.shape.len; i++)
            msg->add_shape(array.shape[i]);

        // See if we need to compress the data.
        if (compressionType == DEFLATE)
        {
            // Store deflated.
            msg->set_compressed_deflate(true);
            msg->set_compressed_snappy(false);
            std::string* data = msg->mutable_data();
            size_t dataSizeEstimate=compressBound(array.size*sizeof(T));
            data->resize(dataSizeEstimate);
            RL_ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*data)[0]), &dataSizeEstimate, (const unsigned char*)array.values, array.size*sizeof(T)));
            data->resize(dataSizeEstimate);
        }
#ifdef OPT_SNAPPY
        else if (compressionType == SNAPPY)
        {
                // Store with snappy compression.
                msg->set_compressed_deflate(false);
                msg->set_compressed_snappy(true);
        }
#endif
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

    template <typename T> static ndarray<T>* deserialize(const robertslab::pbuf::NDArray& msg, size_t alignment=0)
    {
        // Check that the datatype matches.
        if (msg.data_type() != NDArray_datatype_code<double>()) throw robertslab::InvalidArgException("msg", "the array was of the wrong data type", msg.data_type());

        // Get the shape of the ndarray.
        tuple<uint> shape(msg.shape().size(), (const uint*)msg.shape().data());

        // Allocate the ndarray.
        ndarray<T> array = new ndarray<T>(shape, alignment);

        // See if we need to decompress the data.
        if (msg.compressed_deflate())
        {
            size_t tmpBufferSize = array->size*sizeof(T)*sizeof(T);
            RL_ZLIB_EXCEPTION_CHECK(uncompress((unsigned char *)array->values, &tmpBufferSize, (unsigned char*)&(msg.data()[0]), msg.data().size()));
            if (tmpBufferSize != array->size*sizeof(T)*sizeof(T))
                throw robertslab::Exception("error during ndarray inflate deserialization, wrong number of bytes decompressed", tmpBufferSize, array->size*sizeof(T)*sizeof(T));

        }
        else if (msg.compressed_snappy())
        {
#ifdef OPT_SNAPPY
            throw robertslab::InvalidArgException("msg", "support for snappy decompression is not available");
#else
            throw robertslab::InvalidArgException("msg", "support for snappy decompression is not available");
#endif
        }
        else
        {
            if (msg.data().size() != array->size*sizeof(double)) throw robertslab::InvalidArgException("msg", "inconsistent size during ndarray deserialization", msg.data().size(), array->size);
            memcpy(array->values, (const unsigned char*)&(msg.data()[0]), array->size*sizeof(T));
        }

        return array;
    }
};

}
}

#endif // NDARRAYSERIALIZER_H
