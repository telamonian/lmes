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
#ifndef LM_PROTOWRAP_REPEATED
#define LM_PROTOWRAP_REPEATED

#include <algorithm>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/repeated_field.h>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <valarray>

#include "lm/Math.h"
#include "lm/protowrap/Msg.h"
#include "lm/Types.h"

namespace lm {
namespace protowrap {

// templates that deal with necessary specializations if Element is a numeric type

/*
 * - template that specializes to google::protobuf::RepeatedField<Element> for a numeric Element and google::protobuf::RepeatedPtrField<Element> otherwise
 */
template <typename Element, bool=IsNumeric<Element>::value>
struct GoogleRepeatedTypedefPolicy
{
    typedef google::protobuf::RepeatedPtrField<Element> type;
    typedef const google::protobuf::RepeatedPtrField<Element> const_type;
    typedef google::protobuf::internal::RepeatedPtrFieldBackInsertIterator<Element> back_insert_iterator;
};
template <typename Element> struct GoogleRepeatedTypedefPolicy<Element, true>
{
    typedef google::protobuf::RepeatedField<Element> type;
    typedef const google::protobuf::RepeatedField<Element> const_type;
    typedef google::protobuf::internal::RepeatedFieldBackInsertIterator<Element> back_insert_iterator;
};

#if __cplusplus <= 199711L
/*
 * - gets the value of a particular subfield from every Element in the wrapped field and returns all of them in a vector (or writes them to an OutputIterator that you pass in)
 *     - disabled (ie causes a compile-time error) if Element is a numeric type
 */
template <typename Element, bool=IsNumeric<Element>::value>
struct GetAllPolicy
{
    template <typename SubfieldElement>
    static std::vector<SubfieldElement> GetAll(typename GoogleRepeatedTypedefPolicy<Element>::const_type* fieldConstPtr, SubfieldElement (Element::*fieldGetter)() const)
    {
        std::vector<SubfieldElement> values;
        values.reserve(fieldConstPtr->size());

        GetAll(fieldConstPtr, fieldGetter, std::back_inserter(values));
        return values;
    }

    template <typename SubfieldElement, typename OutputIterator>
    static void GetAll(typename GoogleRepeatedTypedefPolicy<Element>::const_type* fieldConstPtr, SubfieldElement (Element::*fieldGetter)() const, OutputIterator outputIt)
    {
        for (typename GoogleRepeatedTypedefPolicy<Element>::type::const_iterator repeatedIt=fieldConstPtr->begin();repeatedIt!=fieldConstPtr->end();repeatedIt++)
        {
            *outputIt++ = (*repeatedIt.*fieldGetter)();
        }
    }
};
template <typename Element> struct GetAllPolicy<Element, true> {};

/*
 * - returns the index of the first element of the wrapped field for which fieldVal==getterFunc(element) , or -1 otherwise.
 *     - disabled (ie causes a compile-time error) if Element is a numeric type
 */
template <typename Element, bool=IsNumeric<Element>::value>
struct IndexPolicy
{
    template <typename SubfieldElement>
    static int Index(typename GoogleRepeatedTypedefPolicy<Element>::type* fieldConstPtr, const SubfieldElement& valToFind, SubfieldElement (Element::*fieldGetter)() const)
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
template <typename Element, bool=IsNumeric<Element>::value>
struct SetAllPolicy
{
    template <typename SubfieldElement, typename SetterReturn>
    static inline void SetAll(typename GoogleRepeatedTypedefPolicy<Element>::type* fieldPtr, const SubfieldElement& valToSet, SetterReturn (Element::*fieldSetter)(SubfieldElement))
    {
        for (typename GoogleRepeatedTypedefPolicy<Element>::type::iterator it=fieldPtr->begin();it!=fieldPtr->end();it++)
        {
            (*it.*fieldSetter)(valToSet);
        }
    }
};
template <typename Element> struct SetAllPolicy<Element, true> {};
#endif

/*
// templates that deal with necessary specializations if Element is derived from MsgWrap
template <typename Element, bool=IsBaseOf<lm::protowrap::Msg<Element, typename Element::WrappedMsg>, Element>::value> struct RepeatedAttributePolicy
{
public:
    typedef typename Element::WrappedField WrappedMsg;
protected:
    typename GoogleRepeatedTypedefPolicy<WrappedMsg>::type* wrappedFieldPtr;
    const typename GoogleRepeatedTypedefPolicy<WrappedMsg>::type* wrappedFieldConstPtr;
};
template <typename Element> struct RepeatedAttributePolicy<Element, true>
{
protected:
    typename GoogleRepeatedTypedefPolicy<Element>::type* wrappedFieldPtr;
    const typename GoogleRepeatedTypedefPolicy<Element>::type* wrappedFieldConstPtr;
};
*/

template <typename Element>
class Repeated
{
public:
// typedefs
    typedef typename GoogleRepeatedTypedefPolicy<Element>::type WrappedField;
    typedef typename WrappedField::iterator iterator;
    typedef typename WrappedField::const_iterator const_iterator;
    typedef typename GoogleRepeatedTypedefPolicy<Element>::back_insert_iterator back_insert_iterator;

// constructors/destructors
    Repeated(): wrappedFieldPtr(NULL),wrappedFieldConstPtr(NULL) {}
    Repeated(WrappedField* fieldPtr): wrappedFieldPtr(NULL),wrappedFieldConstPtr(NULL) {setWrappedField(fieldPtr);}
    Repeated(const WrappedField& fieldConstRef): wrappedFieldPtr(NULL),wrappedFieldConstPtr(NULL) {setWrappedField(fieldConstRef);}
    virtual ~Repeated() {}

// operators
    inline const Element& operator()(int i) const {return Get(i);}
    // conversion operators allow this wrapper to be used wherever google::protobuf::RepeatedField/RepeatedPtrField could be
    operator WrappedField*() {return wrappedFieldPtr;}
    operator const WrappedField*() const {return wrappedFieldConstPtr;}
    operator WrappedField&() {return *wrappedFieldPtr;}
    operator const WrappedField&() const {return *wrappedFieldConstPtr;}

//    inline void deserializeTo(Element* array_ptr_writeto) const
//    {
//        std::copy(begin(), end(), array_ptr_writeto);
//    }
    template <typename OutputIterator>
    inline void deserializeTo(OutputIterator endIt_writeto, typename EnableIf<not HasPushBack<OutputIterator>::value>::type* = 0) const
    {
        std::copy(begin(), end(), endIt_writeto);
    }
    template <typename Container>
    inline void deserializeTo(Container& container_writeto, typename EnableIf<HasPushBack<Container>::value>::type* = 0) const
    {
        std::copy(begin(), end(), std::back_inserter(container_writeto));
    }
    
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

