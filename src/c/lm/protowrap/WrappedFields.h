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
#ifndef LM_PROTOWRAP_WRAPPEDFIELDS_H_
#define LM_PROTOWRAP_WRAPPEDFIELDS_H_

/*
 * - this file contains the WRAPPED_FIELDS macro, to help with making wrapper classes for protocol buffer messages
 *     - if the message definition in your .proto file looks like this
 *         '''
 *         message Foo {
 *             repeated double     field1 = 1 [packed=true];
 *             optional BarMessage field2 = 2;
 *         '''
 *     - then the wrapper class in your .h file would look like this
 *         '''
 *         class FooWrap {
 *             WRAPPED_FIELDS(repeated, double,         field1,
 *                            optional, BarMessageWrap, field2)
 *         public:
 *             //// whatever custom wrapper code you'd like to add ////
 *         };
 *         '''
 *     - some notes
 *         - the effect of the WRAPPED_FIELDS call is to effectively "forward" all of the basic protobuf api methods from Foo to FooWrap
 *         - sadly, this can't be done with templates. Macros are required due to the need for setting the actual names of attributes and methods
 */

/*
 * - MAPTRIPLES macro, modified from https://github.com/swansontec/map-macro
 *     - When passed a function-style macro and a list of arguments, it'll apply them, functional programming style, 3 at a time
 *         - MAPTRIPLES(FOO, a, b, c, d, e, f, ...) -> FOO(a, b, c) FOO(d, e, f) ...
 *     - EVAL has been configured to allow fo a max recursion depth of 64
 */
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


#define MAPTRIPLES_WSEP0(f, separator, x, y, z, peek, ...) f(x, y, z)separator MAP_NEXT (peek, MAPTRIPLES_WSEP1) (f, separator, peek, __VA_ARGS__)
#define MAPTRIPLES_WSEP1(f, separator, x, y, z, peek, ...) f(x, y, z)separator MAP_NEXT (peek, MAPTRIPLES_WSEP0) (f, separator, peek, __VA_ARGS__)
#define MAPTRIPLES_WSEP(f, separator, ...) EVAL (MAPTRIPLES_WSEP1 (f, separator, __VA_ARGS__, (), 0))

/*
#define MAP_WSEP_GET_END() 0, MAP_WSEP_END
#define MAP_WSEP_NEXT0(test, next, ...) next MAP_WSEP_OUT
#define MAP_WSEP_NEXT1(test, next) MAP_WSEP_NEXT0 (test, next, 0)
#define MAP_WSEP_NEXT(test, next)  MAP_WSEP_NEXT1 (MAP_WSEP_GET_END test, next)

#define MAPTRIPLES_WSEP0(f, x, y, z, peek, ...) f(x, y, z) MAP_WSEP_NEXT (peek, MAPTRIPLES_WSEP1) (f, peek, __VA_ARGS__)
#define MAPTRIPLES_WSEP1(f, x, y, z, peek, ...) f(x, y, z) MAP_WSEP_NEXT (peek, MAPTRIPLES_WSEP0) (f, peek, __VA_ARGS__)
#define MAPTRIPLES_WSEP(f, ...) EVAL (MAPTRIPLES_WSEP1 (f, __VA_ARGS__, (), 0))
 */

/*
 * - helper macros for deducing information about wrapped fields
 *     - CATEGORY_TYPE(x) returns numeric is x is a numeric type, and embedded otherwise
 */
#define CATEGORY_bool     0, numeric
#define CATEGORY_float    0, numeric
#define CATEGORY_double   0, numeric
#define CATEGORY_int32_t  0, numeric
#define CATEGORY_int64_t  0, numeric
#define CATEGORY_uint32_t 0, numeric
#define CATEGORY_uint64_t 0, numeric

/*
 * - The RESOLVE macro resolves a macro token to its defined value, or to a default value (passed in as a second arg) if the token is undefined
 *     - The catch is that the token has to be defined as `0, value` instead of the simpler `value`
 */
#define RESOLVE0(token, default_token, ...) sub
#define RESOLVE(token, default_token) RESOLVE0(token, default_token, 0)
#define CATEGORY_TYPE(type) RESOLVE(CATEGORY_##type, embedded)
*/

/*
 * - macros for wrapping singular fields of numeric type in protobuf msgs
 */
#define _WRAPPED_required_numeric(Element, name)                                   \
public:                                                                            \
    void clear_##name() {wrappedMsgPtr->clear_##name();}                           \
    const Element name() const {return wrappedMsgPtr->name();}                     \
    void set_##name(const Element& newVal) {wrappedMsgPtr->set_##name(newVal);}    \
    bool has_##name() const {return wrappedMsgPtr->has_##name();}

#define _WRAPPED_optional_numeric(Element, name)    \
    _WRAPPED_required_numeric(Element, name)

#define _WRAPPED_required_numeric_SEATER(Element, name)

#define _WRAPPED_optional_numeric_SEATER(Element, name)

 
#define _WRAPPED_required_numeric_DESERIALIZETO_SIGNATURE(Element, name)    \
    Element* name##_writeto

 #define _WRAPPED_optional_numeric_DESERIALIZETO_SIGNATURE(Element, name)    \
    *bool has_##name##_writeto, Element* name##_writeto
 
