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
#include <istream>
#include <iterator>
#include <list>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include "lm/input/SimulationParameters.pb.h"
#include "lm/option/SimulationParameters.h"
#include "lm/Print.h"
#include "lm/Types.h"

using std::getline;
using std::pair;

using std::stringstream;
using std::string;
using std::vector;
using std::ws;

namespace lm {
namespace option {

// accessors
SimParamMap::const_iterator SimulationParameters::findFirst(const vector<string>& keys) const
{
    SimParamMap::const_iterator findCIt;
    for (vector<string>::const_iterator keyCIt=keys.begin(); keyCIt!=keys.end(); keyCIt++) {
        findCIt = find(*keyCIt);
        if (not isEnd(findCIt)) {
            return findCIt;
        }
    }
    return findCIt;
}

SimParamMap::iterator SimulationParameters::findFirst(const vector<string>& keys)
{
    // an attempt to recycle the code from a const qualified method into a non-const version of the same method
    // return const_cast<SimParamMap::iterator>(static_cast<const SimulationParameters*>(this)->findFirst(keys));

    // unfortunately, though in general the above would work, it turns out that you can't const_cast an iterator, so we need something a bit more complex
    SimParamMap::const_iterator findCIt(static_cast<const SimulationParameters*>(this)->findFirst(keys));
#if __cplusplus > 199711L
    // http://stackoverflow.com/a/10669041/425458
    // in c++11, calling .erase() with a duplicate const_iterator (ie an empty range) returns a non-const iterator while erasing nothing
    return map.erase(findCIt, findCIt);
#else
    // unlike the c++11 solution, this one (may) run in linear time since in general it has to increment the non-const iterator one by one
    SimParamMap::iterator findIt(_map.begin());
    std::advance(findIt, std::distance<SimParamMap::const_iterator>(findIt,findCIt));
    return findIt;
#endif
}

// mutators
void SimulationParameters::bufToMap(const lm::input::SimulationParameters& inBuf, SimParamMap& outMap)
{
    for (int i=0; i<inBuf.key_size() && i<inBuf.value_size(); i++)
    {
        outMap[inBuf.key(i)] = inBuf.value(i);
    }
}

void SimulationParameters::mapToBuf(const SimParamMap& inMap, lm::input::SimulationParameters& outBuf)
{
    outBuf.Clear();
    for (SimParamMap::const_iterator it=inMap.begin(); it!=inMap.end(); it++) {
        outBuf.add_key(it->first);
        outBuf.add_value(it->second);
    }
}

bool SimulationParameters::rFB(const lm::input::SimulationParameters& inBuf) // rFB = read From Buf
{
    setBuf(inBuf);
    bufToMap();
    return true;
}

bool SimulationParameters::rFF(const lm::io::hdf5::Hdf5File& file) // rFF = read From File
{
    setMap(file.getParameters());
    mapToBuf();
    return true;
}

bool SimulationParameters::rFM(const SimParamMap& inMap) // rFM = read From Map
{
    setMap(inMap);
    mapToBuf();
    return true;
}

}
}
