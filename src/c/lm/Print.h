/*
 * University of Illinois Open Source License
 * Copyright 2008-2010 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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
#ifndef LM_PRINT_H_
#define LM_PRINT_H_

#include <cstdio>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <ostream>
#include <string>
#include <sstream>
#include <vector>

#include "lm/protowrap/Repeated.h"

namespace lm {

// get the first n characters of a string in a new string
std::string head(const std::string& source, size_t length);

// get the last n characters of a string in a new string (see http://stackoverflow.com/a/7597469/425458)
std::string tail(const std::string& source, size_t length);

// join a vector of path elements into a "/" delineated path.
// If absolute, ensures that there is exactly one "/" at the beginning of the path, otherwise any leading "/" are stripped
std::string pathJoin(const std::vector<std::string>& pathElements, bool absolute=true);

// convenience overloads for pathJoin
std::string pathJoin(const std::string& elem0, const std::string& elem1, bool absolute=true);// {std::vector<std::string> elems; elems.push_back(elem0); elems.push_back(elem1); return pathJoin(elems, absolute);}

// get just the final element from a path.
// example: pathName("foo/bar/re")=="re"
std::string pathName(const std::string& path);

// remove the suffix from the final element of a path, then append the newSuffix.
// example: pathWithSuffix("rey/far/foo.bar", ".rab")=="rey/far/foo.rab", pathWithSuffix("rey/far.bar/foo", ".rab")=="rey/far.bar/foo.rab"
std::string pathWithSuffix(const std::string& path, const std::string& newSuffix);

// strip any "/" from the left and right of str
std::string strip(const std::string& str);

/**
 * Class for verbosity-configurable print function.
 */
class Print
{
public:
    static const int VERBOSE_DEBUG              = 10;
    static const int DEBUG                      =  9;
    static const int INFO                       =  4;
    static const int WARNING                    =  3;
    static const int ERROR                      =  2;
    static const int FATAL                      =  1;

    static std::string getDateTimeString();
    static void printDateTimeString();
    static void printf(int verbosity, const char * fmt, ...);
    static void printMsgDebug(int verbosity, const google::protobuf::Message& msg, size_t halfMaxSize=1048576);
    template <typename T> static const char* printf_format_string();
};

}

// forward declare printIterable to allow for printing of nested vectors (via recursive template resolution)
template <class T>
inline std::ostream& printIterable(std::ostream& stream, const T& iterable);

// put printNumeric in the top-level namespace
template <typename T> static void printNumeric(T num)
{
    std::printf(lm::Print::printf_format_string<T>(), num);
}

// EZ printing of std::pairs
template <class T0, class T1>
inline std::ostream& operator << (std::ostream& stream, const std::pair<T0, T1>& p)
{
    // no std::pair::const_iterator, so can't use printIterable(...)
    stream << "[";
    stream << p.first;
    stream << ", " << p.second;
    stream << "]";

    return stream;
}

// EZ printing of std::vectors
template <class T>
inline std::ostream& operator << (std::ostream& stream, const std::vector<T>& vec)
{
    return printIterable(stream, vec);
}

// EZ printing of protobuf repeated fields
template <class T>
inline std::ostream& operator << (std::ostream& stream, const google::protobuf::RepeatedField<T>& repField)
{
    return printIterable(stream, repField);
}

// EZ printing of wrapped protobuf repeated fields
template <class T>
inline std::ostream& operator << (std::ostream& stream, const lm::protowrap::Repeated<T>& repField)
{
    return printIterable(stream, repField);
}

// generic function for printing the contents of an iterable using a std::ostream
template <class T>
inline std::ostream& printIterable(std::ostream& stream, const T& iterable)
{
    typename T::const_iterator it = iterable.begin();

    stream << "[";
    if (it!=iterable.end())
    {
        stream << *it;
        it++;
    }
    for (;it!=iterable.end();++it)
    {
        stream << ", " << *it;
    }
    stream << "]";

    return stream;
}

#endif /* LM_PRINT_H_ */
