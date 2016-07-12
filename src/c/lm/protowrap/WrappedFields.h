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

#define MAPQUADS0(f, x, y, z, w, peek, ...) f(x, y, z, w) MAP_NEXT (peek, MAPQUADS1) (f, peek, __VA_ARGS__)
#define MAPQUADS1(f, x, y, z, w, peek, ...) f(x, y, z, w) MAP_NEXT (peek, MAPQUADS0) (f, peek, __VA_ARGS__)
#define MAPQUADS(f, ...) EVAL (MAPQUADS1 (f, __VA_ARGS__, (), 0))

#define PREPEND_COMMA1(...) ,##__VA_ARGS__
#define PREPEND_COMMA(...) PREPEND_COMMA1(__VA_ARGS__)

#define MAPTRIPLES_WCOMMA0(f, x, y, z, peek, ...) PREPEND_COMMA(f(x, y, z)) MAP_NEXT (peek, MAPTRIPLES_WCOMMA1) (f, peek, __VA_ARGS__)
#define MAPTRIPLES_WCOMMA1(f, x, y, z, peek, ...) PREPEND_COMMA(f(x, y, z)) MAP_NEXT (peek, MAPTRIPLES_WCOMMA0) (f, peek, __VA_ARGS__)
#define MAPTRIPLES_WCOMMA2(f, x, y, z, peek, ...) f(x, y, z) MAP_NEXT (peek, MAPTRIPLES_WCOMMA0) (f, peek, __VA_ARGS__)
#define MAPTRIPLES_WCOMMA(f, ...) EVAL (MAPTRIPLES_WCOMMA2 (f, __VA_ARGS__, (), 0))

#define MAPQUADS_WCOMMA0(f, x, y, z, w, peek, ...) PREPEND_COMMA(f(x, y, z, w)) MAP_NEXT (peek, MAPQUADS_WCOMMA1) (f, peek, __VA_ARGS__)
#define MAPQUADS_WCOMMA1(f, x, y, z, w, peek, ...) PREPEND_COMMA(f(x, y, z, w)) MAP_NEXT (peek, MAPQUADS_WCOMMA0) (f, peek, __VA_ARGS__)
#define MAPQUADS_WCOMMA2(f, x, y, z, w, peek, ...) f(x, y, z, w) MAP_NEXT (peek, MAPQUADS_WCOMMA0) (f, peek, __VA_ARGS__)
#define MAPQUADS_WCOMMA(f, ...) EVAL (MAPQUADS_WCOMMA2 (f, __VA_ARGS__, (), 0))

#define APPEND1(...) ,##__VA_ARGS__
#define APPEND(x) x APPEND1
#define TRIPLES_TO_QUADS(w, ...) MAPTRIPLES_WCOMMA(APPEND(w), __VA_ARGS__)

/*
 * - The RESOLVE macro resolves a macro token to its defined value, or to a default value (passed in as a second arg) if the token is undefined
 *     - The catch is that the token has to be defined as `#define TOKEN 0, value` instead of the simpler `#define TOKEN value`
 */
#define RESOLVE0(token, default_token, ...) default_token
#define RESOLVE(token, default_token) RESOLVE0(token, default_token, 0)
 
#define EXPAND_AND_EVAL(f, ...) f(__VA_ARGS__)

/*
 * - helper macros for deducing information about wrapped fields
 *     - CATEGORY_TYPE(x) returns numeric is x is a numeric type, and embedded otherwise
 */
#define CATEGORY_TYPE_bool     0, numeric
#define CATEGORY_TYPE_float    0, numeric
#define CATEGORY_TYPE_double   0, numeric
#define CATEGORY_TYPE_int32_t  0, numeric
#define CATEGORY_TYPE_int64_t  0, numeric
#define CATEGORY_TYPE_uint32_t 0, numeric
#define CATEGORY_TYPE_uint64_t 0, numeric

#define CATEGORY_RULE_required_enum 0, numeric
#define CATEGORY_RULE_optional_enum 0, numeric
#define CATEGORY_RULE_repeated_enum 0, numeric

