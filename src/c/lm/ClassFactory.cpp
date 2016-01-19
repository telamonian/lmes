/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
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
 * Author(s): Elijah Roberts
 */

#include <map>
#include <string>

#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/Print.h"

using std::map;
using std::string;

namespace lm {

ClassFactory& ClassFactory::getInstance()
{
    static ClassFactory instance;
    return instance;
}

void ClassFactory::registerClass(string baseClassName, string className, ClassAllocator allocator)
{
    knownClasses[baseClassName][className] = allocator;
}

void* ClassFactory::allocateObjectOfClass(string baseClassName, string className)
{
    if (knownClasses.count(baseClassName) == 1)
    {
        map<string,ClassAllocator> knownSubclasses = knownClasses[baseClassName];
        if (knownSubclasses.count(className) == 1)
        {
            ClassAllocator allocator = knownSubclasses[className];
            return allocator();
        }
    }
    throw Exception("No allocator found for baseclass/class", baseClassName.c_str(), className.c_str());
}

void ClassFactory::printRegisteredClasses()
{
    Print::printf(Print::DEBUG, "The following dynamic classes were registered during initialization:");
    for (map<string,map<string,ClassAllocator> >::iterator it=knownClasses.begin(); it != knownClasses.end(); it++)
    {
        string baseClassName = it->first.c_str();
        map<string,ClassAllocator> knownSubclasses = it->second;
        for (map<string,ClassAllocator>::iterator it2=knownSubclasses.begin(); it2 != knownSubclasses.end(); it2++)
        {
            Print::printf(Print::DEBUG, "%s -> %s", baseClassName.c_str(), it2->first.c_str());
        }
    }
}

}
