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
 * Author(s): Max Klein
 */
#ifndef LM_VECTORMATH_H_
#define LM_VECTORMATH_H_

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>

#include "lm/Iterator.h"
#include "lm/Types.h"

template<typename T> struct TypePrecedence { static const int value = -1; };
template<> struct TypePrecedence<long double> { static const int value = 0; };
template<> struct TypePrecedence<double> { static const int value = 1; };
template<> struct TypePrecedence<float> { static const int value = 2; };
template<> struct TypePrecedence<unsigned long long> { static const int value = 3; };
template<> struct TypePrecedence<long long> { static const int value = 4; };
template<> struct TypePrecedence<unsigned long> { static const int value = 5; };
template<> struct TypePrecedence<long> { static const int value = 6; };
template<> struct TypePrecedence<unsigned int> { static const int value = 7; };
template<> struct TypePrecedence<int> { static const int value = 8; };
template<> struct TypePrecedence<unsigned short> { static const int value = 9; };
template<> struct TypePrecedence<short> { static const int value = 10; };
template<> struct TypePrecedence<unsigned char> { static const int value = 11; };
template<> struct TypePrecedence<char> { static const int value = 12; };
template<> struct TypePrecedence<bool> { static const int value = 13; };

/*
 * - PrecendentType has the same return type as an arithmetic operation on types T and U
 */
template<typename T, typename U, bool=((TypePrecedence<T>::value)<=(TypePrecedence<U>::value))>
struct PrecedentType {
    typedef T type;
};
template<typename T, typename U>
struct PrecedentType<T, U, false> {
    typedef U type;
};


/*
 * - some functors, stored in a namespace to distinguish them from any STL functors
 */
namespace lm {

/*
 * - versions of the arithmetic functors from <functional> that can do type promotion
 */
template <typename T, typename U=T> struct plus
{
    // these typedefs are for conformance with the Operation concept expected by e.g. std::bind1st
    typedef T first_argument_type;
    typedef U second_argument_type;
    typedef typename PrecedentType<T, U>::type result_type;
    
    result_type operator() (const T& x, const U& y) const {return x + y;}
};

template <typename T, typename U=T> struct minus
{
    // these typedefs are for conformance with the Operation concept expected by e.g. std::bind1st
    typedef T first_argument_type;
    typedef U second_argument_type;
    typedef typename PrecedentType<T, U>::type result_type;

    result_type operator() (const T& x, const U& y) const {return x - y;}
};

template <typename T, typename U=T> struct multiplies
{
    // these typedefs are for conformance with the Operation concept expected by e.g. std::bind1st
    typedef T first_argument_type;
    typedef U second_argument_type;
    typedef typename PrecedentType<T, U>::type result_type;

    result_type operator() (const T& x, const U& y) const {return x * y;}
};

template <typename T, typename U=T> struct divides
{
    // these typedefs are for conformance with the Operation concept expected by e.g. std::bind1st
    typedef T first_argument_type;
    typedef U second_argument_type;
    typedef typename PrecedentType<T, U>::type result_type;

    result_type operator() (const T& x, const U& y) const {return x / y;}
};

/*
 * - less simple math functors that aren't in STL in the first place
 */

template <typename T, typename U=T> struct exponentiates
{
    // these typedefs are for conformance with the Operation concept expected by e.g. std::bind1st
    typedef T first_argument_type;
    typedef U second_argument_type;
    typedef typename PrecedentType<T, U>::type result_type;

    result_type operator() (const T& base, const U& exponent) const {return pow(base, exponent);}
};

}

/*
 * - This section contains implementations of the basic arithmatic operators (+ - * /) for stl vectors
 *     - all operations are piecewise
 *     - scalar-vector and vector-vector operators are provided
 *     - the return value is a newly initialized vector in all cases, so these are not meant for inner loop usage
 */

/*
 * - scalar-vector operators
 */
