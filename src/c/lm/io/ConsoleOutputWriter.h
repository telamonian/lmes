/*
 * Copyright 2012-2016 Johns Hopkins University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Author(s): Elijah Roberts, Max Klein
 */

#ifndef LM_IO_CONSOLEOUTPUTWRITER
#define LM_IO_CONSOLEOUTPUTWRITER

#include <queue>
#include <cstring>

#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/LatticeTimeSeries.pb.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"

namespace lm {
namespace io {

class ConsoleOutputWriter : public OutputWriter
{
public:
    static const int BUFFER_SIZE=1024*1024;

public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

public:
    ConsoleOutputWriter();
    virtual ~ConsoleOutputWriter();
    virtual void initialize();

protected:
    virtual void processDegreeAdvancementTimeSeries(const lm::io::DegreeAdvancementTimeSeries& data);
    virtual void processFirstPassageTimes(const lm::io::FirstPassageTimes& data);
    virtual void processSpeciesTimeSeries(const lm::io::SpeciesTimeSeries& data);
    virtual void processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data);
    virtual void processConcentrationsTimeSeries(const lm::io::ConcentrationsTimeSeries& data);
    virtual void flush();
    virtual void checkpoint();

private:
    char* buffer;
};

}
}


#endif
