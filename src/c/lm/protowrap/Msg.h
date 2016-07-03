/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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
 * Author(s): Max Klein
 */
#ifndef LM_PROTOWRAP_MSG_H_
#define LM_PROTOWRAP_MSG_H_

// MAP macro, modified from https://github.com/swansontec/map-macro
// EVAL has been configured for a max recursion depth of 64
#define EVAL0(...) __VA_ARGS__
#define EVAL1(...) EVAL0 (EVAL0 (__VA_ARGS__))
#define EVAL2(...) EVAL1 (EVAL1 (__VA_ARGS__))
#define EVAL3(...) EVAL2 (EVAL2 (__VA_ARGS__))
#define EVAL4(...) EVAL3 (EVAL3 (__VA_ARGS__))
#define EVAL(...)  EVAL4 (EVAL4 (__VA_ARGS__))

#define MAP_END(...)
#define MAP_OUT

#define MAP_GET_END() 0, MAP_END
#define MAP_NEXT0(test, next, ...) next MAP_OUT
#define MAP_NEXT1(test, next) MAP_NEXT0 (test, next, 0)
#define MAP_NEXT(test, next)  MAP_NEXT1 (MAP_GET_END test, next)

#define MAPTRIPLES0(f, x, y, z, peek, ...) f(x, y, z) MAP_NEXT (peek, MAPTRIPLES1) (f, peek, __VA_ARGS__)
#define MAPTRIPLES1(f, x, y, z, peek, ...) f(x, y, z) MAP_NEXT (peek, MAPTRIPLES0) (f, peek, __VA_ARGS__)
#define MAPTRIPLES(f, ...) EVAL (MAPTRIPLES1 (f, __VA_ARGS__, (), 0))

// helper macros for deducing information about wrapped fields
#define CATEGORY_float 0, numeric
#define CATEGORY_double 0, numeric
#define CATEGORY_int32_t 0, numeric
#define CATEGORY_int64_t 0, numeric
#define CATEGORY_uint32_t 0, numeric
#define CATEGORY_uint64_t 0, numeric
#define CATEGORY_TYPE0(test, sub, ...) sub
#define CATEGORY_TYPE1(test, sub) CATEGORY_TYPE0 (test, sub, 0)
#define CATEGORY_TYPE(type) CATEGORY_TYPE1 (CATEGORY_##type, embedded)

#define GET_WRAPPER_MACRO0(rule, Category) _WRAPPED_##rule##_##Category
#define GET_WRAPPER_MACRO1(rule, Category) GET_WRAPPER_MACRO0(rule, Category)
#define GET_WRAPPER_MACRO(rule, Element) GET_WRAPPER_MACRO1(rule, CATEGORY_TYPE(Element))

#define GET_SEATER_MACRO0(rule, Category) _WRAPPED_##rule##_##Category##_SEATER
#define GET_SEATER_MACRO1(rule, Category) GET_SEATER_MACRO0(rule, Category)
#define GET_SEATER_MACRO(rule, Element) GET_SEATER_MACRO1(rule, CATEGORY_TYPE(Element))

/*
 * macros for wrapping singular fields of numeric type in protobuf msgs
 */
#define _WRAPPED_required_numeric(Element, name) \
public: \
    const Element& name() const {return _##name;} \
    Element* mutable_##name() {return &_##name;} \
    void set_##name(const Element& newVal) {set_##name();}

#define _WRAPPED_optional_numeric(Element, name) \
    _WRAPPED_required_numeric(Element, name) \
    bool has_##name() {return has_##name();}

#define _WRAPPED_required_numeric_SEATER(Element, name)
#define _WRAPPED_optional_numeric_SEATER(Element, name)

/*
 * macros for repeated fields in protobuf msgs
 */
#define _WRAPPED_repeated_numeric(Element, name) \
protected: \
    mutable lm::protowrap::Repeated<Element> _##name; \
public: \
    const lm::protowrap::Repeated<Element>& name() const {return _##name;} \
    lm::protowrap::Repeated<Element>* mutable_##name() {return &_##name;}

#define _WRAPPED_repeated_embedded(Element, name) \
    _WRAPPED_repeated_numeric(Element, name)

#define _WRAPPED_repeated_numeric_SEATER(Element, name) \
    mutable_##name()->setWrappedField(getWrappedMsg()->mutable_##name())

#define _WRAPPED_repeated_embedded_SEATER(Element, name) \
    _WRAPPED_repeated_numeric_SEATER(Element, name)

/*
 * macros for wrapping singular fields of embedded type (ie msg type) in protobuf msgs
 */
#define _WRAPPED_required_embedded(Element, name) \
protected: \
    mutable Element _##name; \
public: \
    const Element& name() const {return _##name;} \
    Element* mutable_##name() {return &_##name;}

#define _WRAPPED_optional_embedded(Element, name) \
    _WRAPPED_required_embedded(Element, name) \
    bool has_##name() {return has_##name();}

#define _WRAPPED_required_embedded_SEATER(Element, name) \
    mutable_##name()->setWrappedMsg(getWrappedMsg()->mutable_##name());

#define _WRAPPED_optional_embedded_SEATER(Element, name) \
    _WRAPPED_required_embedded_SEATER(Element, name)

#define _WRAPPED_FIELD(rule, Element, name) \
    GET_WRAPPER_MACRO(rule, Element)(Element, name)

#define _WRAPPED_SEATER(rule, Element, name) \
    GET_SEATER_MACRO(rule, Element)(Element, name)
//    _WRAPPED_##rule##_##CATEGORY_TYPE##(Element)##_SEATER (Element, name)

#define WRAPPED_FIELDS(...) \
    MAPTRIPLES(_WRAPPED_FIELD, __VA_ARGS__) \
    void setWrappedEmbeddedMsgs() \
    { \
        MAPTRIPLES(_WRAPPED_SEATER, __VA_ARGS__) \
    }

namespace lm {
namespace protowrap {

// this is a base class for the CRTP pattern, and is to be used in derived classes as so -> class derivedMsg: public Msg<derivedMsg>
template<typename DerivedMsg, typename _WrappedMsg> class Msg
{
public:
    typedef _WrappedMsg WrappedMsg;
    typedef DerivedMsg This;

    Msg(): wrappedMsgPtr(NULL) {}
    Msg(WrappedMsg* newMsgPtr): wrappedMsgPtr(NULL) {setWrappedMsg(newMsgPtr);}
    virtual ~Msg() {}

    // conversion operators allow this wrapper to be used wherever google::protobuf::Message could be
    operator WrappedMsg*() {return wrappedMsgPtr;}
    operator WrappedMsg&() const {return *wrappedMsgPtr;}

    virtual WrappedMsg* getWrappedMsg()
    {
        return wrappedMsgPtr;
    }

    virtual void setWrappedMsg(WrappedMsg* newMsgPtr)
    {
        wrappedMsgPtr = newMsgPtr;

        ((*static_cast<This*>(this)).DerivedMsg::setWrapped)();
    }

    static void setWrapped() {}

protected:
    WrappedMsg* wrappedMsgPtr;
};

}
}

#endif //LM_PROTOWRAP_MSG_H_