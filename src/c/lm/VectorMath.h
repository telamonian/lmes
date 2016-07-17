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
#include <cmath>
#include <functional>
#include <google/protobuf/repeated_field.h>
#include <numeric>
#include <vector>

#include "lm/Types.h"

//template <typename, template <typename> class>
//struct IsSameTemplateNumerical {static const bool value = false;};
//template <template <typename> class T, template <typename> class U, typename Param>
//struct IsSameTemplateNumerical<T<Param>, U> {static const bool value = IsSame<T<Param>, U<Param> >::value and IsNumeric<Param>::value;};
//
//template <template <typename, typename> class T, template <typename> class U, typename Param>
//struct IsSameTemplateNumerical<T<Param>, U> {static const bool value = IsSame<T<Param>, U<Param> >::value and IsNumeric<Param>::value;};
//
//template<typename T> struct IsNumericContainer {static const bool value = IsSameTemplateNumerical<T, std::vector>::value;}; // IsSameTemplateNumerical<T, google::protobuf::RepeatedField>::value or

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
 * - range-range operations
 */
template <typename InputIterator, typename OutputIterator>
inline void add(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
                typename EnableIf<IsNumericIterator<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::plus<typename OutputIterator::value_type>());
}

template <typename InputIterator, typename OutputIterator>
inline void sub(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
                typename EnableIf<IsNumericIterator<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::minus<typename OutputIterator::value_type>());
}

template <typename InputIterator, typename OutputIterator>
inline void mul(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
                typename EnableIf<IsNumericIterator<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::multiplies<typename OutputIterator::value_type>());
}

template <typename InputIterator, typename OutputIterator>
inline void div(InputIterator lhsBegin, InputIterator lhsEnd, InputIterator rhsBegin, OutputIterator outputIt,
                typename EnableIf<IsNumericIterator<InputIterator>::value>::type* = 0)
{
    std::transform(lhsBegin, lhsEnd, rhsBegin, outputIt, std::divides<typename OutputIterator::value_type>());
}


/*
 * container-container operators
 *//*
template <typename InputContainer>
inline InputContainer operator+(const InputContainer& lhs, const InputContainer& rhs,
                                typename EnableIf<IsNumericContainer<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), std::back_inserter(retVal), std::plus<typename InputContainer::value_type>());
    return retVal;
}

template <typename InputContainer>
inline InputContainer operator-(const InputContainer& lhs, const InputContainer& rhs,
                                typename EnableIf<IsNumericContainer<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), std::back_inserter(retVal), std::minus<typename InputContainer::value_type>());
    return retVal;
}

template <typename InputContainer>
inline InputContainer operator*(const InputContainer& lhs, const InputContainer& rhs,
                                typename EnableIf<IsNumericContainer<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), std::back_inserter(retVal), std::multiplies<typename InputContainer::value_type>());
    return retVal;
}

template <typename InputContainer>
inline InputContainer operator/(const InputContainer& lhs, const InputContainer& rhs,
                                typename EnableIf<IsNumericContainer<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), std::back_inserter(retVal), std::divides<typename InputContainer::value_type>());
    return retVal;
}
*/

/*
 * scalar-container operators
 *//*
template <typename InputContainer, typename Scalar>
inline InputContainer operator+(Scalar lhs, const InputContainer& rhs,
                                typename EnableIf<IsNumeric<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(rhs.begin(), rhs.end(), std::back_inserter(retVal), std::bind1st(std::plus<typename InputContainer::value_type>(), lhs));
    return retVal;
}

template <typename InputContainer, typename Scalar>
inline InputContainer operator+(const InputContainer& lhs, Scalar rhs,
                                typename EnableIf<IsNumeric<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(retVal), std::bind2nd(std::plus<typename InputContainer::value_type>(), rhs));
    return retVal;
}

template <typename InputContainer, typename Scalar>
inline InputContainer operator-(Scalar lhs, const InputContainer& rhs,
                                typename EnableIf<IsNumeric<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(rhs.begin(), rhs.end(), std::back_inserter(retVal), std::bind1st(std::minus<typename InputContainer::value_type>(), lhs));
    return retVal;
}

template <typename InputContainer, typename Scalar>
inline InputContainer operator-(const InputContainer& lhs, Scalar rhs,
                                typename EnableIf<IsNumeric<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(retVal), std::bind2nd(std::minus<typename InputContainer::value_type>(), rhs));
    return retVal;
}

template <typename InputContainer, typename Scalar>
inline InputContainer operator*(Scalar lhs, const InputContainer& rhs,
                                typename EnableIf<IsNumeric<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(rhs.begin(), rhs.end(), std::back_inserter(retVal), std::bind1st(std::multiplies<typename InputContainer::value_type>(), lhs));
    return retVal;
}

template <typename InputContainer, typename Scalar>
inline InputContainer operator*(const InputContainer& lhs, Scalar rhs,
                                typename EnableIf<IsNumeric<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(retVal), std::bind2nd(std::multiplies<typename InputContainer::value_type>(), rhs));
    return retVal;
}

template <typename InputContainer, typename Scalar>
inline InputContainer operator/(Scalar lhs, const InputContainer& rhs,
                                typename EnableIf<IsNumeric<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(rhs.begin(), rhs.end(), std::back_inserter(retVal), std::bind1st(std::divides<typename InputContainer::value_type>(), lhs));
    return retVal;
}

template <typename InputContainer, typename Scalar>
inline InputContainer operator/(const InputContainer& lhs, Scalar rhs,
                                typename EnableIf<IsNumeric<InputContainer>::value>::type* = 0)
{
    InputContainer retVal;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(retVal), std::bind2nd(std::divides<typename InputContainer::value_type>(), rhs));
    return retVal;
}*/

/*
 * - container-container functions
 *//*
template <typename InputContainer, typename OutputContainer>
inline void add(const InputContainer& lhs, const InputContainer& rhs, OutputContainer* outputIt,
                typename EnableIf<IsNumericContainer<InputContainer>::value>::type* = 0)
{
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), outputIt, std::plus<typename OutputContainer::value_type>());
}

template <typename InputContainer, typename OutputContainer>
inline void sub(const InputContainer& lhs, const InputContainer& rhs, OutputContainer* outputIt,
                typename EnableIf<IsNumericContainer<InputContainer>::value>::type* = 0)
{
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), outputIt, std::minus<typename OutputContainer::value_type>());
}

template <typename InputContainer, typename OutputContainer>
inline void mul(const InputContainer& lhs, const InputContainer& rhs, OutputContainer* outputIt,
                typename EnableIf<IsNumericContainer<InputContainer>::value>::type* = 0)
{
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), outputIt, std::multiplies<typename OutputContainer::value_type>());
}

template <typename InputContainer, typename OutputContainer>
inline void div(const InputContainer& lhs, const InputContainer& rhs, OutputContainer* outputIt,
                typename EnableIf<IsNumericContainer<InputContainer>::value>::type* = 0)
{
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), outputIt, std::divides<typename OutputContainer::value_type>());
}
*/

#endif /* LM_VECTORMATH_H_ */
