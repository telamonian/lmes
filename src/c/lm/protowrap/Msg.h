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
#ifndef LM_PROTOWRAP_MSG_H_
#define LM_PROTOWRAP_MSG_H_

#include "lm/protowrap/Repeated.h"

namespace lm {
namespace protowrap {

// this is a base class for the CRTP pattern, and is to be used in derived classes as so -> class derivedMsg: public Msg<derivedMsg>
template<typename DerivedMsg, typename _WrappedMsg> class Msg
{
public:
    typedef _WrappedMsg WrappedMsg;
    typedef DerivedMsg This;

    virtual ~Msg() {}

    // conversion operators allow this wrapper to be used wherever google::protobuf::Message could be
    operator WrappedMsg*() {return wrappedMsgPtr;}
    operator WrappedMsg&() const {return *wrappedMsgPtr;}

    inline void Clear() {wrappedMsgPtr->Clear();}
    inline const WrappedMsg& wrappedMsg() const {return *wrappedMsgPtr;}
    inline WrappedMsg* mutableWrappedMsg() {return wrappedMsgPtr;}

    inline void setWrappedMsg(WrappedMsg* newMsgPtr)
    {
        wrappedMsgPtr = newMsgPtr;

        static_cast<This*>(this)->_macro_setWrapped();
    }

protected:
    Msg(): wrappedMsgPtr(NULL) {}
    WrappedMsg* wrappedMsgPtr;
};

}
}

#endif //LM_PROTOWRAP_MSG_H_