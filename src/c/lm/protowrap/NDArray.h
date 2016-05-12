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

#include <deque>
#include <memory>
#include <string>
#include <vector>
#include <zlib.h>

#include "lm/array/Tuple.h"
#include "lm/io/hdf5/HDF5.h"
#include "lm/protowrap/Repeated.h"
#include "lm/Types.h"
#include "robertslab/pbuf/NDArray.pb.h"

namespace lm {
namespace protowrap {

typedef robertslab::pbuf::NDArray_ArrayOrder ArrayOrder;
typedef robertslab::pbuf::NDArray_ByteOrder ByteOrder;
typedef robertslab::pbuf::NDArray_DataType DataType;

// template based mapping to handle NumPy -> C++ Type conversions
template <DataType NPDType> struct CPPType;
// NB: the various NPDTypes correspond to the DataType enum in robertslab::pbuf::NDArray
template <> struct CPPType<robertslab::pbuf::NDArray::float32> {typedef float T;};
template <> struct CPPType<robertslab::pbuf::NDArray::float64> {typedef double T;};
template <> struct CPPType<robertslab::pbuf::NDArray::int32> {typedef int32_t T;};
template <> struct CPPType<robertslab::pbuf::NDArray::int64> {typedef int64_t T;};
template <> struct CPPType<robertslab::pbuf::NDArray::uint32> {typedef uint32_t T;};
template <> struct CPPType<robertslab::pbuf::NDArray::uint64> {typedef uint64_t T;};

// handling strings with NDArray is going to be... complicated. I'm putting implementation on indefinite hold
//template <> struct CPPType<robertslab::pbuf::NDArray::S128> {typedef char* T;};

// template based mapping to handle C++ -> NumPy Type conversions
template <typename CPPDType> struct NDType;
// NB: the various NPDTypes correspond to the DataType enum in robertslab::pbuf::NDArray
template <> struct NDType<float> {static const DataType T = robertslab::pbuf::NDArray::float32;};
template <> struct NDType<double> {static const DataType T = robertslab::pbuf::NDArray::float64;};
template <> struct NDType<int32_t> {static const DataType T = robertslab::pbuf::NDArray::int32;};
template <> struct NDType<int64_t> {static const DataType T = robertslab::pbuf::NDArray::int64;};
template <> struct NDType<uint32_t> {static const DataType T = robertslab::pbuf::NDArray::uint32;};
template <> struct NDType<uint64_t> {static const DataType T = robertslab::pbuf::NDArray::uint64;};

//template <DataType NDType> struct HDF5Type {static const hid_t T = lm::io::hdf5::HDF5Type<CPPType<NDType>>::T;};
//
//hid_t ndTypeToHDF5Type(const DataType NDType)
//{
//    switch (NDType)
//    {
//    case robertslab::pbuf::NDArray::float32: return HDF5Type<robertslab::pbuf::NDArray::float32>::T;
//    case robertslab::pbuf::NDArray::float64: return HDF5Type<robertslab::pbuf::NDArray::float64>::T;
//    case robertslab::pbuf::NDArray::int32:   return HDF5Type<robertslab::pbuf::NDArray::int32>::T;
//    case robertslab::pbuf::NDArray::int64:   return HDF5Type<robertslab::pbuf::NDArray::int64>::T;
//    case robertslab::pbuf::NDArray::uint32:  return HDF5Type<robertslab::pbuf::NDArray::uint32>::T;
//    case robertslab::pbuf::NDArray::uint64:  return HDF5Type<robertslab::pbuf::NDArray::uint64>::T;
//    }
//}

template <typename T>
class NDArray
{
public:
    NDArray(): arrMsg(NULL) {}
    NDArray(robertslab::pbuf::NDArray* newBuf): arrMsg(NULL) {setMsgPtr(newBuf);}
    ~NDArray() {}

// accessors
    uint rank() const {return shape().size();}
    uint32_t size() const {return shape().product();}
    size_t sizeBytes() const {return size()*sizeof(T);}

// mutators
    // array version
    // call this method like this
        // data = new T[ndarray.size()];
        // ndarray.get_data(data);
        // ...
        // delete[] data;
    inline void get_data(T* outputArray)
    {
        if (compressed_deflate())
        {
            size_t countsSize = sizeBytes();
            ZLIB_EXCEPTION_CHECK(uncompress((unsigned char *)outputArray, &countsSize, (unsigned char*)&(data()[0]), data().size()));
            if (countsSize != sizeBytes())
                throw Exception("Error during data decompression, wrong number of bytes returned.");
        }
        else
        {
            memcpy(outputArray, (T*) &(data()[0]), data().size());
        }
    }

    // array version (empty argument)
    // if noCopy, call this method like this
        // ndarray.get_data(data);
        // ...
        // if (ndarray.compressed_deflate()) delete[] data;
    inline T* get_data(bool noCopy=false)
    {
        T* outputArray = NULL;
        if (!noCopy || compressed_deflate())
        {
            outputArray = new T[size()];
            get_data(outputArray);
        }
        else
        {
            outputArray = (T*) &(data()[0]);
        }
        return outputArray;
    }

    // general STL container version
    // TODO: refactor to remove the (probably) unnecessary copy-to-vector
    template <typename ContainerT>
    inline void get_data(ContainerT& outputContainer)
    {
        outputContainer.clear();
        std::vector<T> outputVector;
        get_data(outputVector);
        for (typename std::vector<T>::iterator it=outputVector.begin(); it!=outputVector.end(); it++)
        {
            outputContainer.push_back(*it);
        }
        // alternative version using insert that doesn't work for some reason
//        typename ContainerT::iterator it = outputContainer.begin();
//        outputContainer.insert(it, outputVector.begin(), outputVector.end());
    }

