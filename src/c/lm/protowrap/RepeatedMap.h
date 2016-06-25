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
template <typename ValT, typename KeyT, KeyT (*getKeyFunc)(const ValT&), void (*setKeyFunc)(ValT*, const KeyT&)>
class RepeatedMap: public Repeated<ValT>
{
public:
// typedefs
    typedef typename Repeated<ValT>::GoogleT RepT;
    typedef std::map<KeyT, ValT*> MapT;
    typedef std::map<KeyT, const ValT*> MapConstT;

// constructors/destructors
    RepeatedMap() {}
    RepeatedMap(RepT* repFieldPtr): Repeated<ValT>(repFieldPtr) {}
    RepeatedMap(const RepT& repFieldConstRef): Repeated<ValT>(repFieldConstRef) {}
    virtual ~RepeatedMap() {}

// operators
    ValT* operator[](const KeyT& key) const {return map[key];}

// mutators
    void addMemberValPtrToMap(ValT* memberValPtr) {map[(*getKeyFunc)(memberValPtr)] = memberValPtr;}

    inline virtual void setRepFieldPtr(RepT* newRepFieldPtr)
    {
        // call the base class method
        Repeated<ValT>::setRepFieldPtr(newRepFieldPtr);

        map.clear();
        for (typename Repeated<ValT>::iterator it=Repeated<ValT>::begin();it!=Repeated<ValT>::end();it++)
        {
            addMemberValPtrToMap(&*it);
        }
    }
    inline virtual void setRepFieldPtr(const RepT& newRepFieldConstRef)
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

    ValT* Add(const KeyT& newKey)
    {
        ValT* newVal=Repeated<ValT>::getRepFieldPtr()->Add();
        (*setKeyFunc)(newVal, newKey);
        addMemberValPtrToMap(newVal);

        return newVal;
    }
    void Clear() {Repeated<ValT>::getRepFieldPtr()->Clear(); map.clear();}

protected:
    MapT map;
    MapConstT mapConst;
};



}
}

#endif /* LM_PWRAP_REPEATEDMAP */