template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::plus<T, Scalar>::result_type> >::type
operator+ (Scalar lhs, const std::vector<T>& rhs)
{
    std::vector<typename lm::plus<T, Scalar>::result_type> retVal;
    std::transform(rhs.begin(), rhs.end(), std::back_inserter(retVal), std::bind1st(lm::plus<Scalar, T>(), lhs));
    return retVal;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::minus<T, Scalar>::result_type> >::type
operator- (Scalar lhs, const std::vector<T>& rhs)
{
    std::vector<typename lm::minus<T, Scalar>::result_type> retVal;
    std::transform(rhs.begin(), rhs.end(), std::back_inserter(retVal), std::bind1st(lm::minus<Scalar, T>(), lhs));
    return retVal;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::multiplies<T, Scalar>::result_type> >::type
operator* (Scalar lhs, const std::vector<T>& rhs)
{
    std::vector<typename lm::multiplies<T, Scalar>::result_type> retVal;
    std::transform(rhs.begin(), rhs.end(), std::back_inserter(retVal), std::bind1st(lm::multiplies<Scalar, T>(), lhs));
    return retVal;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::divides<T, Scalar>::result_type> >::type 
operator/ (Scalar lhs, const std::vector<T>& rhs)
{
    std::vector<typename lm::divides<T, Scalar>::result_type> retVal;
    std::transform(rhs.begin(), rhs.end(), std::back_inserter(retVal), std::bind1st(lm::divides<Scalar, T>(), lhs));
    return retVal;
}

/*
 * - vector-scalar operators
 */
template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::plus<T, Scalar>::result_type> >::type
operator+ (const std::vector<T>& lhs, Scalar rhs)
{
    std::vector<typename lm::plus<T, Scalar>::result_type> retVal;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(retVal), std::bind2nd(lm::plus<T, Scalar>(), rhs));
    return retVal;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::minus<T, Scalar>::result_type> >::type
operator- (const std::vector<T>& lhs, Scalar rhs)
{
    std::vector<typename lm::minus<T, Scalar>::result_type> retVal;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(retVal), std::bind2nd(lm::minus<T, Scalar>(), rhs));
    return retVal;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::multiplies<T, Scalar>::result_type> >::type
operator* (const std::vector<T>& lhs, Scalar rhs)
{
    std::vector<typename lm::multiplies<T, Scalar>::result_type> retVal;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(retVal), std::bind2nd(lm::multiplies<T, Scalar>(), rhs));
    return retVal;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::divides<T, Scalar>::result_type> >::type
operator/ (const std::vector<T>& lhs, Scalar rhs)
{
    std::vector<typename lm::divides<T, Scalar>::result_type> retVal;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(retVal), std::bind2nd(lm::divides<T, Scalar>(), rhs));
    return retVal;
}

/*
 * - vector-vector operators
 */
template <typename T, typename U>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<U>::value, std::vector<typename lm::plus<T, U>::result_type> >::type
operator+ (const std::vector<T>& lhs, const std::vector<U>& rhs)
{
    std::vector<typename lm::plus<T, U>::result_type> retVal;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), std::back_inserter(retVal), lm::plus<T, U>());
    return retVal;
}

template <typename T, typename U>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<U>::value, std::vector<typename lm::minus<T, U>::result_type> >::type
operator- (const std::vector<T>& lhs, const std::vector<U>& rhs)
{
    std::vector<typename lm::minus<T, U>::result_type> retVal;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), std::back_inserter(retVal), lm::minus<T, U>());
    return retVal;
}

template <typename T, typename U>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<U>::value, std::vector<typename lm::multiplies<T, U>::result_type> >::type
operator* (const std::vector<T>& lhs, const std::vector<U>& rhs)
{
    std::vector<typename lm::multiplies<T, U>::result_type> retVal;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), std::back_inserter(retVal), lm::multiplies<T, U>());
    return retVal;
}

