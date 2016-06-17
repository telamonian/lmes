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
#ifndef LM_PWRAP_REPEATED
#define LM_PWRAP_REPEATED

#include <google/protobuf/repeated_field.h>
#include <numeric>
#include <sstream>
#include <string>

#include "lm/Math.h"
#include "lm/Types.h"

namespace lm {
namespace protowrap {

// main template for type generator struct that will return google::protobuf::RepeatedField<T> for a numeric T and google::protobuf::RepeatedPtrField<T> otherwise
template <typename ValT, bool> struct _RepeatedTypedef;
template <typename ValT> struct _RepeatedTypedef<ValT, false> {typedef google::protobuf::RepeatedPtrField<ValT> RepT;};
template <typename ValT> struct _RepeatedTypedef<ValT, true> {typedef google::protobuf::RepeatedField<ValT> RepT;};
template <typename ValT> struct RepeatedTypedef {typedef typename _RepeatedTypedef<ValT, IsNumeric<ValT>::value>::RepT RepT;};

template <typename ValT>
class Repeated
{
public:
// typedefs
    typedef typename RepeatedTypedef<ValT>::RepT RepT;
    typedef typename RepT::iterator iterator;
    typedef typename RepT::const_iterator const_iterator;

// constructors/destructors
    Repeated(): repFieldPtr(NULL),repFieldConstPtr(NULL) {}
    Repeated(RepT* repFieldPtr): repFieldPtr(NULL),repFieldConstPtr(NULL) {setRepFieldPtr(repFieldPtr);}
    Repeated(const RepT& repFieldConstRef): repFieldPtr(NULL),repFieldConstPtr(NULL) {setRepFieldPtr(repFieldConstRef);}
    ~Repeated() {}

// operators
    const ValT& operator()(int i) const {return Get(i);}

// accessors
    inline const ValT& first() const {return Get(0);}
    inline const RepT* getRepFieldPtr() const {return repFieldConstPtr;}
    inline const ValT& last() const {return Get(lastIndex());}
    inline const int lastIndex() const {return size() - 1;}
    inline ValT product() const {return ProductFunctor<ValT>::call(begin(), end());}
    std::string repr(const char* suffix="") const
    {
        std::stringstream reprStream("(");
        for (uint i=0; i<size(); i++)
        {
            if (i > 0) reprStream << ',';
            reprStream << Get(i);
        }
        reprStream << ")" << suffix;
        return reprStream.str();
    }

// mutators
    inline Repeated<ValT>& operator<<(ValT val) {getRepFieldPtr()->Add(val); return *this;}
    inline RepT* getRepFieldPtr()
    {
        if (repFieldPtr==NULL) throw Exception("Pointer to internal repeated field (repFieldPtr) set to NULL in lm::protowrap::Repeated instance");
        return repFieldPtr;
    }
    inline void setRepFieldPtr(RepT* newRepFieldPtr)
    {
        repFieldPtr = newRepFieldPtr;
        repFieldConstPtr = newRepFieldPtr;
    }
    inline void setRepFieldPtr(const RepT& newRepFieldConstRef)
    {
        repFieldPtr = NULL;
        repFieldConstPtr = &newRepFieldConstRef;
    }

// pass throughs
// accessors
    const_iterator begin() const {return getRepFieldPtr()->begin();}
    const_iterator end() const {return getRepFieldPtr()->end();}
    bool empty() const {return getRepFieldPtr()->empty();}
    const ValT& Get(int index) const {return getRepFieldPtr()->Get(index);}
    int size() const {return getRepFieldPtr()->size();}

// mutators
    iterator begin() {return getRepFieldPtr()->begin();}
    iterator end() {return getRepFieldPtr()->end();}
    ValT* Add() {return getRepFieldPtr()->Add();}
    void Add(const ValT& value) {getRepFieldPtr()->Add(value);}
    void Clear() {getRepFieldPtr()->Clear();}
    ValT* Mutable(int index) {return getRepFieldPtr()->Mutable(index);}
    void Set(int index, const ValT& value) {getRepFieldPtr()->Set(index, value);}

protected:
    RepT* repFieldPtr;
    const RepT* repFieldConstPtr;
};

}
}

#endif /* LM_PWRAP_REPEATED */
