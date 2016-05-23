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
    template <typename T> static robertslab::pbuf::NDArray* serialize(ndarray<T> array, bool compressDeflate=false, bool compressSnappy=false);
    template <typename T> static void serializeInto(robertslab::pbuf::NDArray* msg, T* data, utuple shape, bool compressDeflate=false, bool compressSnappy=false);
    template <typename T> static void serializeInto(robertslab::pbuf::NDArray* msg, ndarray<T> array, bool compressDeflate=false, bool compressSnappy=false);
    template <typename T> static ndarray<T> deserialize(const robertslab::pbuf::NDArray& msg);
};

}
}

namespace robertslab {

class ZlibException : public Exception
{
public:
    ZlibException(const int errorNumber) : Exception("ZLib exception", errorNumber) {}
};

#define ZLIB_EXCEPTION_CHECK(zlib_call) {int _zlib_ret_=zlib_call; if (_zlib_ret_ != Z_OK) throw robertslab::ZlibException(_zlib_ret_);}

}

#endif // NDARRAYSERIALIZER_H