template <typename T, typename U>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<U>::value, std::vector<typename lm::divides<T, U>::result_type> >::type
operator/ (const std::vector<T>& lhs, const std::vector<U>& rhs)
{
    std::vector<typename lm::divides<T, U>::result_type> retVal;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), std::back_inserter(retVal), lm::divides<T, U>());
    return retVal;
}

/*
 * - scalar-vector compound assignment operators
 */
template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::plus<T, Scalar>::result_type> >::type&
operator+= (Scalar lhs, std::vector<T>& rhs)
{
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), std::bind1st(lm::plus<Scalar, T>(), lhs));
    return rhs;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::minus<T, Scalar>::result_type> >::type&
operator-= (Scalar lhs, std::vector<T>& rhs)
{
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), std::bind1st(lm::minus<Scalar, T>(), lhs));
    return rhs;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::multiplies<T, Scalar>::result_type> >::type&
operator*= (Scalar lhs, std::vector<T>& rhs)
{
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), std::bind1st(lm::multiplies<Scalar, T>(), lhs));
    return rhs;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::divides<T, Scalar>::result_type> >::type&
operator/= (Scalar lhs, std::vector<T>& rhs)
{
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), std::bind1st(lm::divides<Scalar, T>(), lhs));
    return rhs;
}

/*
 * - vector-scalar compound assignment operators
 */
template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::plus<T, Scalar>::result_type> >::type&
operator+= (std::vector<T>& lhs, Scalar rhs)
{
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), std::bind2nd(lm::plus<T, Scalar>(), rhs));
    return lhs;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::minus<T, Scalar>::result_type> >::type&
operator-= (std::vector<T>& lhs, Scalar rhs)
{
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), std::bind2nd(lm::minus<T, Scalar>(), rhs));
    return lhs;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::multiplies<T, Scalar>::result_type> >::type&
operator*= (std::vector<T>& lhs, Scalar rhs)
{
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), std::bind2nd(lm::multiplies<T, Scalar>(), rhs));
    return lhs;
}

template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::divides<T, Scalar>::result_type> >::type&
operator/= (std::vector<T>& lhs, Scalar rhs)
{
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), std::bind2nd(lm::divides<T, Scalar>(), rhs));
    return lhs;
}

/*
 * - vector-vector compound assignment operators
 */
template <typename T, typename U>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<U>::value, std::vector<typename lm::plus<T, U>::result_type> >::type&
operator+= (std::vector<T>& lhs, const std::vector<U>& rhs)
{
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), lm::plus<T, U>());
    return lhs;
}

template <typename T, typename U>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<U>::value, std::vector<typename lm::minus<T, U>::result_type> >::type&
operator-= (std::vector<T>& lhs, const std::vector<U>& rhs)
{
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), lm::minus<T, U>());
    return lhs;
}

template <typename T, typename U>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<U>::value, std::vector<typename lm::multiplies<T, U>::result_type> >::type&
operator*= (std::vector<T>& lhs, const std::vector<U>& rhs)
{
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), lm::multiplies<T, U>());
    return lhs;
}

template <typename T, typename U>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<U>::value, std::vector<typename lm::divides<T, U>::result_type> >::type&
operator/= (std::vector<T>& lhs, const std::vector<U>& rhs)
{   
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), lm::divides<T, U>());
    return lhs;
}


/*
 * - this section container non-operator functions written specifically for vectors
 *     - Like the operators, these functions are written so as to optimize implementation convenience, not performance
 *     - Unless it is made explicit in the function name, the functions that return vectors are expected to allocate their own return values on their own stacks
 */

/*
 * - scalar-vector functions
 */
template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::exponentiates<T, Scalar>::result_type> >::type
pow(Scalar lhs, const std::vector<T>& rhs)
{
    std::vector<typename lm::exponentiates<T, Scalar>::result_type> retVal;
    std::transform(rhs.begin(), rhs.end(), std::back_inserter(retVal), std::bind1st(lm::exponentiates<Scalar, T>(), lhs));
    return retVal;
}

/*
 * - vector-scalar functions
 */
