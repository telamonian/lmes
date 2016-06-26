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
#ifndef OPTIONPARSING
#define OPTIONPARSING

// standalone version of the option parsing code
// for investigating the output of experimental parsers

#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

template<typename T1, typename T2> struct pairVector
{
    typedef std::vector<std::pair<T1, T2> > type;
    typedef typename type::iterator iterator;
    typedef typename type::const_iterator const_iterator;
};

template <typename T1, typename T2>
typename pairVector<T1, T2>::type parsePairVector(const std::string& inString, const std::string& debugMessage="")
{
    typename pairVector<T1, T2>::type parsedPairVector;

    std::stringstream pairVecSS(inString);
    std::string pairString, tokenString;

    while (getline(pairVecSS, pairString, ','))
    {
        std::pair<T1, T2> p;
        std::stringstream pairSS(pairString);

        getline(pairSS, tokenString, ':');
        std::stringstream firstSS(tokenString);
        firstSS >> p.first;

        std::cout << p.first << std::endl;
        // strip any white space in between the last number parsed and the next delimiter
        // pairSS >> std::ws;
        // if (pairSS.peek() == ':')
        //     pairSS.ignore();
        // pairSS >> p.second;

        getline(pairSS, tokenString, ':');
        std::stringstream secondSS(tokenString);
        secondSS >> p.second;

        parsedPairVector.push_back(p);

//        Print::printf(Print::DEBUG, "Parsed %s %s to: %f => %f", debugMessage.c_str(), pairString.c_str(), p.first, p.second);
    }
    std::cout << std::endl;
    return parsedPairVector;
}

#endif /* OPTIONPARSING */