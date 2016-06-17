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
#ifndef LM_INPUT_SIMULATIONPARAMETERS
#define LM_INPUT_SIMULATIONPARAMETERS

#include <map>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <utility>
#include <vector>

#include "lm/io/SimulationParameters.pb.h"
#include "lm/Print.h"
#include "lm/Types.h"

namespace lm {
namespace input {

typedef std::map<std::string,std::string> SimParamMap;

class SimulationParameters
{
public:
    SimulationParameters() {}
    ~SimulationParameters() {}

// accessors
    SimParamMap::const_iterator findFirst(const std::vector<std::string>& keys) const;
    const io::SimulationParameters& buf() {return _buf;}
    const SimParamMap& map() const {return _map;}
    bool isEnd(SimParamMap::const_iterator it) const {return it==_map.end();}

    template <typename T>
    T parse(const std::string &key) const
    {
        T retVal;
        std::stringstream ss(_map.at(key));

        ss >> retVal;
        return retVal;
    }

    template <typename T1, typename T2>
    typename pairVector<T1, T2>::type parsePairVector(const std::string &key, const std::string& debugMessage="") const
    {
        std::stringstream pairVecSS(_map.at(key));

        typename pairVector<T1, T2>::type parsedPairVector;
        std::string pairString;
        while (getline(pairVecSS, pairString, ','))
        {
            std::pair<T1, T2> p;
            std::stringstream pairSS(pairString);

            pairSS >> p.first;
            // strip any white space in between the last number parsed and the next delimiter
            pairSS >> std::ws;
            if (pairSS.peek() == ':')
                pairSS.ignore();
            pairSS >> p.second;

            parsedPairVector.push_back(p);

            Print::printf(Print::DEBUG, "Parsed %s %s to: %f => %f", debugMessage.c_str(), pairString.c_str(), p.first, p.second);
        }
        return parsedPairVector;
    }

    template <typename T> vector<T>
    parseVector(const std::string &key) const
    {
        std::stringstream vecSS(_map.at(key));

        vector<T> parsedVector;
        T i;
        while (vecSS >> i)
        {
            parsedVector.push_back(i);

            // strip any white space in between the last number parsed and the next delimiter
            vecSS >> std::ws;
            if (vecSS.peek() == ',')
                vecSS.ignore();
        }
        return parsedVector;
    }

// mutators

    void set(const lm::io::SimulationParameters& parameters);

    // for the buf <-> map conversion methods, if you drop an arg it'll use the internal map and/or buf
    void bufToMap() {bufToMap(_buf, _map);}
    void bufToMap(const lm::io::SimulationParameters& inBuf) {bufToMap(inBuf, _map);}
    void bufToMap(SimParamMap& outMap) {bufToMap(_buf, outMap);}
    void bufToMap(const lm::io::SimulationParameters& inBuf, SimParamMap& outMap);

    SimParamMap::iterator findFirst(const std::vector<std::string>& keys);

    void mapToBuf() {mapToBuf(_map, _buf);}
    void mapToBuf(const SimParamMap& inMap) {mapToBuf(inMap, _buf);}
    void mapToBuf(lm::io::SimulationParameters& outBuf) {mapToBuf(_map, outBuf);}
    void mapToBuf(const SimParamMap& inMap, lm::io::SimulationParameters& outBuf);


// const qualified pass-throughs to the underlying SimulationParameters buf and SimParamMap
    SimParamMap::const_iterator find(const std::string& key) const {return _map.find(key);}
    SimParamMap::size_type count(const std::string& key) const {return _map.count(key);}

// pass-throughs to the underlying SimulationParameters buf and SimParamMap
    std::string& operator[](const std::string& key) {return _map[key];}
    SimParamMap::iterator find(const std::string& key) {return _map.find(key);}
    SimParamMap::iterator begin() {return _map.begin();}
    SimParamMap::iterator end() {return _map.end();}

protected:
    lm::io::SimulationParameters _buf;
    SimParamMap _map;
};

}
}

#endif /* LM_INPUT_SIMULATIONPARAMETERS */
