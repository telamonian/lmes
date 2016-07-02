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

#include <google/protobuf/descriptor.h>
#include <google/protobuf/repeated_field.h>
#include <numeric>
#include <sstream>
#include <string>

#include "lm/Math.h"
#include "lm/Types.h"

namespace lm {
namespace protowrap {

// template that specializes to google::protobuf::RepeatedField<Element> for a numeric Element and google::protobuf::RepeatedPtrField<Element> otherwise
template <typename Element, bool> struct _RepeatedSpecialization;
template <typename Element> struct _RepeatedSpecialization<Element, false>
{
    typedef google::protobuf::RepeatedPtrField<Element> WrappedField;

    // Returns the index of the first element of the wrapped RepeatedPtrField for which fieldVal==getterFunc(element), or -1 otherwise
    template <typename SubfieldElement>
    static int Index(const SubfieldElement& valToFind, SubfieldElement (Element::*fieldGetter)(), const WrappedField* fieldConstPtr)
    {
        for (int index=0;index<fieldConstPtr->size();index++)
        {
            if ((fieldConstPtr->Get(index).*fieldGetter)()==valToFind) return index;
        }
        return -1;
    }

    // sets a field in every element of the wrapped RepeatedPtrField to the same, specified value
    template <typename SubfieldElement, typename SetterReturn>
    static void SetAll(const SubfieldElement& newFieldVal, SetterReturn (Element::*fieldSetter)(SubfieldElement), WrappedField* fieldPtr)
    {
        for (typename WrappedField::iterator it=fieldPtr->begin();it!=fieldPtr->end();it++)
        {
            (*it.*fieldSetter)(newFieldVal);
        }
    }

protected:
    WrappedField* fieldPtr;
    const WrappedField* fieldConstPtr;
};

template <typename Element> struct _RepeatedSpecialization<Element, true>
{
    typedef google::protobuf::RepeatedField<Element> WrappedField;

    // for numeric types stored in a RepeatedField, the getterFunc version of Index is a dummy function
    template <typename T> static int Index(T, void*, void*) {throw UnimplementedException("Index called with a getterFunc is unimplemented for the Repeated wrapper templated on a numeric type.");}

    // for numeric types stored in a RepeatedField, SetAll is a dummy function
    template <typename T0, typename T1> static void SetAll(T0, T1, void*) {throw UnimplementedException("SetAll is unimplemented for the Repeated wrapper templated on a numeric type.");}
};

template <typename Element> struct RepeatedSpecialization : public _RepeatedSpecialization<Element, IsNumeric<Element>::value> {}; //{typedef typename _RepeatedSpecialization<Element, IsNumeric<Element>::value>::RepeatedField RepeatedField;};

template <typename Element>
class Repeated
{
public:
// typedefs
    typedef typename RepeatedSpecialization<Element>::WrappedField WrappedField;
    typedef typename WrappedField::iterator iterator;
    typedef typename WrappedField::const_iterator const_iterator;

// constructors/destructors
    Repeated(): fieldPtr(NULL),fieldConstPtr(NULL) {}
    Repeated(WrappedField* fieldPtr): fieldPtr(NULL),fieldConstPtr(NULL) {setFieldPtr(fieldPtr);}
    Repeated(const WrappedField& fieldConstRef): fieldPtr(NULL),fieldConstPtr(NULL) {setFieldPtr(fieldConstRef);}
    virtual ~Repeated() {}

// operators
    const Element& operator()(int i) const {return Get(i);}
    // conversion operators allow this wrapper to be used wherever google::protobuf::RepeatedField/RepeatedPtrField could be
    operator WrappedField*() {return fieldPtr;}
    operator const WrappedField&() const {return *fieldPtr;}

// accessors
    inline const Element& first() const {return Get(0);}
    inline const Element& last() const {return Get(lastIndex());}
    inline const int lastIndex() const {return size() - 1;}

    // Returns the index of the first element of the wrapped field for which element==val, or -1 otherwise
    int Index(const Element& val) const
    {
        for (int index=0;index< getFieldPtr()->size();index++)
        {
            if (getFieldPtr()->Get(index)==val) return index;
        }
        return -1;
    };

    // Returns the index of the first element of the wrapped field for which fieldVal==getterFunc(element), or -1 otherwise. Unimplemented if Element is a numeric type
    template <typename SubfieldElement> int Index(const SubfieldElement& valToFind, SubfieldElement getterFuncPtr) const
    {
        return RepeatedSpecialization<Element>::Index(valToFind, getterFuncPtr, getFieldPtr());
    }

    inline Element product() const {return ProductFunctor<Element>::call(begin(), end());}
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
    inline Repeated<Element>& operator<<(Element val) {Add(val); return *this;}

    template <typename SubfieldElement, typename SetterReturn> void SetAll(const SubfieldElement& newFieldVal, SetterReturn setterFuncPtr)
    {
        RepeatedSpecialization<Element>::SetAll(newFieldVal, setterFuncPtr, getFieldPtr());
    }

    inline void reverse()
    {
        int lastI = lastIndex();
        for (int i=0;i<size()/2;++i)
        {
            SwapElements(i, lastI - i);
        }
    }

// virtual funcs
    inline const WrappedField* getFieldPtr() const {return fieldConstPtr;}
    inline WrappedField* getFieldPtr()
    {
        if (fieldPtr==NULL)
        {
            throw Exception("Pointer to internal repeated field (fieldPtr) set to NULL in lm::protowrap::Repeated instance");
        }
        return fieldPtr;
    }
    inline virtual void setFieldPtr(WrappedField* newRepFieldPtr)
    {
        fieldPtr = newRepFieldPtr;
        fieldConstPtr = newRepFieldPtr;
    }
    inline virtual void setRepFieldPtr(const WrappedField& newRepFieldConstRef)
    {
        fieldPtr = NULL;
        fieldConstPtr = &newRepFieldConstRef;
    }

// pass throughs
// accessors
    const_iterator begin() const {return getFieldPtr()->begin();}
    const_iterator end() const {return getFieldPtr()->end();}
    bool empty() const {return getFieldPtr()->empty();}
    const Element& Get(int index) const {return getFieldPtr()->Get(index);}
    const google::protobuf::Descriptor* GetDescriptor() const {getFieldPtr()->GetDescriptor();}
    int size() const {return getFieldPtr()->size();}

// mutators
    iterator begin() {return getFieldPtr()->begin();}
    iterator end() {return getFieldPtr()->end();}
    Element* Add() {return getFieldPtr()->Add();}
    void Add(const Element& value) {getFieldPtr()->Add(value);}
    void Clear() {getFieldPtr()->Clear();}
    Element* Mutable(int index) {return getFieldPtr()->Mutable(index);}
    void Set(int index, const Element& value) {getFieldPtr()->Set(index, value);}
    void SwapElements(int index1, int index2) {getFieldPtr()->SwapElements(index1, index2);}

protected:
    WrappedField* fieldPtr;
    const WrappedField* fieldConstPtr;
};

}
}

#endif /* LM_PWRAP_REPEATED */