template <typename T, typename Scalar>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<Scalar>::value, std::vector<typename lm::exponentiates<T, Scalar>::result_type> >::type
pow(const std::vector<T>& lhs, Scalar rhs)
{
    std::vector<typename lm::exponentiates<T, Scalar>::result_type> retVal;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(retVal), std::bind2nd(lm::exponentiates<T, Scalar>(), rhs));
    return retVal;
}

/*
 * - vector-vector functions
 */
template <typename T, typename U>
inline typename EnableIf<IsNumeric<T>::value and IsNumeric<U>::value, std::vector<typename lm::exponentiates<T, U>::result_type> >::type
pow(const std::vector<T>& lhs, const std::vector<U>& rhs)
{
    std::vector<typename lm::exponentiates<T, U>::result_type> retVal;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), std::back_inserter(retVal), lm::exponentiates<T, U>());
    return retVal;
}

/*
 * - non-operator functions on single vectors
 */

/*
 * - will cause exception if vector lhs is empty
 */
template <typename T>
inline typename EnableIf<IsNumeric<T>::value, T>::type
max(const std::vector<T>& lhs)
{
    return *std::max_element(lhs.begin(), lhs.end());
};

/*
 * - will cause exception if vector lhs is empty
 */
template <typename T>
inline typename EnableIf<IsNumeric<T>::value, T>::type
min(const std::vector<T>& lhs)
{
    return *std::min_element(lhs.begin(), lhs.end());
};

template <typename T>
inline typename EnableIf<IsNumeric<T>::value, T>::type
prod(const std::vector<T>& lhs)
{
    // be careful, as this form of std::accumulate will return 1 for an empty vector
    return std::accumulate(lhs.begin(), lhs.end(), static_cast<T>(1), std::multiplies<T>());
};

template <typename T>
inline typename EnableIf<IsNumeric<T>::value, T>::type
sum(const std::vector<T>& lhs)
{
    return std::accumulate(lhs.begin(), lhs.end(), static_cast<T>(0), std::plus<T>());
};

/*
 * - This section contains functions that are more flexible versions of the vector operators/functions above
 *     - Instead of vectors, they'll take any possible range (ie a start and an end iterator)
 *     - They do not allocate a new range for the return value, so they may be more efficient than the operator versions
 */

/*
 * - scalar-range functions
 */
