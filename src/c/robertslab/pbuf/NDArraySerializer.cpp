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

}
}
