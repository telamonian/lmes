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

namespace lm {
namespace pwrap {

template <typename T>
class Repeated
{
public:
    Repeated(): bufField(NULL) {}
    Repeated(google::protobuf::RepeatedField<T> * bufField): bufField(bufField) {}
    ~Repeated() {}

// pass throughs
// accessors
    const T& Get(int index) const {return bufField->Get(index);}
    T* Mutable(int index) {return bufField->Mutable(index);}

// mutators
    T* Add() {return bufField->Add();}
    void Add(const T& value) {bufField->Add(value);}
    void Set(int index, const T& value) {bufField->Set(index, value);}

// wrapper functions
// accessors

// mutators
    inline Repeated<T>& operator<<(T val) {bufField->Add(val); return &this;}
//    inline Repeated<T>& operator<<(Repeated<T>& rep, T val) {rep.bufField->Add(val); return rep;}
    inline void setBufField(google::protobuf::RepeatedField<T>* newBufField) {bufField=newBufField;}

public:
    google::protobuf::RepeatedField<T>* bufField;
};

}
}

#endif /* LM_PWRAP_REPEATED */
