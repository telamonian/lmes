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
#ifndef LM_ITERATOR_H_
#define LM_ITERATOR_H_

#include <iterator>
#include <memory>
#include <string>

#include "lm/Types.h"

//template <typename Iter>
//Iter next(Iter iter)
//{
//    return ++iter;
//}

/*
 * - function to check if an iterator points to the last element in a container
 *     - modified from http://stackoverflow.com/a/3516224/425458
 */
template <typename Iter, typename Cont>
bool isLast(Iter iter, const Cont& cont)
{
    // if the iterator points to the end, then return true only if the container is zero sized
    if (cont.end()==iter)
    {
        return (cont.size()==0);
    }
    // otherwise, check if the next iterator value brings us to the container end
    else
    {
        return (cont.end()==(++iter));
    }
}

/*
 * - iterator traits specialized by tag (output, forward, etc). more generic than the STL version
 *     - OutputIteratorTraits modified from http://stackoverflow.com/a/29084919/425458
 */
template<class T>
struct OutputIteratorTraits
:std::iterator_traits<T> {};

template< class OutputIt, class T>
struct OutputIteratorTraits<std::raw_storage_iterator<OutputIt, T> >
:std::iterator<std::output_iterator_tag, T> {};

template<class Container>
struct OutputIteratorTraits<std::back_insert_iterator<Container> >
:std::iterator<std::output_iterator_tag, typename Container::value_type> {};

template<class Container>
struct OutputIteratorTraits<std::front_insert_iterator<Container> >
:std::iterator<std::output_iterator_tag, typename Container::value_type> {};

template<class Container>
struct OutputIteratorTraits<std::insert_iterator<Container> >
:std::iterator<std::output_iterator_tag, typename Container::value_type> {};

#if __cplusplus > 199711L

template <class T, class charT = char, class traits = std::char_traits<charT> >
struct OutputIteratorTraits<std::ostream_iterator<T, charT, traits> >
:std::iterator<std::output_iterator_tag, T> {};

template <class charT, class traits = std::char_traits<charT> >
struct OutputIteratorTraits<std::ostreambuf_iterator<charT, traits> >
:std::iterator<std::output_iterator_tag, charT> {};

#endif

/*
 * - simplified version of transform_iterator from boost
 */
/*
#include <boost/iterator/iterator_adaptor.hpp>

template <class UnaryFunction, class Iterator, class Reference = boost::use_default, class Value = boost::use_default>
class transform_iterator;

namespace internal
{
// Compute the iterator_adaptor instantiation to be used for transform_iterator
template <class UnaryFunc, class Iterator, class Reference, class Value>
struct transform_iterator_base
{
private:

    typedef typename boost::detail::ia_dflt_help<Reference, result_of<const UnaryFunc(typename std::iterator_traits<Iterator>::reference)> >::type reference;
    typedef typename boost::detail::ia_dflt_help<Value, remove_reference<reference> >::type cv_value_type;

public:
    typedef boost::iterator_adaptor<
    transform_iterator<UnaryFunc, Iterator, Reference, Value>, Iterator, cv_value_type,
    boost::use_default,    // Leave the traversal category alone
    reference> type;
};
}

template <class UnaryFunc, class Iterator, class Reference, class Value>
class transform_iterator : public internal::transform_iterator_base<UnaryFunc, Iterator, Reference, Value>::type
{
    typedef typename internal::transform_iterator_base<UnaryFunc, Iterator, Reference, Value>::type super_t;

public:
    transform_iterator() { }

    transform_iterator(Iterator const& x, UnaryFunc f)
    :super_t(x), m_f(f)
    {
    }

    explicit transform_iterator(Iterator const& x)
    :super_t(x)
    {
    }

    template <class OtherUnaryFunction, class OtherIterator, class OtherReference, class OtherValue>
    transform_iterator(transform_iterator<OtherUnaryFunction, OtherIterator, OtherReference, OtherValue> const& t,
                       typename EnableIfConvertible<OtherIterator, Iterator>::type* = 0,
                       typename EnableIfConvertible<OtherUnaryFunction, UnaryFunc>::type* = 0)
    :super_t(t.base()), m_f(t.functor())
    {
    }

    UnaryFunc functor() const { return m_f; }

private:
    typename super_t::reference dereference() const { return m_f(*this->base()); }

    UnaryFunc m_f;
};

template <class UnaryFunc, class Iterator>
inline transform_iterator<UnaryFunc, Iterator> make_transform_iterator(Iterator it, UnaryFunc fun)
{
    return transform_iterator<UnaryFunc, Iterator>(it, fun);
}

template <class Return, class Argument, class Iterator>
inline transform_iterator< Return (*)(Argument), Iterator, Return>
make_transform_iterator(Iterator it, Return (*fun)(Argument))
{
    return transform_iterator<Return (*)(Argument), Iterator, Return>(it, fun);
}
*/

#endif /* LM_ITERATOR_H_ */