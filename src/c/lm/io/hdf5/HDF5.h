/*
 * University of Illinois Open Source License
 * Copyright 2010 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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
 * Author(s): Elijah Roberts
 */

#ifndef LM_IO_HDF5_HDF5_H_
#define LM_IO_HDF5_HDF5_H_

#include <hdf5.h>
#include <hdf5_hl.h>
#include "lm/Exceptions.h"

namespace lm {
namespace io {
namespace hdf5 {

// template based mappings to handle HDF5 <-> C++ Type conversions
//template <hid_t HDF5Type> struct CPPType;
//template <> struct CPPType<H5T_IEEE_F32LE> {typedef float T;};
//template <> struct CPPType<H5T_IEEE_F64LE> {typedef double T;};
//template <> struct CPPType<H5T_STD_I32LE> {typedef int32_t T;};
//template <> struct CPPType<H5T_STD_I64LE> {typedef int64_t T;};
//template <> struct CPPType<H5T_STD_U32LE> {typedef uint32_t T;};
//template <> struct CPPType<H5T_STD_U64LE> {typedef uint64_t T;};

// template based mappings to handle HDF5 <-> C++ Type conversions
//template <hid_t HDF5Type> struct CPPType;
//template <> struct CPPType<H5T_IEEE_F32LE_g> {typedef float T;};
//template <> struct CPPType<H5T_IEEE_F64LE_g> {typedef double T;};
//template <> struct CPPType<H5T_STD_I32LE_g> {typedef int32_t T;};
//template <> struct CPPType<H5T_STD_I64LE_g> {typedef int64_t T;};
//template <> struct CPPType<H5T_STD_U32LE_g> {typedef uint32_t T;};
//template <> struct CPPType<H5T_STD_U64LE_g> {typedef uint64_t T;};

template <typename CPPType> struct HDF5Type;
template <> struct HDF5Type<float> {static hid_t T() {return H5T_IEEE_F32LE;}};
template <> struct HDF5Type<double> {static hid_t T() {return H5T_IEEE_F64LE;}};
template <> struct HDF5Type<int32_t> {static hid_t T() {return H5T_STD_I32LE;}};
template <> struct HDF5Type<int64_t> {static hid_t T() {return H5T_STD_I64LE;}};
template <> struct HDF5Type<uint32_t> {static hid_t T() {return H5T_STD_U32LE;}};
template <> struct HDF5Type<uint64_t> {static hid_t T() {return H5T_STD_U64LE;}};

//template <typename CPPType> struct HDF5Type;
//template <> struct HDF5Type<float> {static const hid_t T = H5T_IEEE_F32LE_g;};
//template <> struct HDF5Type<double> {static const hid_t T = H5T_IEEE_F64LE_g;};
//template <> struct HDF5Type<int32_t> {static const hid_t T = H5T_STD_I32LE_g;};
//template <> struct HDF5Type<int64_t> {static const hid_t T = H5T_STD_I64LE_g;};
//template <> struct HDF5Type<uint32_t> {static const hid_t T = H5T_STD_U32LE_g;};
//template <> struct HDF5Type<uint64_t> {static const hid_t T = H5T_STD_U64LE_g;};

//template <> struct HDF5Type<float> {static const hid_t T;};
//template <> struct HDF5Type<double> {static const hid_t T;};
//template <> struct HDF5Type<int32_t> {static const hid_t T;};
//template <> struct HDF5Type<int64_t> {static const hid_t T;};
//template <> struct HDF5Type<uint32_t> {static const hid_t T;};
//template <> struct HDF5Type<uint64_t> {static const hid_t T;};
//
//const hid_t HDF5Type<float>::T = H5T_IEEE_F32LE;
//const hid_t HDF5Type<double>::T = H5T_IEEE_F64LE;
//const hid_t HDF5Type<int32_t>::T = H5T_STD_I32LE;
//const hid_t HDF5Type<int64_t>::T = H5T_STD_I64LE;
//const hid_t HDF5Type<uint32_t>::T = H5T_STD_U32LE;
//const hid_t HDF5Type<uint64_t>::T = H5T_STD_U64LE;

class HDF5Exception : public lm::Exception
{
public:
    HDF5Exception(herr_t errorCode, const char * file, const int line) : Exception("HDF5 error",(int)errorCode,file,line) {}
//    virtual ~HDF5Exception() throw() {}
    void printStackTrace() {H5Eprint2(H5E_DEFAULT,NULL);}
};

}
}
}

#define HDF5_EXCEPTION_CHECK(hdf5_call) {herr_t _hdf5_ret_=hdf5_call; if (_hdf5_ret_ < 0) throw lm::io::hdf5::HDF5Exception(_hdf5_ret_,__FILE__,__LINE__);}
#define HDF5_EXCEPTION_CALL(val,hdf5_call) val=hdf5_call; if (val < 0) {herr_t _hdf5_err_=(herr_t)val; val=0; throw lm::io::hdf5::HDF5Exception(_hdf5_err_,__FILE__,__LINE__);}



#endif
