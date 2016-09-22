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
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "lm/Iterator.h"
#include "lm/Print.h"
#include "lm/Types.h"

using std::string;
using std::stringstream;
using std::vector;

namespace lm {

std::string Print::getDateTimeString()
{
    time_t now;
    time(&now);
    struct tm nowParts;
    localtime_r(&now, &nowParts);
    std::ostringstream s;
    s << nowParts.tm_year+1900 << "-" << nowParts.tm_mon << "-" << nowParts.tm_mday << " " << nowParts.tm_hour << ":" << nowParts.tm_min << ":"  << nowParts.tm_sec;
    return s.str();
}

void Print::printDateTimeString()
{
    time_t now;
    time(&now);
    struct tm nowParts;
    localtime_r(&now, &nowParts);
    ::printf("%04d-%02d-%02d %02d:%02d:%02d) ", nowParts.tm_year+1900, nowParts.tm_mon, nowParts.tm_mday, nowParts.tm_hour, nowParts.tm_min, nowParts.tm_sec);
}

void Print::printf(int verbosity, const char * fmt, ...)
{
    if (verbosity <= VERBOSITY_LEVEL)
    {
        va_list args;
        printDateTimeString();
        if (verbosity >= DEBUG)
            ::printf("Debug: ");
        else if (verbosity == WARNING)
            ::printf("Warning: ");
        else if (verbosity == ERROR)
            ::printf("ERROR: ");
        else if (verbosity == FATAL)
            ::printf("FATAL ERRROR: ");
        va_start(args,fmt);
        vprintf(fmt,args);
        va_end(args);
        ::printf("\n");
    }
}

void Print::printMsgDebug(int verbosity, const google::protobuf::Message& msg, size_t halfMaxSize)
{
    if (msg.DebugString().size() > 2*halfMaxSize+1)
    {
        lm::Print::printf(verbosity, "%s...%s", lm::head(msg.DebugString(), halfMaxSize).c_str(), lm::tail(msg.DebugString(), halfMaxSize).c_str());
    }
    else
    {
        lm::Print::printf(verbosity, "%s", msg.DebugString().c_str());
    }
}

template<> const char* Print::printf_format_string<int>() {return "%d";}
template<> const char* Print::printf_format_string<uint>() {return "%u";}
template<> const char* Print::printf_format_string<double>() {return "%f";}

// get the first n characters of a string in a new string
string head(const string& source, size_t length)
{
    return source.substr(0, length);
}

// get the last n characters of a string in a new string (see http://stackoverflow.com/a/7597469/425458)
string tail(const string& source, size_t length)
{
    if (length>=source.size())
    {
        return source;
    }
    return source.substr(source.size() - length);
}

// "lambda" function needed for pathJoin
bool isNotSlash(const char& c) {return c!='/';}

string pathJoin(const vector<string>& pathElements, bool absolute)
{
    bool isAbsolute = false;
    stringstream ss;
    vector<string>::const_iterator it=pathElements.begin();

    // find the first non-zero length string in the vector
    while ((it!=pathElements.end()) and (it->size()==0)) it++;

    // check to see if the first non-zero length element is already an absolute path
    isAbsolute = ((*it)[0]=='/');

    for (;it!=pathElements.end()--;it++)
    {
        // see http://stackoverflow.com/a/9359324/425458
        // By ending at the right iterator, we will do the equivalent of the rstrip operation...
        string::const_iterator right = std::find_if(it->rbegin(), it->rend(), isNotSlash).base();

        // ...and by starting at the left iterator, we will do the equivalent of the lstrip operation.
        string::const_iterator left = std::find_if(it->begin(), right, isNotSlash);

        ss << string(left, right);

        if (not isLast(it, pathElements))
        {
            ss << "/";
        }
    }

    string joinedPath(ss.str());
    // the strip ops will have removed any leading "/", so if we want one add it back now
    if ((absolute or isAbsolute) and joinedPath.size() > 0 and isNotSlash(joinedPath[0])) joinedPath.insert(0, "/");

    return joinedPath;
}

string pathJoin(const string& elem0, const string& elem1, bool absolute)
{
    std::vector<std::string> elems;
    elems.push_back(elem0);
    elems.push_back(elem1);

    return pathJoin(elems, absolute);
}

}