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
#include <vector>

#include "lm/Types.h"

//template <typename, template <typename> class>
//struct IsSameTemplateNumerical {static const bool value = false;};
//template <template <typename> class T, template <typename> class U, typename Param>
//struct IsSameTemplateNumerical<T<Param>, U> {static const bool value = IsSame<T<Param>, U<Param> >::value and IsNumeric<Param>::value;};
//
//template <template <typename, typename> class T, template <typename> class U, typename Param0, typename Param1>
//struct IsSameTemplateNumerical<T<Param0, Param1>, U> {static const bool value = IsSame<T<Param0, Param1>, U<Param0, Param1> >::value and IsNumeric<Param0>::value;};
//
//template<typename T> struct IsNumericContainer {static const bool value = IsSameTemplateNumerical<T, std::vector>::value;}; // IsSameTemplateNumerical<T, google::protobuf::RepeatedField>::value or

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
 * - versions of the arithmetic functors from <functional> that can do type promotion
 */
namespace lm {

template <typename T, typename U=T> struct plus
{
    typedef T first_argument_type;
    typedef U second_argument_type;
    typedef typename PrecedentType<T, U>::type result_type;
    
    result_type operator() (const T& x, const U& y) const {return x + y;}
};

template <typename T, typename U=T> struct minus
{
    typedef T first_argument_type;
    typedef U second_argument_type;
    typedef typename PrecedentType<T, U>::type result_type;

    result_type operator() (const T& x, const U& y) const {return x - y;}
};

template <typename T, typename U=T> struct multiplies
{
    typedef T first_argument_type;
    typedef U second_argument_type;
    typedef typename PrecedentType<T, U>::type result_type;

    result_type operator() (const T& x, const U& y) const {return x * y;}
};

template <typename T, typename U=T> struct divides
{
    typedef T first_argument_type;
    typedef U second_argument_type;
    typedef typename PrecedentType<T, U>::type result_type;

    result_type operator() (const T& x, const U& y) const {return x / y;}
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
 * - This section contains functions that are more flexible versions of the vector operators
 *     - Instead of vectors, they'll take any possible range (ie a start and an end iterator)
 *     - They do not allocate a new range for the return value, so they may be more efficient than the operator versions
 */

/*
 * - scalar-range functions
 */
template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void add(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd, OutputIterator outputIt,
                typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(rhsBegin, rhsEnd, outputIt, std::bind1st(std::plus<typename OutputIterator::value_type>(), lhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void add(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs, OutputIterator outputIt,
                typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, outputIt, std::bind2nd(std::plus<typename OutputIterator::value_type>(), rhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void sub(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd, OutputIterator outputIt,
                typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(rhsBegin, rhsEnd, outputIt, std::bind1st(std::minus<typename OutputIterator::value_type>(), lhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void sub(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs, OutputIterator outputIt,
                typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, outputIt, std::bind2nd(std::minus<typename OutputIterator::value_type>(), rhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void mul(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd, OutputIterator outputIt,
                typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(rhsBegin, rhsEnd, outputIt, std::bind1st(std::multiplies<typename OutputIterator::value_type>(), lhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void mul(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs, OutputIterator outputIt,
                typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, outputIt, std::bind2nd(std::multiplies<typename OutputIterator::value_type>(), rhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void div(Scalar lhs, InputIterator rhsBegin, InputIterator rhsEnd, OutputIterator outputIt,
                typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(rhsBegin, rhsEnd, outputIt, std::bind1st(std::divides<typename OutputIterator::value_type>(), lhs));
}

template <typename InputIterator, typename OutputIterator, typename Scalar>
inline void div(InputIterator lhsBegin, InputIterator lhsEnd, Scalar rhs, OutputIterator outputIt,
                typename EnableIf<IsNumeric<Scalar>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, outputIt, std::bind2nd(std::divides<typename OutputIterator::value_type>(), rhs));
}

/*
 * - range-range functions
 */
template <typename InputIterator, typename OutputIterator>
inline void add(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
                typename EnableIf<not IsNumeric<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::plus<typename OutputIterator::value_type>());
}

template <typename InputIterator, typename OutputIterator>
inline void sub(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
                typename EnableIf<not IsNumeric<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::minus<typename OutputIterator::value_type>());
}

template <typename InputIterator, typename OutputIterator>
inline void mul(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
                typename EnableIf<not IsNumeric<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::multiplies<typename OutputIterator::value_type>());
}

template <typename InputIterator, typename OutputIterator>
inline void div(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
                typename EnableIf<not IsNumeric<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::divides<typename OutputIterator::value_type>());
}

//template <typename InputContainer>
//inline void cumprod(InputContainer container)
//{
//    std::partial_sum(container.begin(), container.end(), container.begin(), std::multiplies<typename InputContainer::value_type>());
//}

#endif /* LM_VECTORMATH_H_ */
