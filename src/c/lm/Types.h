/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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

#ifndef TYPES_H_
#define TYPES_H_

// Some !#@*@(& macros interfere with the definition of the BIG_ENDIAN and LITTLE_ENDIAN ByteOrder enums in NDArray.pb.h, so fix that
//#ifdef BIG_ENDIAN
//#undef BIG_ENDIAN
//#endif
//#ifdef LITTLE_ENDIAN
//#undef LITTLE_ENDIAN
//#endif

#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <utility>
#include <vector>

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <limits>

#include "lm/Exceptions.h"
#include "robertslab/Types.h"

/*
 * General types.
 */

typedef unsigned char       uchar;
typedef unsigned int        uint;
typedef unsigned long       ulong;

typedef long                intv_t;
typedef unsigned long       uintv_t;

typedef uint8_t 		    byte;

/*
 * Physical types.
 */
typedef double              si_dist_t;
typedef double              si_time_t;

/*
 * AVX types.
 */
#ifdef OPT_AVX
#include <immintrin.h>
#define DOUBLES_PER_AVX 4
#define INT32S_PER_AVX 8
#define avxd __m256d
#define avxi __m256i
#endif

#if defined(OPT_AVX) && !defined(OPT_FMA)
#define _mm256_fmadd_pd(a,b,c) _mm256_add_pd(_mm256_mul_pd(a,b),c)
#endif

/*
 *  Array types.
 */

template<typename T0, typename T1> struct PairVector
{
    typedef std::pair<T0, T1> Pair;
    typedef std::vector<Pair> T;
    typedef typename T::iterator iterator;
    typedef typename T::const_iterator const_iterator;

    T vec;
};

template<typename Key0, typename Key1, typename Value> struct PairMap
{
    typedef std::pair<Key0, Key1> Key;
    typedef std::map<Key, Value> T;
    typedef typename T::iterator iterator;
    typedef typename T::const_iterator const_iterator;

    T map;
};

/*
 * type traits
 */

// qualifier adders/removers. Used by other type traits, so listed first
template< class T> struct AddConst {typedef const T type;};

template<typename T> struct RemoveConst {typedef T type;};
template<typename T> struct RemoveConst<T const> {typedef T type;};
template<typename T> struct RemoveVolatile {typedef T type;};
template<typename T> struct RemoveVolatile<T volatile> {typedef T type;};
template<typename T> struct RemoveCV {typedef typename RemoveConst<typename RemoveVolatile<T>::type>::type type;};

template<typename T> struct RemovePointer {typedef T type;};
template<typename T> struct RemovePointer<T*> {typedef T type;};
template<typename T> struct RemovePointer<T* const> {typedef T type;};
template<typename T> struct RemovePointer<T* volatile> {typedef T type;};
template<typename T> struct RemovePointer<T* const volatile> {typedef T type;};

// the rest of the type traits
template <bool cond, class T=void> struct EnableIf {typedef T type;};
template <class T> struct EnableIf<false, T> {};

template <bool cond, class T=void> struct EnableIfNot {typedef T type;};
template <class T> struct EnableIfNot<true, T> {};

template <typename T> struct HasBegin
{
    template <typename U, typename U::iterator (U::*)()> struct Test;
    template <typename U> static char test(Test<U, &U::begin> *);
    template <typename U> static int test(...);

    static const bool value = sizeof(test<T>(0)) == sizeof(char);
};

template <typename T> struct HasEnd
{
    template <typename U, typename U::iterator (U::*)()> struct Test;
    template <typename U> static char test(Test<U, &U::end> *);
    template <typename U> static int test(...);

    static const bool value = sizeof(test<T>(0)) == sizeof(char);
};

template <typename T> struct HasBeginEnd
{
    static const bool value = HasBegin<T>::value and HasEnd<T>::value;
};

template <typename T> struct HasPushBack
{
    template <typename U, void (U::*)(const typename U::value_type&)> struct Test;
    template <typename U> static char test(Test<U, &U::push_back> *);
    template <typename U> static int test(...);

    static const bool value = sizeof(test<T>(0)) == sizeof(char);
};