#define GET_CATEGORY(rule, type) RESOLVE(CATEGORY_TYPE_##type, RESOLVE(CATEGORY_RULE_##rule, embedded))

#define RULE_required_enum 0, required
#define RULE_optional_enum 0, optional
#define RULE_repeated_enum 0, repeated

#define GET_RULE(rule) RESOLVE(RULE_##rule, rule)

/*
 * - macros for wrapping singular fields of numeric type in protobuf msgs
 */
#define _WRAPPED_required_numeric_ATTR(Element, name)                                     \
public:                                                                                   \
    inline void clear_##name() {wrappedMsgPtr->clear_##name();}                           \
    inline const Element name() const {return wrappedMsgPtr->name();}                     \
    inline void set_##name(const Element& newVal) {wrappedMsgPtr->set_##name(newVal);}    \
    inline bool has_##name() const {return wrappedMsgPtr->has_##name();}

#define _WRAPPED_optional_numeric_ATTR(Element, name)    \
    _WRAPPED_required_numeric_ATTR(Element, name)

#define _WRAPPED_required_numeric_SEATER(Element, name)
#define _WRAPPED_optional_numeric_SEATER(Element, name)


#define _WRAPPED_required_numeric_DESERIALIZETO_CONTAINER_TEMPLATE_SIGNATURE(Element, name)
#define _WRAPPED_optional_numeric_DESERIALIZETO_CONTAINER_TEMPLATE_SIGNATURE(Element, name)
#define _WRAPPED_required_numeric_DESERIALIZETO_CONTAINER_SIGNATURE(Element, name)    \
    Element* name##_writeto

#define _WRAPPED_optional_numeric_DESERIALIZETO_CONTAINER_SIGNATURE(Element, name)    \
    *bool has_##name##_writeto, Element* name##_writeto
 
#define _WRAPPED_required_numeric_DESERIALIZETO_CONTAINER(Element, name)     \
    *name##_writeto = name();