template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void 
add(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd, OutputIterator outputIt,
    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(rhsBegin, rhsEnd, outputIt, std::bind1st(std::plus<typename OutputIterator::value_type>(), lhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void 
add(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs, OutputIterator outputIt,
    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, outputIt, std::bind2nd(std::plus<typename OutputIterator::value_type>(), rhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void 
sub(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd, OutputIterator outputIt,
    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(rhsBegin, rhsEnd, outputIt, std::bind1st(std::minus<typename OutputIterator::value_type>(), lhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void 
sub(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs, OutputIterator outputIt,
    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, outputIt, std::bind2nd(std::minus<typename OutputIterator::value_type>(), rhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void 
mul(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd, OutputIterator outputIt,
    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(rhsBegin, rhsEnd, outputIt, std::bind1st(std::multiplies<typename OutputIterator::value_type>(), lhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void 
mul(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs, OutputIterator outputIt,
    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, outputIt, std::bind2nd(std::multiplies<typename OutputIterator::value_type>(), rhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void 
div(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd, OutputIterator outputIt,
    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(rhsBegin, rhsEnd, outputIt, std::bind1st(std::divides<typename OutputIterator::value_type>(), lhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void 
div(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs, OutputIterator outputIt,
    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, outputIt, std::bind2nd(std::divides<typename OutputIterator::value_type>(), rhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void 
pow(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd, OutputIterator outputIt,
    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(rhsBegin, rhsEnd, outputIt, std::bind1st(lm::exponentiates<typename OutputIterator::value_type>(), lhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void 
pow(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs, OutputIterator outputIt,
    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, outputIt, std::bind2nd(lm::exponentiates<typename OutputIterator::value_type>(), rhs));
}

///*
// * - scalar-range functions, 3 arg versions that store output in the input range
// */
//template <typename InputIterator, typename Scalar>
//inline void add(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd,
//    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
//{
//    InputIterator rhsOutput(rhsBegin);
//	std::transform(rhsBegin, rhsEnd, rhsOutput, std::bind1st(std::plus<typename InputIterator::value_type>(), lhs));
//}
//
//template <typename InputIterator, typename Scalar>
//inline void add(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs,
//    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
//{
//    InputIterator lhsOutput(lhsBegin);
//	std::transform(lhsBegin, lhsEnd, lhsOutput, std::bind2nd(std::plus<typename InputIterator::value_type>(), rhs));
//}
//
//template <typename InputIterator, typename Scalar>
//inline void sub(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd,
//    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
//{
//    InputIterator rhsOutput(rhsBegin);
//	std::transform(rhsBegin, rhsEnd, rhsOutput, std::bind1st(std::minus<typename InputIterator::value_type>(), lhs));
//}
//
//template <typename InputIterator, typename Scalar>
//inline void sub(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs,
//    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
//{
//    InputIterator lhsOutput(lhsBegin);
//	std::transform(lhsBegin, lhsEnd, lhsOutput, std::bind2nd(std::minus<typename InputIterator::value_type>(), rhs));
//}
//
//template <typename InputIterator, typename Scalar>
//inline void mul(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd,
//    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
//{
//    InputIterator rhsOutput(rhsBegin);
//	std::transform(rhsBegin, rhsEnd, rhsOutput, std::bind1st(std::multiplies<typename InputIterator::value_type>(), lhs));
//}
//
//template <typename InputIterator, typename Scalar>
//inline void mul(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs,
//    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
//{
//    InputIterator lhsOutput(lhsBegin);
//	std::transform(lhsBegin, lhsEnd, lhsOutput, std::bind2nd(std::multiplies<typename InputIterator::value_type>(), rhs));
//}
//
//template <typename InputIterator, typename Scalar>
//inline void div(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd,
//    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
//{
//    InputIterator rhsOutput(rhsBegin);
//	std::transform(rhsBegin, rhsEnd, rhsOutput, std::bind1st(std::divides<typename InputIterator::value_type>(), lhs));
//}
//
//template <typename InputIterator, typename Scalar>
//inline void div(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs,
//    typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
//{
//    InputIterator lhsOutput(lhsBegin);
//	std::transform(lhsBegin, lhsEnd, lhsOutput, std::bind2nd(std::divides<typename InputIterator::value_type>(), rhs));
//}

/*
 * - range-range functions
 */
template <typename InputIterator, typename OutputIterator>
inline void 
add(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
    typename EnableIf<not IsNumeric<typename IteratorTraits<InputIterator>::value_type>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::plus<typename OutputIteratorTraits<OutputIterator>::value_type>());
}

template <typename InputIterator, typename OutputIterator>
inline void 
sub(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
    typename EnableIf<not IsNumeric<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::minus<typename OutputIterator::value_type>());
}

template <typename InputIterator, typename OutputIterator>
inline void 
mul(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
    typename EnableIf<not IsNumeric<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::multiplies<typename OutputIterator::value_type>());
}

template <typename InputIterator, typename OutputIterator>
inline void 
div(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
    typename EnableIf<not IsNumeric<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::divides<typename OutputIterator::value_type>());
}

template <typename InputIterator, typename OutputIterator>
inline void 
pow(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
    typename EnableIf<not IsNumeric<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, lm::exponentiates<typename OutputIterator::value_type>());
}

//template <typename InputContainer>
//inline void cumprod(InputContainer container)
//{
//    std::partial_sum(container.begin(), container.end(), container.begin(), std::multiplies<typename InputContainer::value_type>());
//}

#endif /* LM_VECTORMATH_H_ */