    // vector version
    inline void get_data(std::vector<T>& outputVector)
    {
        outputVector.clear();
        outputVector.resize(size());
        get_data(outputVector.data());
    }

    inline void _set_props(const utuple& shape, DataType dtype, bool compressed)
    {
        set_shape(shape);
        set_data_type(dtype);
        set_compressed_deflate(compressed);
    }

    // array version
    inline void set_array(const T* inputArray, const utuple& shape, bool compressed)
    {
        _set_props(shape, NDType<T>::T, compressed);
        set_data(inputArray);
    }

    // general STL container version
    // TODO: refactor to remove the (probably) unnecessary copy-to-vector
//    template <template <typename, typename=std::allocator<T> > class ContainerT>
//    inline void set_array(const ContainerT<T>& inputContainer, bool compressed=false)
    template <typename ContainerT>
    inline void set_array(const ContainerT& inputContainer, bool compressed=false)
    {
        utuple shape(inputContainer.size());
        _set_props(shape, NDType<T>::T, compressed);
        set_data(std::vector<T>(inputContainer.begin(), inputContainer.end()).data());
    }

    // TODO: refactor to remove the (probably) unnecessary copy-to-vector
//    template <template <typename, typename=std::allocator<T> > class ContainerT>
//    inline void set_array(const ContainerT<T>& inputContainer, const utuple& shape, bool compressed=false)
    template <typename ContainerT>
    inline void set_array(const ContainerT& inputContainer, const utuple& shape, bool compressed=false)
    {
        _set_props(shape, NDType<T>::T, compressed);
        if (size()!=inputContainer.size())
        {
            throw Exception("When serializing NDArray, size of data container and specified shape did not match: %d, %s", (int)inputContainer.size(), _shape.repr().c_str());
        }
        set_data(std::vector<T>(inputContainer.begin(), inputContainer.end()).data());
    }

    // vector version
    inline void set_array(const std::vector<T>& inputVector, bool compressed=false)
    {
        utuple shape(inputVector.size());
        _set_props(shape, NDType<T>::T, compressed);
        set_data(inputVector.data());
    }

    inline void set_array(const std::vector<T>& inputVector, const utuple& shape, bool compressed=false)
    {
        _set_props(shape, NDType<T>::T, compressed);
        if (size()!=inputVector.size())
        {
            throw Exception("When serializing NDArray, size of data vector and specified shape did not match: %d, %s", (int)inputVector.size(), _shape.repr().c_str());
        }
        set_data(inputVector.data());
    }

    // array version
    inline void set_data(const T* inputArray)
    {
        if (compressed_deflate())
        {
            size_t dataSizeEstimate=compressBound(size()*sizeof(T));
            mutable_data()->resize(dataSizeEstimate);
            ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*mutable_data())[0]), &dataSizeEstimate, (unsigned char*)inputArray, size()*sizeof(T)));
            mutable_data()->resize(dataSizeEstimate);
        }
        else
        {
            mutable_data()->resize(size()*sizeof(T));
            memcpy((unsigned char*)&((*mutable_data())[0]), (unsigned char*)inputArray, size()*sizeof(T));
        }
    }

//    template <template <typename, typename=std::allocator<T> > class ContainerT>  // possibly will need template <typename=T, typename=std::allocator<T>> class ContainerT
//    inline void set_data(const ContainerT& valueCont)
//    {
//        if (compressed_deflate())
//        {
//            size_t dataSizeEstimate=compressBound(valueCont.size()*sizeof(T));
//            mutable_data()->resize(dataSizeEstimate);
//            ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*mutable_data())[0]), &dataSizeEstimate, (unsigned char*)valueCont.data(), valueCont.size()*sizeof(T)));
//            mutable_data()->resize(dataSizeEstimate);
//        }
//        else
//        {
//            mutable_data()->resize(valueCont.size()*sizeof(T));
//            memcpy((unsigned char*)&((*mutable_data())[0]), (unsigned char*)valueCont.data(), valueCont.size()*sizeof(T));
//        }
//    }

    NDArray* setMsgPtr(robertslab::pbuf::NDArray* newArrMsg)
    {
        arrMsg=newArrMsg;
        _shape.setRepFieldPtr(arrMsg->mutable_shape());
        return this;
    }

// pass throughs
// accessors
    ArrayOrder array_order() const {return arrMsg->array_order();}
    ByteOrder byte_order() const {return arrMsg->byte_order();}
    DataType data_type() const {return arrMsg->data_type();}
    const Repeated<uint32_t>& shape() const {return _shape;}
    uint32_t shape(int index) const {return _shape.Get(index);}

    const std::string& data() const {return arrMsg->data();}
    bool compressed_deflate() const {return arrMsg->compressed_deflate();}

// mutators
    Repeated<uint32_t>* mutable_shape() {return &_shape;}
    std::string* mutable_data() {return arrMsg->mutable_data();}
    Repeated<uint32_t>& shape() {return _shape;}

    void set_array_order(ArrayOrder value) {arrMsg->set_array_order(value);}
    void set_byte_order(ByteOrder value) {arrMsg->set_byte_order(value);}
    void set_data_type(DataType value) {arrMsg->set_data_type(value);}
    void set_shape(int index, const uint32_t& value) {_shape.Set(index, value);}

    void set_shape(const utuple& shape)
    {
        _shape.Clear();
        for (int i=0;i<shape.len;i++)
        {
            _shape.Add(shape[i]);
        }
    }

    void set_compressed_deflate(bool value) {arrMsg->set_compressed_deflate(value);}

public:
    robertslab::pbuf::NDArray* arrMsg;
protected:
    Repeated<uint32_t> _shape;
};

}
}

#endif /* LM_PWRAP_NDARRAY */