#define _WRAPPED_required_numeric_DESERIALIZETO(Element, name)     \
    *name##_writeto = name();

#define _WRAPPED_optional_numeric_DESERIALIZETO(Element, name)     \
    *has_##name##_writeto = has_##name()                           \
    if (*has_##name##_writeto) *name##_writeto = name();

 
#define _WRAPPED_required_numeric_SERIALIZEFROM_SIGNATURE(Element, name)    \
    Element name##_readfrom

#define _WRAPPED_optional_numeric_SERIALIZEFROM_SIGNATURE(Element, name)    \
    bool has_##name##_readfrom, Element name##_readfrom
 
#define _WRAPPED_required_numeric_SERIALIZEFROM(Element, name)    \
    set_##name(name##_readfrom);

#define _WRAPPED_optional_numeric_SERIALIZEFROM(Element, name)    \
    if (has_##name##_readfrom) set_##name(name##_readfrom);


/*
 * - macros for wrapping singular fields of embedded type (ie msg type) in protobuf msgs
 */
#define _WRAPPED_required_embedded(Element, name) \
protected: \
    mutable Element _##name; \
public: \
    const Element& name() const {return _##name;} \
    Element* mutable_##name() {return &_##name;} \
    void clear_##name() {_##name.Clear();} \
    bool has_##name() const {return wrappedMsgPtr->has_##name();}

#define _WRAPPED_optional_embedded(Element, name) \
    _WRAPPED_required_embedded(Element, name)

#define _WRAPPED_required_embedded_SEATER(Element, name) \
    mutable_##name()->setWrappedMsg(wrappedMsgPtr->mutable_##name());

#define _WRAPPED_optional_embedded_SEATER(Element, name) \
    _WRAPPED_required_embedded_SEATER(Element, name)

/*
 * - macros for repeated fields in protobuf msgs
 */
#define _WRAPPED_repeated_base(Element, name) \
protected: \
    mutable lm::protowrap::Repeated<Element> _##name; \
public: \
    void clear_##name() {_##name.Clear();} \
    const lm::protowrap::Repeated<Element>& name() const {return _##name;} \
    lm::protowrap::Repeated<Element>* mutable_##name() {return &_##name;} \
    Element name(int index) const {return _##name(index);} \
    int name##_size() const {return _##name.size();}

#define _WRAPPED_repeated_numeric(Element, name) \
    _WRAPPED_repeated_base(Element, name) \
    void add_##name(Element value) {_##name.Add(value);} \
    void set_##name(int index, Element value) {_##name.Set(index, value);}

#define _WRAPPED_repeated_embedded(Element, name) \
    _WRAPPED_repeated_base(Element, name) \
    Element* add_##name() {return _##name.Add();}

#define _WRAPPED_repeated_numeric_SEATER(Element, name) \
    mutable_##name()->setWrappedField(wrappedMsgPtr->mutable_##name());

#define _WRAPPED_repeated_embedded_SEATER(Element, name) \
    _WRAPPED_repeated_numeric_SEATER(Element, name)

/*
 * - these macros allow us to "lookup" the wrapper macros defined above as needed
 */
#define GET_WRAPPER_MACRO0(rule, Category) _WRAPPED_##rule##_##Category
#define GET_WRAPPER_MACRO1(rule, Category) GET_WRAPPER_MACRO0(rule, Category)
#define GET_WRAPPER_MACRO(rule, Element) GET_WRAPPER_MACRO1(rule, CATEGORY_TYPE(Element))

#define GET_SEATER_MACRO0(rule, Category) _WRAPPED_##rule##_##Category##_SEATER
#define GET_SEATER_MACRO1(rule, Category) GET_SEATER_MACRO0(rule, Category)
#define GET_SEATER_MACRO(rule, Element) GET_SEATER_MACRO1(rule, CATEGORY_TYPE(Element))

/*
 * - the macros that will be called directly by MAPTRIPLES
 */
#define _WRAPPED_FIELD(rule, Element, name) \
    GET_WRAPPER_MACRO(rule, Element)(Element, name)

#define _WRAPPED_SEATER(rule, Element, name) \
    GET_SEATER_MACRO(rule, Element)(Element, name)

/*
#define _WRAPPED_DESERIALIZETO(rule, Element, name)         \
    GET_SEATER_MACRO(rule, Element)(Element, name)
 */

/*
 * - the implementation macros
 */
#define WRAPPED_FIELDS(...) \
    MAPTRIPLES(_WRAPPED_FIELD, __VA_ARGS__) \
    void _macro_setWrapped() \
    { \
        MAPTRIPLES(_WRAPPED_SEATER, __VA_ARGS__) \
    }


/*
#define WRAPPED_FIELDS_W_SERIALIZERS(...)   \
    WRAPPED_FIELDS(__VA_ARGS__)             \
    void deserializeTo(MAPTRIPLES_WSEP(_))
*/

#define MSG_WRAP_CONSTRUCTORS(MsgWrapperClass)                            \
public:                                                                   \
    MsgWrapperClass() {};                                                 \
    MsgWrapperClass(WrappedMsg* newMsgPtr) {setWrappedMsg(newMsgPtr);}    \
    virtual ~MsgWrapperClass() {}


#endif /* LM_PROTOWRAP_WRAPPEDFIELDS_H_ */