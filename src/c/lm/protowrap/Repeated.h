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
#include "lm/protowrap/Msg.h"
#include "lm/protowrap/WrappedFields.h"
#include "lm/Types.h"
#include "lm/Version.h"

namespace lm {
namespace protowrap {

/*
// template that specializes to google::protobuf::RepeatedField<Element> for a numeric Element and google::protobuf::RepeatedPtrField<Element> otherwise
template <typename Element, bool=IsNumeric<Element>::value> struct RepeatedGoogleTypePolicy
{
    typedef google::protobuf::RepeatedPtrField<Element> GoogleFieldType;


#if __cplusplus <= 199711L
    // Returns the index of the first element of the wrapped RepeatedPtrField for which fieldVal==getterFunc(element), or -1 otherwise
    template <typename SubfieldElement>
    static int Index(const SubfieldElement& valToFind, SubfieldElement (Element::*fieldGetter)(), const GoogleFieldType* fieldConstPtr)
    {
        for (int index=0;index<fieldConstPtr->size();index++)
        {
            if ((fieldConstPtr->Get(index).*fieldGetter)()==valToFind) return index;
        }
        return -1;
    }

    // sets a field in every element of the wrapped RepeatedPtrField to the same, specified value
    template <typename SubfieldElement, typename SetterReturn>
    static void SetAll(const SubfieldElement& newFieldVal, SetterReturn (Element::*fieldSetter)(SubfieldElement), GoogleFieldType* fieldPtr)
    {
        for (typename GoogleFieldType::iterator it=fieldPtr->begin();it!=fieldPtr->end();it++)
        {
            (*it.*fieldSetter)(newFieldVal);
        }
    }
#endif
};

template <typename Element> struct RepeatedGoogleTypePolicy<Element, true>
{
    typedef google::protobuf::RepeatedField<Element> GoogleFieldType;

#if __cplusplus <= 199711L
    // for numeric types stored in a RepeatedField, the getterFunc version of Index is a dummy function
    template <typename T> static int Index(T, void*, void*) {throw UnimplementedException("Index called with a getterFunc is unimplemented for the Repeated wrapper templated on a numeric type.");}

    // for numeric types stored in a RepeatedField, SetAll is a dummy function
    template <typename T0, typename T1> static void SetAll(T0, T1, void*) {throw UnimplementedException("SetAll is unimplemented for the Repeated wrapper templated on a numeric type.");}
#endif
};
*/

// templates that deal with necessary specializations if Element is a numeric type

/*
 * - template that specializes to google::protobuf::RepeatedField<Element> for a numeric Element and google::protobuf::RepeatedPtrField<Element> otherwise
 */
template <typename Element, bool=IsNumeric<Element>::value> struct RepeatedGoogleTypePolicy
{
    typedef google::protobuf::RepeatedPtrField<Element> type;
};
template <typename Element> struct RepeatedGoogleTypePolicy<Element, true>
{
    typedef google::protobuf::RepeatedField<Element> type;
};

#if __cplusplus <= 199711L
/*
 * - returns the index of the first element of the wrapped field for which fieldVal==getterFunc(element) , or -1 otherwise.
 *     - disabled (ie causes a compile-time error) if Element is a numeric type
 */
template <typename Element, bool=IsNumeric<Element>::value> struct IndexPolicy
{
    template <typename SubfieldElement>
    static int Index(const SubfieldElement& valToFind, SubfieldElement (Element::*fieldGetter)(), const typename RepeatedGoogleTypePolicy<Element>::type* fieldConstPtr)
    {
        for (int index=0;index<fieldConstPtr->size();index++)
        {
            if ((fieldConstPtr->Get(index).*fieldGetter)()==valToFind) return index;
        }
        return -1;
    }
};
template <typename Element> struct IndexPolicy<Element, true> {};

/*
 * - sets a subfield in every element of the wrapped field to a single value, valToSet
 *     - disabled (ie causes a compile-time error) if Element is a numeric type
 */
template <typename Element, bool=IsNumeric<Element>::value> struct SetAllPolicy
{
    template <typename SubfieldElement, typename SetterReturn>
    static void SetAll(const SubfieldElement& valToSet, SetterReturn (Element::*fieldSetter)(SubfieldElement), typename RepeatedGoogleTypePolicy<Element>::type* fieldPtr)
    {
        for (typename RepeatedGoogleTypePolicy<Element>::type::iterator it=fieldPtr->begin();it!=fieldPtr->end();it++)
        {
            (*it.*fieldSetter)(valToSet);
        }
    }
};
template <typename Element> struct SetAllPolicy<Element, true> {};
#endif

//// templates that deal with necessary specializations if Element is derived from MsgWrap
//template <typename Element, bool=IsBaseOf<lm::protowrap::Msg<Element, typename Element::WrappedMsg>, Element>::value> struct RepeatedAttributePolicy
//{
//public:
//    typedef typename Element::WrappedField WrappedMsg;
//protected:
//    typename RepeatedGoogleTypePolicy<WrappedMsg>::type* wrappedFieldPtr;
//    const typename RepeatedGoogleTypePolicy<WrappedMsg>::type* wrappedFieldConstPtr;
//};
//template <typename Element> struct RepeatedAttributePolicy<Element, true>
//{
//protected:
//    typename RepeatedGoogleTypePolicy<Element>::type* wrappedFieldPtr;
//    const typename RepeatedGoogleTypePolicy<Element>::type* wrappedFieldConstPtr;
//};

template <typename Element>
class Repeated //: public RepeatedAttributePolicy<Element>
{
public:
// typedefs
    typedef typename RepeatedGoogleTypePolicy<Element>::type WrappedField;
    typedef typename WrappedField::iterator iterator;
    typedef typename WrappedField::const_iterator const_iterator;

// constructors/destructors
    Repeated(): wrappedFieldPtr(NULL),wrappedFieldConstPtr(NULL) {}
    Repeated(WrappedField* fieldPtr): wrappedFieldPtr(NULL),wrappedFieldConstPtr(NULL) {setWrappedField(fieldPtr);}
    Repeated(const WrappedField& fieldConstRef): wrappedFieldPtr(NULL),wrappedFieldConstPtr(NULL) {setWrappedField(fieldConstRef);}
    virtual ~Repeated() {}

// operators
    const Element& operator()(int i) const {return Get(i);}
    // conversion operators allow this wrapper to be used wherever google::protobuf::RepeatedField/RepeatedPtrField could be
    operator WrappedField*() {return wrappedFieldPtr;}
    operator const WrappedField*() const {return wrappedFieldConstPtr;}
    operator WrappedField&() {return *wrappedFieldPtr;}
    operator const WrappedField&() const {return *wrappedFieldConstPtr;}

