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
    typedef robertslab::pbuf::NDArray MsgT;

    NDArray(): msgPtr(NULL),msgConstPtr(NULL) {}
    NDArray(const MsgT& msgConstRef): msgPtr(NULL),msgConstPtr(NULL) {setMsg(msgConstRef);}
    NDArray(MsgT* msgMutablePtr): msgPtr(NULL),msgConstPtr(NULL) {setMsg(msgMutablePtr);}
    ~NDArray() {}

// accessors
    const MsgT* getMsg() const {return msgConstPtr;}
    uint rank() const {return shape().size();}
    uint32_t size() const {return shape().product();}
    size_t sizeBytes() const {return size()*sizeof(T);}

// mutators
    MsgT* getMsg()
    {
        if (msgPtr==NULL) throw Exception("Pointer to internal message (msgPtr) set to NULL in lm::protowrap::NDArray instance");
        return msgPtr;
    }

    /*
     * - array version
     * - call this method like this
     *
     *     data = new T[ndarray.size()];
     *     ndarray.get_data(data);
     *     ...
     *     delete[] data;
     */
    inline void get_data(T* outputArray) const
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

    /*
     * - array version (empty argument)
     * - if noCopy==true, call this method like this
     *
     *     T* data = ndarray.get_data(true);
     *     ...
     *     if (ndarray.compressed_deflate()) delete[] data;
     *
     * - else call this method like this
     *
     *     T* data = ndarray.get_data();
     *     ...
     *     delete[] data;
     */
    inline T* get_data(bool noCopy=false) const
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

    /*
     * - general STL container version
     * - call this method like this
     *
     *     std::some_container<T> data;
     *     ndarray.get_data(data);
     */
    template <template <typename, typename=std::allocator<T> > class ContainerT>
    inline void get_data(ContainerT<T>& outputContainer) const
    {
        outputContainer.clear();

        // TODO: refactor compression/decompression to remove the (probably) unnecessary copy-to-vector
        // if we need decompression, we have to copy the data over into a contiguous block of memory (ie a std::vector). Otherwise we can do something more optimized
        if (compressed_deflate())
        {
            std::vector<T> outputVector;
            get_data(outputVector);
            for (typename std::vector<T>::iterator it=outputVector.begin(); it!=outputVector.end(); it++)
            {
                outputContainer.push_back(*it);
            }
            // alternative version using insert that doesn't work for some reason
                // typename ContainerT::iterator it = outputContainer.begin();
                // outputContainer.insert(it, outputVector.begin(), outputVector.end());
        }
        else
        {
            // get a typed pointer to allow accessing the underlying string field in appropriately sized chunks
            T* outputArray = (T*) &(data()[0]);
            for (int i=0;i<size();i++)
            {
                outputContainer.push_back(outputArray[i]);
            }
        }
    }

    /*
     * - STL vector version
     *     - skips a copy operation present in the general STL container version
     * - call this method like this
     *
     *     std::vector<T> data;
     *     ndarray.get_data(data);
     */
    inline void get_data(std::vector<T>& outputVector) const
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
    template <template <typename, typename=std::allocator<T> > class ContainerT>
    inline void set_array(const ContainerT<T>& inputContainer, const utuple& shape, bool compressed=false)
    {
        _set_props(shape, NDType<T>::T, compressed);
        if (size()!=inputContainer.size())
        {
            throw Exception("When serializing NDArray, size of data container and specified shape did not match: %d, %s", (int)inputContainer.size(), _shape.repr().c_str());
        }

        // TODO: refactor compression/decompression to remove the (probably) unnecessary copy-to-vector
        // if we need compression, we have to copy the data over into a contiguous block of memory (ie a std::vector). Otherwise we can do something more optimized
        if (compressed_deflate())
        {
            set_data(std::vector<T>(inputContainer.begin(), inputContainer.end()).data());
        }
        else
        {
            // resize the underlying string field
            mutable_data()->resize(sizeBytes());

            // get a typed pointer to the underlying string field. This lets us write values of type T directly to the field
            T* dataAsTypedArray = (T*) &(data()[0]);

            // we use i for the array and an iterator for the container in case the container is not optimized for random access (eg deque)
            int i=0;
            for (typename ContainerT<T>::const_iterator it=inputContainer.begin();it!=inputContainer.end();it++)
            {
                dataAsTypedArray[i] = *it;
            }
        }
    }

    // general STL container version, for 1D arrays
    template <template <typename, typename=std::allocator<T> > class ContainerT>
    inline void set_array(const ContainerT<T>& inputContainer, bool compressed=false)
    {
        // for container input, if shape is not specified assume 1D array of size==inputContainer.size()
        set_array(inputContainer, utuple(inputContainer.size()), compressed);
    }

    // STL vector version
    inline void set_array(const std::vector<T>& inputVector, const utuple& shape, bool compressed=false)
    {
        _set_props(shape, NDType<T>::T, compressed);
        if (size()!=inputVector.size())
        {
            throw Exception("When serializing NDArray, size of data vector and specified shape did not match: %d, %s", (int)inputVector.size(), _shape.repr().c_str());
        }
        set_data(inputVector.data());
    }

    // STL vector version, for 1D arrays
    inline void set_array(const std::vector<T>& inputVector, bool compressed=false)
    {
        // for vector input, if shape is not specified assume 1D array of size==inputVector.size()
        set_array(inputVector, utuple(inputVector.size()), compressed);
    }

    inline void set_data(const T* inputArray)
    {
        if (compressed_deflate())
        {
            size_t dataSizeEstimate=compressBound(sizeBytes());
            mutable_data()->resize(dataSizeEstimate);
            ZLIB_EXCEPTION_CHECK(compress((unsigned char*)&((*mutable_data())[0]), &dataSizeEstimate, (unsigned char*)inputArray, sizeBytes()));
            mutable_data()->resize(dataSizeEstimate);
        }
        else
        {
            mutable_data()->resize(sizeBytes());
            memcpy((unsigned char*)&((*mutable_data())[0]), (unsigned char*)inputArray, sizeBytes());
        }
    }

    NDArray* setMsg(MsgT* newMsgMutablePtr)
    {
        msgPtr = newMsgMutablePtr;
        msgConstPtr = newMsgMutablePtr;
        _shape.setRepFieldPtr(msgPtr->mutable_shape());
        return this;
    }

    NDArray* setMsg(const MsgT& newArrMsgConstRef)
    {
        msgPtr = NULL;
        msgConstPtr = &newArrMsgConstRef;
        _shape.setRepFieldPtr(msgConstPtr->shape());
        return this;
    }

