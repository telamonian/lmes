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

#ifndef LM_IO_SFILE_SFILE_H
#define LM_IO_SFILE_SFILE_H

#include <string>
#include <google/protobuf/message.h>

#include "lm/io/sfile/SFileRecord.h"

using std::string;

namespace lm {
namespace io {
namespace sfile {

class SFile
{
public:
    SFile();
    virtual ~SFile();
    virtual bool isFile()=0;
    virtual bool isSFile();
    virtual bool isDir()=0;
    virtual bool exists()=0;
    virtual string getFilename()=0;
    virtual int64_t getSize()=0;
    virtual void openTruncate()=0;
    virtual void openAppend()=0;
    virtual void openRead()=0;
    virtual size_t read(void* buffer, size_t length)=0;
    virtual void readFully(void* buffer, size_t length);
    virtual void skip(uint64_t length)=0;
    virtual size_t write(void* buffer, size_t length)=0;
    virtual bool isEof()=0;
    virtual void flush()=0;
    virtual void close()=0;
    virtual SFileRecord readNextSFileRecord();
    virtual void writeSFileRecord(SFileRecord record);
    virtual void writeMessage(const google::protobuf::Message& message)=0;

public:
    template <typename Msg>
    bool mergeNextMessage(Msg* msg)
    {
        std::string recordTypeStr("protobuf:" + Msg::default_instance().GetDescriptor()->full_name());
        return mergeNextMessage(msg, recordTypeStr);
    }

    template <typename Msg>
    bool mergeNextMessage(Msg* msg, const string& recordType)
    {
        // get the next record
        SFileRecord r = readNextSFileRecord();

        // See if this is an input record.
        if (r.type == recordType)
        {
            // Allocate a buffer.
            char* buffer = new char[r.dataSize];

            // Read the record.
            readFully(buffer, r.dataSize);

            std::string buffString(buffer, buffer+r.dataSize);

            // Parse the record.
            Msg newMsg;
            if (!newMsg.ParsePartialFromArray(buffer, r.dataSize)) THROW_EXCEPTION(RuntimeException, "unable to deserialize record of type %s", recordType.c_str());

            // Merge this record into the global input record.
            msg->MergeFrom(newMsg);

            // Release the buffer.
            delete[] buffer;
            return true;
        }
        else
        {
            return false;
        }
    }

    template <typename MsgRepeated>
    void readAllMessages(MsgRepeated* msgRepeated)
    {
        // concrete example of the generic statement attempted bellow
        //google::protobuf::RepeatedPtrField<lm::input::SimulationInput>::value_type::default_instance().GetDescriptor()->full_name();
        typename MsgRepeated::value_type* msg(NULL);
        std::string recordTypeStr("protobuf:" + MsgRepeated::Element::default_instance().GetDescriptor()->full_name());

        // Read all of the records.
        while (isEof())
        {
            // add a new message to the repeated, if needed
            if (msg==NULL)
            {
                msg = (msgRepeated->Add());
            }

            // Read the next record.
            lm::io::sfile::SFileRecord r = readNextSFileRecord();

            if (not readNextMessage(msg, recordTypeStr))
            {
                // If the record is of the wrong type, skip it
                skip(r.dataSize);
            }
            else
            {
                // If the record was successfully parsed, NULL the msg pointer
                msg = NULL;
            }
        }
    }

    template <typename Msg>
    bool readNextMessage(Msg* msg)
    {
        std::string recordTypeStr("protobuf:" + Msg::default_instance().GetDescriptor()->full_name());
        return readNextMessage(msg, recordTypeStr);
    }

    template <typename Msg>
    bool readNextMessage(Msg* msg, const string& recordType)
    {
        // get the next record
        SFileRecord r = readNextSFileRecord();

        // See if this is an input record.
        if (r.type == recordType)
        {
            // Allocate a buffer.
            char* buffer = new char[r.dataSize];

            // Read the record.
            readFully(buffer, r.dataSize);

            std::string buffString(buffer, buffer+r.dataSize);

            // Parse the record.
            if (!msg->ParsePartialFromArray(buffer, r.dataSize)) THROW_EXCEPTION(RuntimeException, "unable to deserialize record of type %s", recordType.c_str());

            // Release the buffer.
            delete[] buffer;
            return true;
        }
        else
        {
            return false;
        }
    }

};

}
}
}

#endif
