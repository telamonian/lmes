/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
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
 * Author(s): Elijah Roberts
 */

#ifndef LM_MAIN_DATAOUTPUTQUEUE
#define LM_MAIN_DATAOUTPUTQUEUE

#include <queue>
#include <google/protobuf/message.h>
#include "lm/Exceptions.h"
#include "lm/Types.h"
#include "lm/thread/Thread.h"

using std::queue;
using lm::thread::PthreadException;



namespace lm {
namespace main {

class DataOutputQueue
{
protected:
    class DataSet
    {
    public:
        DataSet(size_t size):size(size) {data=new byte[size];}
        DataSet(void * origData, size_t size):size(size) {data=new byte[size]; memcpy(data, origData, size);}
        virtual ~DataSet() {delete [] data; data=NULL;}
        byte * data;
        size_t size;
    };

public:
    static const uint SPECIES_COUNTS        =   10;
    static const uint FIRST_PASSAGE_TIMES   =   11;
    static const uint PARAMETER_VALUES      =   12;
    static const uint BYTE_LATTICE          =   20;

public:
    static void setInstance(DataOutputQueue * instance);
    static DataOutputQueue * getInstance();

private:
    static DataOutputQueue * instance;

public:
    DataOutputQueue() throw(PthreadException);
    virtual ~DataOutputQueue()  throw(PthreadException);

    virtual void pushDataSet(uint type, uint replicate, ::google::protobuf::Message * message, void * payload=NULL, size_t payloadSize=0, void (*payloadSerializer)(void *, void *, size_t)=NULL) throw(PthreadException);
    virtual void pushDataSet(void * data, size_t dataSize) throw(PthreadException);
    virtual void pushDataSet(DataSet * dataSet) throw(PthreadException);

protected:
    pthread_mutex_t dataMutex;
    queue<DataSet *> dataQueue;

};

}
}


#endif