template <typename T, typename=void> struct IsIterable {static const bool value = false;};
template <typename T> struct IsIterable<T, typename T::Iterator> {static const bool value = true;};

template<class T> struct IsPointerHelper {static const bool value = false;};
template<class T> struct IsPointerHelper<T*> {static const bool value = true;};
template<class T> struct IsPointer {static const bool value = IsPointerHelper<typename RemoveCV<T>::type>::value;};

template <typename T> struct IsNumeric {static const bool value = std::numeric_limits<T>::is_specialized;};

template <typename T, typename=void> struct IsIntegral {static const bool value = false;};
template <typename T> struct IsIntegral<T, typename EnableIf<IsNumeric<T>::value>::type> {static const bool value = std::numeric_limits<T>::is_integer;};

template <typename T, typename U> struct IsSame {static const bool value = false;};
template <typename T> struct IsSame<T, T> {static const bool value = true;};

template <typename, template <typename> class> struct IsSameTemplate {static const bool value = false;};
template <template <typename> class T, template <typename> class U, typename Param> struct IsSameTemplate<T<Param>, U> {static const bool value = IsSame<T<Param>, U<Param> >::value;};

// SFINAE friendly version of iterator_traits
template<typename Iterator, typename=void, typename=void, typename=void, typename=void, typename=void, typename=void, typename=void>
struct IteratorTraits
{
    static const bool is_specialized = false;
};

template<typename Iterator>
struct IteratorTraits<Iterator, typename Iterator::iterator_category, typename Iterator::value_type, typename Iterator::difference_type, typename Iterator::pointer, typename Iterator::reference>
{
    static const bool is_specialized = true;

    typedef typename Iterator::iterator_category iterator_category;
    typedef typename Iterator::value_type        value_type;
    typedef typename Iterator::difference_type   difference_type;
    typedef typename Iterator::pointer           pointer;
    typedef typename Iterator::reference         reference;

    typedef void container_type;
    typedef void const_reference;
};

// specialization for back_insert_iterators
template<typename Iterator>
struct IteratorTraits<Iterator, typename Iterator::container_type>
{
    static const bool is_specialized = true;

    typedef typename Iterator::container_type::value_type value_type;
    typedef typename Iterator::container_type             container_type;

    typedef void iterator_category;
    typedef void difference_type;
    typedef void pointer;
    typedef void reference;
    typedef void const_reference;
};

// if the iterator has none of the relevant typedefs, assume that it is in fact just a c-style pointer
template<typename Iterator>
struct IteratorTraits<Iterator, typename EnableIf<IsPointer<Iterator>::value>::type>
{
    static const bool is_specialized = true;

    typedef typename RemovePointer<Iterator>::type value_type;
    typedef typename RemovePointer<Iterator>::type* pointer;
    typedef typename RemovePointer<Iterator>::type& reference;
    typedef const typename RemovePointer<Iterator>::type& const_reference;

    typedef void container_type;
    typedef void iterator_category;
    typedef void difference_type;
};

/*
template<typename B, typename D>
struct IsBaseOf {
    typedef char (&yes)[1];
    typedef char (&no)[2];

#if defined(MACOSX)
#undef check
#endif

    static yes check(const B*);
    static no check(const void*);

    enum {
        value = sizeof(check(static_cast<const D*>(NULL))) == sizeof(yes),
    };
};

template<typename T> struct disable_if_void {template <typename This, typename Func> static T* call(This* _this, Func func) {return (*_this.*func)();}};
template<> struct disable_if_void<void> {template <typename This, typename Func> static void* call(This* _this, Func func) {return NULL;}};

template<typename T>
struct IsIterator {static const bool value = IteratorTraits<T>::is_specialized;};
//struct IsIterator<T, typename EnableIf<not IsSame<typename lm::internal::iterator_traits<T>::value_type, void>::value>::type> {static const bool value = true;};


template <typename T>
struct IsNumericIterator {static const bool value = IsIterator<T>::value and IsNumeric<typename IteratorTraits<T>::value_type>::value;};

 */

#endif /* TYPES_H_ */
