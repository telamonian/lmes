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
#include <string>
#include <vector>

#include "lm/io/SimulationParameters.pb.h"
#include "lm/option/SimulationParameters.h"

using std::string;
using std::vector;

namespace lm {
namespace option {

const char notFoundCStr[] = {1,2,3,4,5,6,7,8,9,10,11,12};    //{'S','F','R','X',1,65,243,72,36,217,55,18,134,11,234,83};
const string SimulationParameters::notFound(notFoundCStr);  // = "\1\2\3\4\5\6\7\8\9\10\11\12";

// accessors
SimParamMap::iterator SimulationParameters::findFirst(vector<string>& keys)
{
    SimParamMap::iterator findIt;
    for (vector<string>::iterator keyIt=keys.begin(); keyIt!=keys.end(); keyIt++) {
        findIt = find(*keyIt);
        if (not isEnd(findIt)) {
            return findIt;
        }
    }
    return findIt;
}

// mutators
void SimulationParameters::bufToMap(const lm::io::SimulationParameters& inBuf, SimParamMap& outMap)
{
    for (int i=0; i<inBuf.key_size() && i<inBuf.value_size(); i++)
    {
        outMap[inBuf.key(i)] = inBuf.value(i);
    }
}

void SimulationParameters::mapToBuf(SimParamMap& inMap, lm::io::SimulationParameters& outBuf)
{
    outBuf.Clear();
    for (SimParamMap::iterator it=inMap.begin(); it!=inMap.end(); it++) {
        outBuf.add_key(it->first);
        outBuf.add_value(it->second);
    }
}

bool SimulationParameters::rFB(const lm::io::SimulationParameters& inBuf) // rFB = read From Buf
{
    setBuf(inBuf);
    bufToMap();
    return true;
}

bool SimulationParameters::rFF(lm::io::hdf5::Hdf5File* file) // rFF = read From File
{
    setMap(file->getParameters());
    mapToBuf();
    return true;
}

bool SimulationParameters::rFM(SimParamMap& inMap) // rFM = read From Map
{
    setMap(inMap);
    mapToBuf();
    return true;
}

}
}
