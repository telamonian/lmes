/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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

#include <list>
#include <map>
#include <string>

#include <dlfcn.h>


#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/Print.h"

using std::list;
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

void ClassFactory::registerClassesFromExternalLibrary(string filename)
{
    if (loadedExternalLibraries.count(filename) == 0)
    {
        void* libraryHandle;
        if ((libraryHandle=dlopen(filename.c_str(), RTLD_NOW)) == NULL)
            throw Exception("Failed to load shared library",filename.c_str(), dlerror());

        void* symbolHandle;
        if ((symbolHandle=dlsym(libraryHandle, "registerClasses")) == NULL)
        {
            dlclose(libraryHandle);
            throw Exception("Failed to find registerClasses symbol in shared library",filename.c_str(), dlerror());
        }

        // Save the library handle from this library.
        loadedExternalLibraries[filename] = libraryHandle;

        // Get the class defintions from the library.
        ExternalClassDefinitions definitions;
        definitions.numberClasses = 0;
        definitions.baseClassNames = NULL;
        definitions.classNames = NULL;
        definitions.allocators = NULL;
        ExternalLibraryRegisterClasses f = (ExternalLibraryRegisterClasses)symbolHandle;
        (*f)(&definitions);

        if (definitions.numberClasses == 0)
        {
            lm::Print::printf(lm::Print::INFO, "No classes located in shared library %s", filename.c_str());
            return;
        }

        // Make sure we have pointers.
        if (definitions.baseClassNames == NULL || definitions.classNames == NULL || definitions.allocators == NULL)
            throw Exception("Invalid pointers in class defintions from external library",filename.c_str());

        // Register the classes.
        for (int i=0; i<definitions.numberClasses; i++)
        {
            if (definitions.baseClassNames[i] == NULL || definitions.classNames[i] == NULL || definitions.allocators[i] == NULL)
                throw Exception("Invalid pointers in class defintion from external library",filename.c_str(),i);
            registerClass(definitions.baseClassNames[i], definitions.classNames[i], definitions.allocators[i]);
        }

        // Free the space used by the definitions.
        delete[] definitions.baseClassNames;
        delete[] definitions.classNames;
        delete[] definitions.allocators;

        lm::Print::printf(lm::Print::INFO, "Successfully loaded %d classes from shared library %s",definitions.numberClasses, filename.c_str());
    }
}

string ClassFactory::getBaseClass(string className)
{
    for (map<string,map<string,ClassAllocator> >::iterator it=knownClasses.begin(); it != knownClasses.end(); it++)
    {
        string baseClassName = it->first.c_str();
        map<string,ClassAllocator> knownSubclasses = it->second;
        for (map<string,ClassAllocator>::iterator it2=knownSubclasses.begin(); it2 != knownSubclasses.end(); it2++)
        {
            if (it2->first.c_str() == className)
                return baseClassName;
        }
    }
    throw Exception("No allocator found for class", className.c_str());
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

list<string> ClassFactory::getAllSubclasses(string baseClassName)
{
    list<string> subclasses;
    if (knownClasses.count(baseClassName) == 1)
    {
        map<string,ClassAllocator> knownSubclasses = knownClasses[baseClassName];
        for (map<string,ClassAllocator>::iterator it=knownSubclasses.begin(); it != knownSubclasses.end(); it++)
        {
            subclasses.push_back(it->first.c_str());
        }
    }
    return subclasses;
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