    // accessors
    inline const Element& first() const {return Get(0);}
    inline const Element& last() const {return Get(lastIndex());}
    inline const int lastIndex() const {return size() - 1;}

    // Returns the index of the first element of the wrapped field for which element==val, or -1 otherwise
    int Index(const Element& val) const
    {
        for (int index=0;index< wrappedField()->size();index++)
        {
            if (wrappedField()->Get(index)==val) return index;
        }
        return -1;
    };

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

    inline void reverse()
    {
        int lastI = lastIndex();
        for (int i=0;i<size()/2;++i)
        {
            SwapElements(i, lastI - i);
        }
    }

// virtual funcs
    inline const WrappedField* wrappedField() const {return wrappedFieldConstPtr;}
    inline WrappedField* wrappedField()
    {
        if (wrappedFieldPtr==NULL)
        {
            throw Exception("Pointer to internal repeated field (wrappedFieldPtr) set to NULL in lm::protowrap::Repeated instance");
        }
        return wrappedFieldPtr;
    }
    inline virtual void setWrappedField(WrappedField* newFieldPtr)
    {
        wrappedFieldPtr = newFieldPtr;
        wrappedFieldConstPtr = newFieldPtr;
    }
    inline virtual void setWrappedField(const WrappedField& newFieldConstRef)
    {
        wrappedFieldPtr = NULL;
        wrappedFieldConstPtr = &newFieldConstRef;
    }

// pass throughs
// accessors
    const_iterator begin() const {return wrappedField()->begin();}
    const_iterator end() const {return wrappedField()->end();}
    bool empty() const {return wrappedField()->empty();}
    const Element& Get(int index) const {return wrappedField()->Get(index);}
    const google::protobuf::Descriptor* GetDescriptor() const {wrappedField()->GetDescriptor();}
    int size() const {return wrappedField()->size();}

// mutators
    iterator begin() {return wrappedField()->begin();}
    iterator end() {return wrappedField()->end();}
    Element* Add() {return wrappedField()->Add();}
    void Add(const Element& value) {wrappedField()->Add(value);}
    void Clear() {wrappedField()->Clear();}
    Element* Mutable(int index) {return wrappedField()->Mutable(index);}
    void Set(int index, const Element& value) {wrappedField()->Set(index, value);}
    void SwapElements(int index1, int index2) {wrappedField()->SwapElements(index1, index2);}

/*
 * function that need to be disabled if Element is a numeric type
 */
    template <typename U>  //, typename EnableIf<!IsNumeric<U>::value>::type>
    void AddAllocated(U* value)
    {
        wrappedField()->AddAllocated(value);
    }

#if __cplusplus <= 199711L
    // Returns the index of the first element of the wrapped field for which fieldVal==getterFunc(element), or -1 otherwise. Unimplemented if Element is a numeric type
    template <typename SubfieldElement> int Index(const SubfieldElement& valToFind, SubfieldElement getterFuncPtr) const
    {
        return IndexPolicy<Element>::Index(valToFind, getterFuncPtr, wrappedField());
    }

    template <typename SubfieldElement, typename SetterReturn> void SetAll(const SubfieldElement& newFieldVal, SetterReturn setterFuncPtr)
    {
        SetAllPolicy<Element>::SetAll(newFieldVal, setterFuncPtr, wrappedField());
    }

#else
    // sets a subfield in every element of the wrapped field to a single value, valToSet. Enabled only if Element is not numeric
    template <typename SubfieldElement, typename SubfieldSetter, typename U=Element, typename=typename EnableIf<!IsNumeric<U>::value>::type>
    static void SetAll(const SubfieldElement& valToSet, SubfieldSetter subfieldSetter, WrappedField* fieldPtr)
    {
        for (typename WrappedField::iterator it=fieldPtr->begin();it!=fieldPtr->end();it++)
        {
            (*it.*subfieldSetter)(valToSet);
        }
    }

    // Returns the index of the first element of the wrapped RepeatedPtrField for which fieldVal==getterFunc(element), or -1 otherwise. Enabled only if Element is not a numeric type
    template <typename SubfieldElement, typename SubfieldGetter, typename U=Element, typename=typename EnableIf<!IsNumeric<U>::value>::type>
    int Index(const SubfieldElement& valToFind, SubfieldGetter subfieldGetter, const WrappedField* fieldConstPtr)
    {
        for (int index=0;index<fieldConstPtr->size();index++)
        {
            if ((wrappedField()->Get(index).*subfieldGetter)()==valToFind) return index;
        }
        return -1;
    }
#endif

protected:
    WrappedField* wrappedFieldPtr;
    const WrappedField* wrappedFieldConstPtr;
};

}
}

#endif /* LM_PWRAP_REPEATED */
