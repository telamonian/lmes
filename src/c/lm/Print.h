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
#include <string>
#include <vector>

namespace lm {

// join a vector of path elements into a "/" delineated path.
// If absolute, ensures that there is exactly one "/" at the beginning of the path, otherwise any leading "/" are stripped
std::string pathJoin(const std::vector<std::string>& pathElements, bool absolute=true);

// convenience overloads for pathJoin
std::string pathJoin(const std::string& elem0, const std::string& elem1, bool absolute=true);// {std::vector<std::string> elems; elems.push_back(elem0); elems.push_back(elem1); return pathJoin(elems, absolute);}

// get the first n characters of a string in a new string
std::string head(const std::string& source, size_t length)
{
    return source.substr(0, length);
}

// get the last n characters of a string in a new string (see http://stackoverflow.com/a/7597469/425458)
std::string tail(const std::string& source, size_t length)
{
    if (length>=source.size())
    {
        return source;
    }
    return source.substr(source.size() - length);
}

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

// put printNumeric in the top-level namespace
template <typename T> static void printNumeric(T num)
{
    std::printf(lm::Print::printf_format_string<T>(), num);
}

#endif /* LM_PRINT_H_ */

