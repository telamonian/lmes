/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
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
 * Author(s): Elijah Roberts, Max Klein
 */
#ifndef LM_PWRAP_REPEATEDMAP
#define LM_PWRAP_REPEATEDMAP

#include <google/protobuf/repeated_field.h>
#include <map>
#include <numeric>
#include <sstream>
#include <string>

#include "lm/Math.h"
#include "lm/protowrap/Repeated.h"
#include "lm/Types.h"

namespace lm {
namespace protowrap {

// version of Repeated<complex_type> with operator[] that allows for accessing entries based on a map to an attribute of complex_type
// TODO: iron out the const version of this class
template <typename Element, typename Key, Key (*getKeyFunc)(const Element&), void (*setKeyFunc)(Element*, const Key&)>
class RepeatedMap: public Repeated<Element>
{
public:
// typedefs
    typedef typename Repeated<Element>::RepeatedField RepeatedField;
    typedef std::map<Key, Element*> ElementPtrMap;
    typedef std::map<Key, const Element*> ElementConstPtrMap;

// constructors/destructors
    RepeatedMap() {}
    RepeatedMap(RepeatedField* repFieldPtr): Repeated<Element>(repFieldPtr) {}
    RepeatedMap(const RepeatedField& repFieldConstRef): Repeated<Element>(repFieldConstRef) {}
    virtual ~RepeatedMap() {}

// operators
    Element* operator[](const Key& key) const {return map[key];}

// mutators
    void addMemberValPtrToMap(Element* memberValPtr) {map[(*getKeyFunc)(memberValPtr)] = memberValPtr;}

    inline virtual void setRepFieldPtr(RepeatedField* newRepFieldPtr)
    {
        // call the base class method
        Repeated<Element>::setRepFieldPtr(newRepFieldPtr);

        map.clear();
        for (typename Repeated<Element>::iterator it=Repeated<Element>::begin();it!=Repeated<Element>::end();it++)
        {
            addMemberValPtrToMap(&*it);
        }
    }
    inline virtual void setRepFieldPtr(const RepeatedField& newRepFieldConstRef)
    {
        throw UnimplementedException("Const version of RepeatedMap not yet implemented");
//        // call the base class method
//        Repeated::setRepFieldPtr(newRepFieldConstRef);
//
//        map.clear();
//        for (const_iterator it=begin();it!=end();it++)
//        {
//            addMemberValPtrToMap()
//        }
    }

    Element* Add(const Key& newKey)
    {
        Element* newVal=Repeated<Element>::getRepFieldPtr()->Add();
        (*setKeyFunc)(newVal, newKey);
        addMemberValPtrToMap(newVal);

        return newVal;
    }
    void Clear() {Repeated<Element>::getRepFieldPtr()->Clear(); map.clear();}

protected:
    ElementPtrMap map;
    ElementConstPtrMap mapConst;
};



}
}

#endif /* LM_PWRAP_REPEATEDMAP */
