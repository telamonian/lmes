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
#ifndef LM_OPTION_SIMULATIONPARAMETERS
#define LM_OPTION_SIMULATIONPARAMETERS

#include <list>
#include <map>
#include <string>
#include <vector>

#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/SimulationParameters.pb.h"

namespace lm {
namespace option {

typedef std::map<std::string,std::string> SimParamMap;

class SimulationParameters
{
public:
    SimulationParameters() {}
    SimulationParameters(const lm::io::SimulationParameters& newBuf) {rFB(newBuf);}
    SimulationParameters(lm::io::hdf5::Hdf5File* file) {rFF(file);}
    SimulationParameters(SimParamMap& newMap) {rFM(newMap);}
    ~SimulationParameters() {}

// accessors
    SimParamMap::const_iterator findFirst(const std::vector<std::string>& keys) const;
    SimParamMap::iterator findFirst(const std::vector<std::string>& keys);
    lm::io::SimulationParameters* getBuf() {return &buf;}
    SimParamMap* getMap() {return &map;}
    const SimParamMap* getMapConst() const {return &map;}
    bool isEnd(SimParamMap::const_iterator it) const {return it==map.end();}
    std::list<int> parseIntList(const std::string& key) const;

// mutators
    // for the buf <-> map conversion methods, if you drop an arg it'll use the internal map and/or buf
    void bufToMap() {bufToMap(buf, map);}
    void bufToMap(const lm::io::SimulationParameters& inBuf) {bufToMap(inBuf, map);}
    void bufToMap(SimParamMap& outMap) {bufToMap(buf, outMap);}
    void bufToMap(const lm::io::SimulationParameters& inBuf, SimParamMap& outMap);

    void mapToBuf() {mapToBuf(map, buf);}
    void mapToBuf(SimParamMap& inMap) {mapToBuf(inMap, buf);}
    void mapToBuf(lm::io::SimulationParameters& outBuf) {mapToBuf(map, outBuf);}
    void mapToBuf(SimParamMap& inMap, lm::io::SimulationParameters& outBuf);

    bool rFB(const lm::io::SimulationParameters& inBuf); // rFB = read From Buf
    bool rFF(const lm::io::hdf5::Hdf5File& file); // rFF = read From File
    bool rFM(const SimParamMap& inMap); // rFM = read From Map

    void setBuf(const lm::io::SimulationParameters& newBuf) {*getBuf() = newBuf;}
    void setMap(const SimParamMap& newMap) {*getMap() = newMap;}

// pass-throughs to the underlying SimulationParameters buf and SimParamMap
    std::string& operator[](const std::string& key) {return map[key];}

    SimParamMap::size_type count(const std::string& key) const {return map.count(key);}
    SimParamMap::iterator find(const std::string& key) {return map.find(key);}

    SimParamMap::iterator begin() {return map.begin();}
    SimParamMap::iterator end() {return map.end();}

// serialization interface
    void deserialize(const lm::io::SimulationParameters& inBuf) {rFB(inBuf);}
    lm::io::SimulationParameters* serialize() {return getBuf();}

private:
    lm::io::SimulationParameters buf;
    SimParamMap map;
};

}
}

#endif /* LM_OPTION_SIMULATIONPARAMETERS */
