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
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include "lm/input/SimulationParameters.pb.h"
#include "lm/input/SimulationParametersWrap.h"
#include "lm/Print.h"
#include "lm/Types.h"

using std::getline;
using std::pair;
using std::setw;
using std::stringstream;
using std::string;
using std::vector;
using std::ws;

namespace lm {
namespace input {

// accessors
bool SimulationParametersWrap::checkAllParsed() const
{
    return _mapUnparsed.size() <= 0;
}

SimulationParametersWrap::const_iterator SimulationParametersWrap::findFirst(const vector<string>& keys) const
{
    const_iterator findCIt;
    for (vector<string>::const_iterator keyCIt=keys.begin(); keyCIt!=keys.end(); keyCIt++) {
        findCIt = find(*keyCIt);
        if (not isEnd(findCIt)) {
            return findCIt;
        }
    }
    return findCIt;
}

SimulationParametersWrap::iterator SimulationParametersWrap::findFirst(const vector<string>& keys)
{
    // an attempt to recycle the code from a const qualified method into a non-const version of the same method
    // return const_cast<iterator>(static_cast<const SimulationParametersWrap*>(this)->findFirst(keys));

    // unfortunately, though in general the above would work, it turns out that you can't const_cast an iterator, so we need something a bit more complex
    const_iterator findCIt(static_cast<const SimulationParametersWrap*>(this)->findFirst(keys));
#if __cplusplus > 199711L
    // http://stackoverflow.com/a/10669041/425458
    // in c++11, calling .erase() with a duplicate const_iterator (ie an empty range) returns a non-const iterator while erasing nothing
    return map.erase(findCIt, findCIt);
#else
    // unlike the c++11 solution, this one (may) run in linear time since in general it has to increment the non-const iterator one by one
    iterator findIt(_map.begin());
    std::advance(findIt, std::distance<const_iterator>(findIt,findCIt));
    return findIt;
#endif
}

void SimulationParametersWrap::printParsed() const
{
    stringstream outputSS;
    outputSS << "Parsed the following user-defined simulation parameters:\n";
    outputSS << setw(20) << "KEY" << " " << setw(20) << "VALUE" << "\n";


    for (const_iterator it=_mapParsed.begin(); it!=_mapParsed.end(); it++)
    {
        outputSS << setw(20) << it->first << " " << setw(20) << it->second << "\n";
    }

    Print::printf(Print::INFO, outputSS.str().c_str());
}

void SimulationParametersWrap::printUnparsed() const
{
    stringstream outputSS;
    outputSS << "Invalid parameters. The following simulation parameters in the input file were not recognized during parsing:\n";
    outputSS << setw(20) << "KEY" << " " << setw(20) << "VALUE" << "\n";


    for (const_iterator it=_mapUnparsed.begin(); it!=_mapUnparsed.end(); it++)
    {
        outputSS << setw(20) << it->first << " " << setw(20) << it->second << "\n";
    }

    // There were user parameters we couldn't parse, kill the simulation
    THROW_EXCEPTION(InputException, outputSS.str().c_str());
}

// mutators
void SimulationParametersWrap::bufToMap(const lm::input::SimulationParameters& inBuf, parammap& outMap)
{
    for (int i=0; i<inBuf.key_size() && i<inBuf.value_size(); i++)
    {
        outMap[inBuf.key(i)] = inBuf.value(i);
    }
}

void SimulationParametersWrap::mapToBuf(const parammap& inMap, lm::input::SimulationParameters& outBuf)
{
    outBuf.Clear();
    for (const_iterator it=inMap.begin(); it!=inMap.end(); it++) {
        outBuf.add_key(it->first);
        outBuf.add_value(it->second);
    }
}

bool SimulationParametersWrap::rFB(const lm::input::SimulationParameters& inBuf, bool setupUnparsed) // rFB = read From Buf
{
    // set up the protocol buffer
    setBuf(inBuf);

    // set up the stl map
    bufToMap();

    if (setupUnparsed)
    {
        // set up the map that keeps track of unparsed entries
        if (setupUnparsed) initMapUnparsed();
    }

    return true;
}

bool SimulationParametersWrap::rFF(const lm::io::hdf5::Hdf5File& file, bool setupUnparsed) // rFF = read From File
{
    // set up the stl map
    setMap(file.getParameters());

    // set up the protocol buffer
    mapToBuf();

    // set up the map that keeps track of unparsed entries
    if (setupUnparsed) initMapUnparsed();

    return true;
}

bool SimulationParametersWrap::rFM(const parammap& inMap, bool setupUnparsed) // rFM = read From Map
{
    // set up the stl map
    setMap(inMap);

    // set up the protocol buffer
    mapToBuf();

    // set up the map that keeps track of unparsed entries
    if (setupUnparsed) initMapUnparsed();

    return true;
}

}
}
