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

// MAP macro modified from https://github.com/swansontec/map-macro
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

#define MAP0(f, x, peek, ...) f(x) MAP_NEXT (peek, MAP1) (f, peek, __VA_ARGS__)
#define MAP1(f, x, peek, ...) f(x) MAP_NEXT (peek, MAP0) (f, peek, __VA_ARGS__)
#define MAP(f, ...) EVAL (MAP1 (f, __VA_ARGS__, (), 0))

#define MAPPAIRS0(f, x, y, peek, ...) f(x, y) MAP_NEXT (peek, MAPPAIRS1) (f, peek, __VA_ARGS__)
#define MAPPAIRS1(f, x, y, peek, ...) f(x, y) MAP_NEXT (peek, MAPPAIRS0) (f, peek, __VA_ARGS__)
#define MAPPAIRS(f, ...) EVAL (MAPPAIRS1 (f, __VA_ARGS__, (), 0))

/*
 * macros for wrapping singular fields of numeric type in protobuf msgs
 */
#define _WRAPPED_FIELD(fieldName, Element) \
public: \
    const Element& fieldName() const {return _##fieldName;}; \
    Element* mutable_##fieldName() {return &_##fieldName;}; \
    void set_##fieldName(const Element& newVal) {set_##fieldName();};

#define WRAPPED_FIELD(...) \
    MAPPAIRS(_WRAPPED_FIELD, __VA_ARGS__)

/*
 * macros for repeated fields in protobuf msgs
 */
#define _WRAPPED_REPEATED(fieldName, Element) \
protected: \
    mutable lm::protowrap::Repeated<Element> _##fieldName; \
public: \
    const lm::protowrap::Repeated<Element>& fieldName() const {return _##fieldName;}; \
    lm::protowrap::Repeated<Element>* mutable_##fieldName() {return &_##fieldName;};

#define _WRAPPED_REPEATED_SEATER(fieldName, Element) \
    mutable_##fieldName()->setWrappedField(getWrappedMsg()->mutable_##fieldName());

#define WRAPPED_REPEATED(...) \
    MAPPAIRS(_WRAPPED_REPEATED, __VA_ARGS__) \
    void setWrappedRepeatedFields() \
    { \
        MAPPAIRS(_WRAPPED_REPEATED_SEATER, __VA_ARGS__) \
    }

/*
 * macros for wrapping singular fields of embedded type (ie msg type) in protobuf msgs
 */
#define _WRAPPED_EMBEDDED(fieldName, Msg) \
protected: \
    mutable Msg _##fieldName; \
public: \
    const Msg& fieldName() const {return _##fieldName;}; \
    Msg* mutable_##fieldName() {return &_##fieldName;};

#define WRAPPED_EMBEDDED_SEATER(fieldName, Msg) \
    mutable_##fieldName()->setWrappedMsg(getWrappedMsg()->mutable_##fieldName());

#define WRAPPED_EMBEDDED(...) \
    MAPPAIRS(_WRAPPED_EMBEDDED, __VA_ARGS__) \
    void setWrappedEmbeddedMsgs() \
    { \
        MAPPAIRS(WRAPPED_EMBEDDED_SEATER, __VA_ARGS__) \
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

        ((*static_cast<This*>(this)).DerivedMsg::setWrappedEmbeddedMsgs)();
        ((*static_cast<This*>(this)).DerivedMsg::setWrappedRepeatedFields)();
    }

    static void setWrappedEmbeddedMsgs() {}
    static void setWrappedRepeatedFields() {}

protected:
    WrappedMsg* wrappedMsgPtr;
};

}
}

#endif //LM_PROTOWRAP_MSG_H_