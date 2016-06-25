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
#include <sstream>
#include <string>

#include "lm/Math.h"
#include "lm/Types.h"

namespace lm {
namespace protowrap {

// template that specializes for google::protobuf::RepeatedField<T> for a numeric T and google::protobuf::RepeatedPtrField<T> otherwise
template <typename ValT, bool> struct _RepeatedSpecialization;
template <typename ValT> struct _RepeatedSpecialization<ValT, false>
{
    typedef google::protobuf::RepeatedPtrField<ValT> RepT;

    // Returns the index of the first element of the wrapped RepeatedPtrField for which fieldVal==getterFunc(element), or -1 otherwise
    template <typename FieldValT>
    static int Index(const FieldValT& fieldVal, FieldValT (ValT::*getterFunc)(), const RepT* repFieldConstPtr) const
    {
        for (int index=0;index<repFieldConstPtr->size();index++)
        {
            if (*(repFieldConstPtr->Get(index)).*getterFunc()==fieldVal) return index;
        }
        return -1;
    };

    // sets a field in every element of the wrapped RepeatedPtrField to the same, specified value
    template <typename FieldValT, typename SetterReturnT>
    static void SetAll(const FieldValT& newFieldVal, SetterReturnT (ValT::*setterFunc)(FieldValT), RepT* repFieldPtr)
    {
        for (typename RepT::iterator it=repFieldPtr->begin();it!=repFieldPtr->end();it++)
        {
            (*it.*setterFunc)(newFieldVal);
        }
    }

protected:
    RepT* repFieldPtr;
    const RepT* repFieldConstPtr;
};

template <typename ValT> struct _RepeatedSpecialization<ValT, true>
{
    typedef google::protobuf::RepeatedField<ValT> RepT;

    // for numeric types stored in a RepeatedField, the getterFunc version of Index is a dummy function
    template <typename T> static int Index(const T&, void*, void*) const {throw UnimplementedException("Index called with a getterFunc is unimplemented for the Repeated wrapper templated on a numeric type.");}

    // for numeric types stored in a RepeatedField, SetAll is a dummy function
    template <typename T> static void SetAll(const T&, void*, void*) {throw UnimplementedException("SetAll is unimplemented for the Repeated wrapper templated on a numeric type.");}
};

template <typename ValT> struct RepeatedSpecialization : public _RepeatedSpecialization<ValT, IsNumeric<ValT>::value> {}; //{typedef typename _RepeatedSpecialization<ValT, IsNumeric<ValT>::value>::GoogleT GoogleT;};

template <typename ValT>
class Repeated
{
public:
// typedefs
    typedef typename RepeatedSpecialization<ValT>::RepT GoogleT;
    typedef typename GoogleT::iterator iterator;
    typedef typename GoogleT::const_iterator const_iterator;

// constructors/destructors
    Repeated(): repFieldPtr(NULL),repFieldConstPtr(NULL) {}
    Repeated(GoogleT* repFieldPtr): repFieldPtr(NULL),repFieldConstPtr(NULL) {setRepFieldPtr(repFieldPtr);}
    Repeated(const GoogleT& repFieldConstRef): repFieldPtr(NULL),repFieldConstPtr(NULL) {setRepFieldPtr(repFieldConstRef);}
    virtual ~Repeated() {}

// operators
    const ValT& operator()(int i) const {return Get(i);}
    // conversion operators allow this wrapper to be used wherever google::protobuf::RepeatedField/RepeatedPtrField could be
    operator GoogleT*() {return repFieldPtr;}
    operator GoogleT&() const {return *repFieldPtr;}

// accessors
    inline const ValT& first() const {return Get(0);}
    inline const ValT& last() const {return Get(lastIndex());}
    inline const int lastIndex() const {return size() - 1;}

    // Returns the index of the first element of the wrapped field for which element==val, or -1 otherwise
    int Index(const ValT& val) const
    {
        for (int index=0;index<getRepFieldPtr()->size();index++)
        {
            if (getRepFieldPtr()->Get(index)==val) return index;
        }
        return -1;
    };

    // Returns the index of the first element of the wrapped field for which fieldVal==getterFunc(element), or -1 otherwise. Unimplemented if ValT is a numeric type
    template <typename FieldValT> int Index(const FieldValT& indexVal, ValT getterFuncPtr) const
    {
        return RepeatedSpecialization<ValT>::Index(indexVal, getterFuncPtr, getRepFieldPtr());
    }

    inline ValT product() const {return ProductFunctor<ValT>::call(begin(), end());}
    std::string repr(const char* suffix="") const
    {
        std::stringstream reprStream("(");
        for (uint i=0; i<size(); i++)
        {
            if (i > 0) reprStream << ',';
            reprStream << Get(i);
        }
        reprStream << ")" << suffix;
        return reprStream.str();
    }

// mutators
    inline Repeated<ValT>& operator<<(ValT val) {Add(val); return *this;}

    template <typename FieldValT, typename SetterReturnT> void SetAll(const FieldValT& setVal, SetterReturnT setterFuncPtr)
    {
        RepeatedSpecialization<ValT>::SetAll(setVal, setterFuncPtr, getRepFieldPtr());
    }

    inline void reverse()
    {
        int lastIndex = lastIndex();
        for (int i=0;i<size()/2;++i)
        {
            SwapElements(i, lastIndex - i);
        }
    }

// virtual funcs
    inline const GoogleT* getRepFieldPtr() const {return repFieldConstPtr;}
    inline GoogleT* getRepFieldPtr()
    {
        if (repFieldPtr==NULL) throw Exception("Pointer to internal repeated field (repFieldPtr) set to NULL in lm::protowrap::Repeated instance");
        return repFieldPtr;
    }
    inline virtual void setRepFieldPtr(GoogleT* newRepFieldPtr)
    {
        repFieldPtr = newRepFieldPtr;
        repFieldConstPtr = newRepFieldPtr;
    }
    inline virtual void setRepFieldPtr(const GoogleT& newRepFieldConstRef)
    {
        repFieldPtr = NULL;
        repFieldConstPtr = &newRepFieldConstRef;
    }

// pass throughs
// accessors
    const_iterator begin() const {return getRepFieldPtr()->begin();}
    const_iterator end() const {return getRepFieldPtr()->end();}
    bool empty() const {return getRepFieldPtr()->empty();}
    const ValT& Get(int index) const {return getRepFieldPtr()->Get(index);}
    int size() const {return getRepFieldPtr()->size();}

// mutators
    iterator begin() {return getRepFieldPtr()->begin();}
    iterator end() {return getRepFieldPtr()->end();}
    ValT* Add() {return getRepFieldPtr()->Add();}
    void Add(const ValT& value) {getRepFieldPtr()->Add(value);}
    void Clear() {getRepFieldPtr()->Clear();}
    ValT* Mutable(int index) {return getRepFieldPtr()->Mutable(index);}
    void Set(int index, const ValT& value) {getRepFieldPtr()->Set(index, value);}
    void SwapElements(int index1, int index2) {getRepFieldPtr()->SwapElements(index1, index2);}

protected:
    GoogleT* repFieldPtr;
    const GoogleT* repFieldConstPtr;
};

}
}

#endif /* LM_PWRAP_REPEATED */
