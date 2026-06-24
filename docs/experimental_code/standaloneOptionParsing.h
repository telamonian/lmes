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
#include <locale>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

struct numpunct: std::numpunct<char>
{
    std::string do_truename() const { return "True"; }
    std::string do_falsename() const { return "False"; }
};

template<typename T0, typename T1> struct PairVector
{
    typedef std::vector<std::pair<T0, T1> > type;
    typedef typename type::iterator iterator;
    typedef typename type::const_iterator const_iterator;
};

template <typename T>
void parseNextTokenTo(std::stringstream* tokensSS, T* destination, char delimiter=':')
{
    std::string tokenString;
    std::getline(*tokensSS, tokenString, delimiter);
    std::stringstream tokenSS(tokenString);
    tokenSS >> *destination;
}

template <>
void parseNextTokenTo<bool>(std::stringstream* tokensSS, bool* destination, char delimiter)
{
    std::string tokenString;
    std::getline(*tokensSS, tokenString, delimiter);
    *destination = (tokenString=="1" or tokenString=="true" or tokenString=="True");
}

//template <>
//void parseNextTokenTo<bool>(std::stringstream* tokensSS, bool* destination, char delimiter)
//{
//    std::locale loc(std::cout.getloc(), new numpunct);
//
//    bool test0=false, test1=false, test2=false;
//
//    std::string tokenString;
//    std::getline(*tokensSS, tokenString, delimiter);
//    std::stringstream testSS0(tokenString), testSS1(tokenString), testSS2(tokenString);
//    testSS1.setf(std::ios::boolalpha);
//    testSS2.imbue(loc);
//    testSS2.setf(std::ios::boolalpha);
//
//    testSS0 >> test0;
//    testSS1 >> test1;
//    testSS2 >> test2;
//
//    *destination = (test0 or (test1 or test2));
//}

template <typename T0, typename T1>
typename PairVector<T0, T1>::type parsePairVector(const std::string& inString, bool setBoolAlpha=false)
{
    typename PairVector<T0, T1>::type parsedPairVector;

    std::stringstream pairVecSS(inString);
    std::string pairString;

    while (std::getline(pairVecSS, pairString, ','))
    {
        std::pair<T0, T1> p;
        std::stringstream pairSS(pairString);

        parseNextTokenTo(&pairSS, &p.first);
        parseNextTokenTo(&pairSS, &p.second);

        // strip any white space in between the last number parsed and the next delimiter
        // pairSS >> std::ws;
        // if (pairSS.peek() == ':')
        //     pairSS.ignore();
        // pairSS >> p.second;

        parsedPairVector.push_back(p);
    }
    std::cout << std::endl;
    return parsedPairVector;
}

#endif /* OPTIONPARSING */