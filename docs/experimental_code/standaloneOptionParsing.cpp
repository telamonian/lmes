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

// standalone version of the option parsing code
// for investigating the output of experimental parsers
#include <iostream>
#include <string>

#include "standaloneOptionParsing.h"

template <typename T0, typename T1>
void parsePairVectorAndPrint(const std::string& inString, bool setBoolAlpha=false)
{
    typename PairVector<T0, T1>::type pairs(parsePairVector<T0, T1>(inString, setBoolAlpha));
    for (typename PairVector<T0, T1>::const_iterator it=pairs.begin();it!=pairs.end();it++)
    {
        std::cout << it->first << " " << it->second << std::endl;
    }
    std::cout << std::endl;
}

int main()
{
    parsePairVectorAndPrint<std::string, double>("  bob :   13  , rodney:8,lucash:928");
    parsePairVectorAndPrint<double, double>("9:13,10.9:18,28:928.8");
    parsePairVectorAndPrint<std::string, bool>("True:True,False:False,  bob :   bob  , rodney:rodney,true:true,false:false, 0:0, 1    :1,99:99");
    parsePairVectorAndPrint<std::string, std::string>("  bob :   rob  ,rodney:lucash,false   :true   ,true   :    false");
}