    inline back_insert_iterator back_inserter() {return google::protobuf::RepeatedFieldBackInserter(wrappedField());}

    inline void reverse()
    {
        int lastI = lastIndex();
        for (int i=0;i<size()/2;++i)
        {
            SwapElements(i, lastI - i);
        }
    }

    template <typename InputIterator>
    inline void serializeFrom(const InputIterator& startIt, const InputIterator& endIt, typename EnableIf<not HasBeginEnd<InputIterator>::value>::type* = 0)
    {
        std::copy(startIt, endIt, back_inserter());
    }
    template <typename Container>
    inline void serializeFrom(const Container& container_readfrom, typename EnableIf<HasBeginEnd<Container>::value>::type* = 0)
    {
        std::copy(container_readfrom.begin(), container_readfrom.end(), back_inserter());
    }
    
// getters and setters for the wrapped field
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
    inline virtual void setWrappedFieldNull()
    {
        wrappedFieldPtr = NULL;
        wrappedFieldConstPtr = NULL;
    }

// pass throughs
// accessors
    inline const_iterator begin() const {return wrappedField()->begin();}
    inline const_iterator end() const {return wrappedField()->end();}
    const Element* data() const {return wrappedField()->data();}
    inline bool empty() const {return wrappedField()->empty();}
    inline const Element& Get(int index) const {return wrappedField()->Get(index);}
    inline const google::protobuf::Descriptor* GetDescriptor() const {wrappedField()->GetDescriptor();}
    inline int size() const {return wrappedField()->size();}

// mutators
    inline iterator begin() {return wrappedField()->begin();}
    inline iterator end() {return wrappedField()->end();}
    inline Element* Add() {return wrappedField()->Add();}
    inline void Add(const Element& value) {wrappedField()->Add(value);}
    inline void Clear() {wrappedField()->Clear();}
    inline Element* Mutable(int index) {return wrappedField()->Mutable(index);}
    Element* mutable_data() {return wrappedField()->mutable_data();}
    inline void Set(int index, const Element& value) {wrappedField()->Set(index, value);}
    inline void SwapElements(int index1, int index2) {wrappedField()->SwapElements(index1, index2);}

/*
 * function that need to be disabled if Element is a numeric type
 */
    // template <typename U>  //, typename EnableIf<!IsNumeric<U>::value>::type>
    void AddAllocated(Element* value) //(U* value)
    {
        wrappedField()->AddAllocated(value);
    }

    Element* ReleaseLast()
    {
        return wrappedField()->ReleaseLast();
    }

#if __cplusplus <= 199711L
    template <typename SubfieldElement> inline std::vector<SubfieldElement> GetAll(SubfieldElement getterFuncPtr) const
    {
        return GetAllPolicy<Element>::GetAll(wrappedField(), getterFuncPtr);
    }

    template <typename SubfieldElement, typename OutputIterator> inline void GetAll(SubfieldElement getterFuncPtr, OutputIterator outputIt) const
    {
        GetAllPolicy<Element>::GetAll(wrappedField(), getterFuncPtr, outputIt);
    }

    template <typename SubfieldElement> inline int Index(const SubfieldElement& valToFind, SubfieldElement getterFuncPtr) const
    {
        return IndexPolicy<Element>::Index(wrappedField(), valToFind, getterFuncPtr);
    }

