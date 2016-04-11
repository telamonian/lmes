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
#ifndef LM_PWRAP_REPEATED
#define LM_PWRAP_REPEATED

#include <google/protobuf/repeated_field.h>
#include <numeric>
#include <string>

#include "lm/Math.h"
#include "lm/Types.h"

namespace lm {
namespace protowrap {

// main template for type generator struct that will return google::protobuf::RepeatedField<T> for a numeric T and google::protobuf::RepeatedPtrField<T> otherwise
template <typename T, bool> struct _RepeatedTypedef;
template <typename T> struct _RepeatedTypedef<T, false> {typedef google::protobuf::RepeatedPtrField<T> type;};
template <typename T> struct _RepeatedTypedef<T, true> {typedef google::protobuf::RepeatedField<T> type;};
template <typename T> struct RepeatedTypedef {typedef typename _RepeatedTypedef<T, IsNumeric<T>::value>::type type;};

template <typename T>
class Repeated
{
public:
// typedefs
    typedef typename RepeatedTypedef<T>::type type;
    typedef typename type::iterator iterator;
    typedef typename type::const_iterator const_iterator;

// constructors/destructors
    Repeated(): repFieldPtr(NULL) {}
    Repeated(type* repFieldPtr): repFieldPtr(repFieldPtr) {}
    ~Repeated() {}

// accessors
    inline T product() const {return ProductFunctor<T>::call(begin(), end());}

// mutators
    inline Repeated<T>& operator<<(T val) {repFieldPtr->Add(val); return *this;}
    inline void setRepFieldPtr(type* newRepFieldPtr) {repFieldPtr = newRepFieldPtr;}

// pass throughs
// accessors
    const_iterator begin() const {return repFieldPtr->begin();}
    const_iterator end() const {return repFieldPtr->end();}
    const T& Get(int index) const {return repFieldPtr->Get(index);}

// mutators
    iterator begin() {return repFieldPtr->begin();}
    iterator end() {return repFieldPtr->end();}
    T* Add() {return repFieldPtr->Add();}
    void Add(const T& value) {repFieldPtr->Add(value);}
    void Clear() {repFieldPtr->Clear();}
    T* Mutable(int index) {return repFieldPtr->Mutable(index);}
    void Set(int index, const T& value) {repFieldPtr->Set(index, value);}

protected:
    type* repFieldPtr;

};

}
}

#endif /* LM_PWRAP_REPEATED */
