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
 * Author(s): Elijah Roberts, Max Klein
 */
#ifndef REPLICATE_LM_IO_ReplicateSFILEOutputWriter
#define REPLICATE_LM_IO_ReplicateSFILEOutputWriter

#include <google/protobuf/message.h>
#include <string>

#include "lm/io/DegreeAdvancementTimeSeries.pb.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/LatticeTimeSeries.pb.h"
#include "lm/io/LimitTracking.pb.h"
#include "lm/io/OrderParameterFirstPassageTimes.pb.h"
#include "lm/io/OrderParameterTimeSeries.pb.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/sfile/SFileOutputWriter.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/sfile/SFile.h"

namespace lm {
namespace replicates {
namespace io {
namespace sfile {

class ReplicateSFileOutputWriter : public lm::io::sfile::SFileOutputWriter
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

public:
    ReplicateSFileOutputWriter(): lm::io::sfile::SFileOutputWriter() {}
    virtual ~ReplicateSFileOutputWriter() {}

protected:
//    template <typename T>
//    virtual void initName(char* buffer, const T& data)
//    {
//        memset(buffer, 0, RECORD_NAME_BUFFER_MAX_SIZE+1);
//        snprintf(buffer,RECORD_NAME_BUFFER_MAX_SIZE,"%s/Simulations/%llu/OrderParameterTimeSeries", recordNamePrefix.c_str(), data.trajectory_id());
//    }
//    virtual void initNameGeneric(char* buffer, const google::protobuf::Message& data);

    virtual void processGenericMessage(const google::protobuf::Message& data);

    virtual void processDegreeAdvancementTimeSeries(const lm::io::DegreeAdvancementTimeSeries& data);
    virtual void processFFluxOutput(const lm::io::FFluxOutput& data);
    virtual void processFirstPassageTimes(const lm::io::FirstPassageTimes& data);
    virtual void processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data);
    virtual void processLimitTracking(const lm::io::LimitTracking& data);
    virtual void processOrderParameterFirstPassageTimes(const lm::io::OrderParameterFirstPassageTimes& data);
    virtual void processOrderParameterTimeSeries(const lm::io::OrderParameterTimeSeries& data);
    virtual void processSpeciesCounts(const lm::io::SpeciesCounts& data);
    virtual void processSpeciesTimeSeries(const lm::io::SpeciesTimeSeries& data);
};

}
}
}
}

#endif /* REPLICATE_LM_IO_ReplicateSFILEOutputWriter */