    template <typename SubfieldElement, typename SetterReturn> inline void SetAll(const SubfieldElement& newFieldVal, SetterReturn setterFuncPtr)
    {
        SetAllPolicy<Element>::SetAll(wrappedField(), newFieldVal, setterFuncPtr);
    }

#else
    /*
     * - gets the value of a particular subfield from every Element in the wrapped field and returns all of them in a vector (or writes them to an OutputIterator that you pass in)
     *     - disabled (ie causes a compile-time error) if Element is a numeric type
     */
    template <typename SubfieldElement, typename U=Element, typename=typename EnableIf<!IsNumeric<U>::value>::type>
    inline std::vector<SubfieldElement> GetAll(SubfieldElement (Element::*fieldGetter)()) const
    {
        std::vector<SubfieldElement> values;
        values.reserve(fieldConstPtr->size());

        GetAll(fieldGetter, std::back_inserter(values));
        return values;
    }

    template <typename SubfieldElement, typename OutputIterator, typename U=Element, typename=typename EnableIf<!IsNumeric<U>::value>::type>
    inline void GetAll(SubfieldElement (Element::*fieldGetter)(), OutputIterator outputIt) const
    {
        for (typename GoogleRepeatedTypedefPolicy<Element>::type::const_iterator repeatedIt=begin();repeatedIt!=end();repeatedIt++)
        {
            *outputIt++ = (*repeatedIt.*fieldGetter)();
        }
    }

    /*
     * - returns the index of the first element of the wrapped field for which fieldVal==getterFunc(element) , or -1 otherwise.
     *     - disabled (ie causes a compile-time error) if Element is a numeric type
     */
    template <typename SubfieldElement, typename SubfieldGetter, typename U=Element, typename=typename EnableIf<!IsNumeric<U>::value>::type>
    int Index(const SubfieldElement& valToFind, SubfieldGetter subfieldGetter, const WrappedField* fieldConstPtr)
    {
        for (int index=0;index<fieldConstPtr->size();index++)
        {
            if ((wrappedField()->Get(index).*subfieldGetter)()==valToFind) return index;
        }
        return -1;
    }

    /*
     * - sets a subfield in every element of the wrapped field to a single value, valToSet
     *     - disabled (ie causes a compile-time error) if Element is a numeric type
     */
    template <typename SubfieldElement, typename SubfieldSetter, typename U=Element, typename=typename EnableIf<!IsNumeric<U>::value>::type>
    static void SetAll(const SubfieldElement& valToSet, SubfieldSetter subfieldSetter, WrappedField* fieldPtr)
    {
        for (typename WrappedField::iterator it=fieldPtr->begin();it!=fieldPtr->end();it++)
        {
            (*it.*subfieldSetter)(valToSet);
        }
    }

#endif

protected:
    WrappedField* wrappedFieldPtr;
    const WrappedField* wrappedFieldConstPtr;
};

/*
 * some type_traits
 */
//template<typename> struct IsGoogleRepeated {static const bool value = false;};
//template<template <typename> class T, typename Element> struct IsGoogleRepeated<T<Element> > {static const bool value = IsSame<T<Element>, google::protobuf::RepeatedField<Element> >::value | IsSame<T<Element>, google::protobuf::RepeatedPtrField<Element> >::value;};

template<typename T> struct IsGoogleRepeated {static const bool value = IsSameTemplate<T, google::protobuf::RepeatedField>::value | IsSameTemplate<T, google::protobuf::RepeatedPtrField>::value;};


//template<template <typename> class T, typename Element> struct IsGoogleRepeated {static const bool value = IsSame<T, google::protobuf::RepeatedField>::value | IsSame<T, google::protobuf::RepeatedPtrField>::value;};
template<typename T> struct IsRepeated {static const bool value = IsSameTemplate<T, Repeated>::value;};

/*
 * non-member helper functions
 */
template <typename T, typename=typename EnableIf<IsGoogleRepeated<T>::value>::type>
typename GoogleRepeatedTypedefPolicy<typename T::value_type>::back_insert_iterator back_inserter(T* googleRepeated)
{
    return google::protobuf::RepeatedFieldBackInserter(googleRepeated);
}

template <typename T, typename=typename EnableIf<IsRepeated<T>::value>::type>
typename T::back_insert_iterator back_inserter(T* repeated)
{
    return repeated->back_inserter();
}

template <typename Container>
std::back_insert_iterator<typename Container::value_type> back_inserter(Container* container)
{
    return std::back_inserter(container);
}

// this specialization allows for conversion of data to explicitly specified Element (ie type) when constructing the valarray
template <typename Element=void> struct make_valarray
{
    template <typename T>
    inline static std::valarray<Element> call(const T& repeated)
    {
        std::valarray<Element> newValarray(repeated.size());
        std::copy(repeated.begin(), repeated.end(), &newValarray[0]);
        return newValarray;
    }
};

// this specialization allows for Element to be deduced implicitly from the passed-in repeated object
template <> struct make_valarray<void>
{
    template <typename T>
    inline static std::valarray<typename T::WrappedField::value_type> call(const T& repeated)
    {
        return std::valarray<typename T::WrappedField::value_type>(repeated.data(), repeated.size());
    }
};

}
}

#endif /* LM_PROTOWRAP_REPEATED */