#define _WRAPPED_optional_numeric_DESERIALIZETO_CONTAINER(Element, name)     \
    *has_##name##_writeto = has_##name()                                     \
    if (*has_##name##_writeto) *name##_writeto = name();


#define _WRAPPED_required_numeric_SERIALIZEFROM_CONTAINER_TEMPLATE_SIGNATURE(Element, name)
#define _WRAPPED_optional_numeric_SERIALIZEFROM_CONTAINER_TEMPLATE_SIGNATURE(Element, name)
#define _WRAPPED_required_numeric_SERIALIZEFROM_CONTAINER_SIGNATURE(Element, name)    \
    Element name##_readfrom

#define _WRAPPED_optional_numeric_SERIALIZEFROM_CONTAINER_SIGNATURE(Element, name)    \
    bool has_##name##_readfrom, Element name##_readfrom
 
#define _WRAPPED_required_numeric_SERIALIZEFROM_CONTAINER(Element, name)    \
    set_##name(name##_readfrom);

#define _WRAPPED_optional_numeric_SERIALIZEFROM_CONTAINER(Element, name)    \
    if (has_##name##_readfrom) set_##name(name##_readfrom);


/*
 * - macros for wrapping singular fields of embedded type (ie msg type) in protobuf msgs
 */
#define _WRAPPED_required_embedded_ATTR(Element, name)                     \
protected:                                                                 \
    mutable Element _##name;                                               \
public:                                                                    \
    inline const Element& name() const {return _##name;}                   \
    inline Element* mutable_##name() {return &_##name;}                    \
    inline void clear_##name() {_##name.Clear();}                          \
    inline bool has_##name() const {return wrappedMsgPtr->has_##name();}

#define _WRAPPED_optional_embedded_ATTR(Element, name)    \
    _WRAPPED_required_embedded_ATTR(Element, name)

#define _WRAPPED_required_embedded_SEATER(Element, name)                \
    mutable_##name()->setWrappedMsg(wrappedMsgPtr->mutable_##name());

#define _WRAPPED_optional_embedded_SEATER(Element, name)    \
    _WRAPPED_required_embedded_SEATER(Element, name)


#define _WRAPPED_required_embedded_DESERIALIZETO_CONTAINER_TEMPLATE_SIGNATURE(Element, name)
#define _WRAPPED_optional_embedded_DESERIALIZETO_CONTAINER_TEMPLATE_SIGNATURE(Element, name)
#define _WRAPPED_required_embedded_DESERIALIZETO_CONTAINER_SIGNATURE(Element, name)
#define _WRAPPED_optional_embedded_DESERIALIZETO_CONTAINER_SIGNATURE(Element, name)
#define _WRAPPED_required_embedded_DESERIALIZETO_CONTAINER(Element, name)
#define _WRAPPED_optional_embedded_DESERIALIZETO_CONTAINER(Element, name)

#define _WRAPPED_required_embedded_SERIALIZEFROM_CONTAINER_TEMPLATE_SIGNATURE(Element, name)
#define _WRAPPED_optional_embedded_SERIALIZEFROM_CONTAINER_TEMPLATE_SIGNATURE(Element, name)
#define _WRAPPED_required_embedded_SERIALIZEFROM_CONTAINER_SIGNATURE(Element, name)
#define _WRAPPED_optional_embedded_SERIALIZEFROM_CONTAINER_SIGNATURE(Element, name)
#define _WRAPPED_required_embedded_SERIALIZEFROM_CONTAINER(Element, name)
#define _WRAPPED_optional_embedded_SERIALIZEFROM_CONTAINER(Element, name)

/*
 * - macros for repeated fields in protobuf msgs
 */
#define _WRAPPED_repeated_base_ATTR(Element, name)                                   \
protected:                                                                           \
    mutable lm::protowrap::Repeated<Element> _##name;                                \
public:                                                                              \
    inline void clear_##name() {_##name.Clear();}                                    \
    inline const lm::protowrap::Repeated<Element>& name() const {return _##name;}    \
    inline lm::protowrap::Repeated<Element>* mutable_##name() {return &_##name;}     \
    inline Element name(int index) const {return _##name(index);}                    \
    inline int name##_size() const {return _##name.size();}

#define _WRAPPED_repeated_numeric_ATTR(Element, name)                                \
    _WRAPPED_repeated_base_ATTR(Element, name)                                       \
    inline void add_##name(Element value) {_##name.Add(value);}                      \
    inline void set_##name(int index, Element value) {_##name.Set(index, value);}

#define _WRAPPED_repeated_embedded_ATTR(Element, name)                               \
    _WRAPPED_repeated_base_ATTR(Element, name)                                       \
    inline Element* add_##name() {return _##name.Add();}


#define _WRAPPED_repeated_numeric_SEATER(Element, name)                              \
    mutable_##name()->setWrappedField(wrappedMsgPtr->mutable_##name());

#define _WRAPPED_repeated_embedded_SEATER(Element, name)                             \
    _WRAPPED_repeated_numeric_SEATER(Element, name)


#define _WRAPPED_repeated_numeric_DESERIALIZETO_CONTAINER_TEMPLATE_SIGNATURE(Element, name)     \
    typename name##_Container
#define _WRAPPED_repeated_numeric_DESERIALIZETO_CONTAINER_SIGNATURE(Element, name)              \
    name##_Container* name##_writeto
#define _WRAPPED_repeated_numeric_DESERIALIZETO_CONTAINER(Element, name)                        \
    name().deserializeTo(name##_writeto);

#define _WRAPPED_repeated_embedded_DESERIALIZETO_CONTAINER_TEMPLATE_SIGNATURE(Element, name)
#define _WRAPPED_repeated_embedded_DESERIALIZETO_CONTAINER_SIGNATURE(Element, name)
#define _WRAPPED_repeated_embedded_DESERIALIZETO_CONTAINER(Element, name)


#define _WRAPPED_repeated_numeric_SERIALIZEFROM_CONTAINER_TEMPLATE_SIGNATURE(Element, name)     \
    typename name##_Container
#define _WRAPPED_repeated_numeric_SERIALIZEFROM_CONTAINER_SIGNATURE(Element, name)              \
    const name##_Container& name##_readfrom    
#define _WRAPPED_repeated_numeric_SERIALIZEFROM_CONTAINER(Element, name)                        \
    mutable_##name()->serializeFrom(name##_readfrom);

#define _WRAPPED_repeated_embedded_SERIALIZEFROM_CONTAINER_TEMPLATE_SIGNATURE(Element, name)
#define _WRAPPED_repeated_embedded_SERIALIZEFROM_CONTAINER_SIGNATURE(Element, name)
#define _WRAPPED_repeated_embedded_SERIALIZEFROM_CONTAINER(Element, name)

/*
 * - these macros allow us to "lookup" the wrapper macros defined above as needed
 */
#define _GET_MACRO0(rule, Category, kind)                                              \
    _WRAPPED_##rule##_##Category##_##kind

#define _GET_MACRO(rule, Element, kind)                                                \
    EXPAND_AND_EVAL(_GET_MACRO0, GET_RULE(rule), GET_CATEGORY(rule, Element), kind)

#define _MAKE_WRAPPER_FOR(kind, rule, Element, name)                                   \
    _GET_MACRO(rule, Element, kind)(Element, name)

/*
 * - the macros that will be called directly by MAPTRIPLES
 */

#define _MAP_WRAPPERS(kind, ...)                                              \
    MAPQUADS(_MAKE_WRAPPER_FOR, TRIPLES_TO_QUADS(kind, __VA_ARGS__))

#define _MAP_WRAPPERS_WCOMMA(kind, ...)                                       \
    MAPQUADS_WCOMMA(_MAKE_WRAPPER_FOR, TRIPLES_TO_QUADS(kind, __VA_ARGS__))

/*
 * - the implementation macros
 */
#define WRAPPED_FIELDS(...)                         \
    _MAP_WRAPPERS(ATTR, __VA_ARGS__)                \
                                                    \
    void _macro_setWrapped()                        \
    {                                               \
        _MAP_WRAPPERS(SEATER, __VA_ARGS__)          \
    }

#define WRAPPED_FIELDS_W_SERIALIZERS(...)                                                       \
    WRAPPED_FIELDS(__VA_ARGS__)                                                                 \
                                                                                                \
    template <_MAP_WRAPPERS_WCOMMA(DESERIALIZETO_CONTAINER_TEMPLATE_SIGNATURE, __VA_ARGS__)>    \
    void deserializeTo(_MAP_WRAPPERS_WCOMMA(DESERIALIZETO_CONTAINER_SIGNATURE, __VA_ARGS__))    \
    {                                                                                           \
        _MAP_WRAPPERS(DESERIALIZETO_CONTAINER, __VA_ARGS__)                                     \
    }                                                                                           \
                                                                                                \
    template <_MAP_WRAPPERS_WCOMMA(SERIALIZEFROM_CONTAINER_TEMPLATE_SIGNATURE, __VA_ARGS__)>    \
    void serializeFrom(_MAP_WRAPPERS_WCOMMA(SERIALIZEFROM_CONTAINER_SIGNATURE, __VA_ARGS__))    \
    {                                                                                           \
        _MAP_WRAPPERS(SERIALIZEFROM_CONTAINER, __VA_ARGS__)                                     \
    }                                                                                           \

#define MSG_WRAP_CONSTRUCTORS(MsgWrapperClass)                            \
public:                                                                   \
    MsgWrapperClass() {};                                                 \
    MsgWrapperClass(WrappedMsg* newMsgPtr) {setWrappedMsg(newMsgPtr);}    \
    virtual ~MsgWrapperClass() {}


#endif /* LM_PROTOWRAP_WRAPPEDFIELDS_H_ */