// pass throughs
// accessors
    ArrayOrder array_order() const {return getMsg()->array_order();}
    ByteOrder byte_order() const {return getMsg()->byte_order();}
    DataType data_type() const {return getMsg()->data_type();}
    const Repeated<uint32_t>& shape() const {return _shape;}
    uint32_t shape(int index) const {return _shape.Get(index);}

    const std::string& data() const {return getMsg()->data();}
    bool compressed_deflate() const {return getMsg()->compressed_deflate();}

// mutators
    Repeated<uint32_t>* mutable_shape() {return &_shape;}
    std::string* mutable_data() {return getMsg()->mutable_data();}
    Repeated<uint32_t>& shape() {return _shape;}

    void set_array_order(ArrayOrder value) {getMsg()->set_array_order(value);}
    void set_byte_order(ByteOrder value) {getMsg()->set_byte_order(value);}
    void set_data_type(DataType value) {getMsg()->set_data_type(value);}
    void set_shape(int index, const uint32_t& value) {_shape.Set(index, value);}

    void set_shape(const utuple& shape)
    {
        _shape.Clear();
        for (int i=0;i<shape.len;i++)
        {
            _shape.Add(shape[i]);
        }
    }

    void set_compressed_deflate(bool value) {getMsg()->set_compressed_deflate(value);}

public:
    robertslab::pbuf::NDArray* msgPtr;
    const robertslab::pbuf::NDArray* msgConstPtr;
protected:
    Repeated<uint32_t> _shape;
};

}
}

#endif /* LM_PWRAP_NDARRAY */
