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

#include <limits>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <utility>
#include <vector>

#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/SimulationParameters.pb.h"
#include "lm/Print.h"
#include "lm/Types.h"

namespace lm {
namespace option {

// three different token parsers to ensure correct behavior. Statically dispatched with a combination of SFINAE and template specializaiton
// parser for integral types. Parses first to a double in order to ensure that sci notation (e.g. "1e4") parses as expected. Dispatched by function argument SFINAE
template <typename T>
inline void _parseNextToken(const std::string& tokenString, T* destination)//, typename EnableIf<IsIntegral<T>::value>::type* = 0)
{
    double temp;
    std::stringstream tokenSS(tokenString);

    tokenSS >> temp;
    *destination = static_cast<T>(temp);
}

// parser for booleans. Any of the "1", "true", and "True" tokens convert to true boolean values, everything else converts to false. Dispatched via template specialization
template <>
inline void _parseNextToken<bool>(const std::string& tokenString, bool* destination)//, void*)
{
    *destination = (tokenString=="1" or tokenString=="true" or tokenString=="True");
}

// parser for everything else.
//template <typename T>
//inline void _parseNextToken(const std::string& tokenString, T* destination, typename EnableIfNot<IsIntegral<T>::value>::type*)
//{
//    std::stringstream tokenSS(tokenString);
//
//    tokenSS >> *destination;
//}

// pops the next token from the tokensSS stream and converts it to the appropriate type using the overloads of the >> operator
// if delimiter is set, get everything up to the next delimiter or the EOL and treat that as the next token
template <typename T>
inline bool parseNextToken(std::stringstream* tokensSS, T* destination, char delimiter)
{
    std::string tokenString;
    if (not std::getline(*tokensSS, tokenString, delimiter)) return false;

    _parseNextToken(tokenString, destination);
    return true;
}

// we have no token separator so the next token is everything left in the tokens stream
template <typename T>
inline bool parseNextToken(std::stringstream* tokensSS, T* destination)
{
    std::string tokenString;
    if (not std::getline(*tokensSS, tokenString)) return false;

    _parseNextToken(tokenString, destination);
    return true;
}


class SimulationParameters
{
public:
    typedef std::map<std::string,std::string> SimParamMap;

    SimulationParameters() {}
    SimulationParameters(const lm::input::SimulationParameters& newBuf) {rFB(newBuf);}
    SimulationParameters(const lm::io::hdf5::Hdf5File& file) {rFF(file);}
    SimulationParameters(const SimParamMap& newMap) {rFM(newMap);}
    ~SimulationParameters() {}

// accessors
    SimParamMap::const_iterator findFirst(const std::vector<std::string>& keys) const;
    const lm::input::SimulationParameters& buf() {return _buf;}
    const SimParamMap& map() const {return _map;}
    bool isEnd(SimParamMap::const_iterator it) const {return it==_map.end();}

    template <typename T>
    T parse(const std::string &key) const
    {
        T retVal;
        std::stringstream ss(_map.at(key));

        parseNextToken(&ss, &retVal);
//        ss >> retVal;
        return retVal;
    }

    template <typename T1, typename T2>
    typename PairVector<T1, T2>::T parsePairVector(const std::string &key, const std::string& debugMessage="") const
    {
        typename PairVector<T1, T2>::T parsedPairVector;
        std::stringstream pairVecSS(_map.at(key));
        std::string pairString, tokenString;

        while (getline(pairVecSS, pairString, ','))
        {
            std::pair<T1, T2> p;
            std::stringstream pairSS(pairString);

            parseNextToken(&pairSS, &p.first, ':');
            parseNextToken(&pairSS, &p.second, ':');

//            getline(pairSS, tokenString, ':');
//            std::stringstream firstSS(tokenString);
//            firstSS >> p.first;
//
//            getline(pairSS, tokenString, ':');
//            std::stringstream secondSS(tokenString);
//            secondSS >> p.second;

            parsedPairVector.push_back(p);

            Print::printf(Print::DEBUG, "Parsed %s %s to: %f => %f", debugMessage.c_str(), pairString.c_str(), p.first, p.second);
        }
        return parsedPairVector;
    }

    template <typename T> std::vector<T>
    parseVector(const std::string &key) const
    {
        std::stringstream vectorSS(_map.at(key));

        std::vector<T> parsedVector;
        T token;
        while (parseNextToken(&vectorSS, &token, ','))
        {
            parsedVector.push_back(token);
        }
//        T i;
//        while (tokensSS >> i)
//        {
//            parsedVector.push_back(i);
//
//            // strip any white space in between the last number parsed and the next delimiter
//            tokensSS >> std::ws;
//            if (tokensSS.peek() == ',')
//                tokensSS.ignore();
//        }
        return parsedVector;
    }


// mutators
    // for the buf <-> map conversion methods, if you drop an arg it'll use the internal map and/or buf
    void bufToMap() {bufToMap(_buf, _map);}
    void bufToMap(const lm::input::SimulationParameters& inBuf) {bufToMap(inBuf, _map);}
    void bufToMap(SimParamMap& outMap) {bufToMap(_buf, outMap);}
    void bufToMap(const lm::input::SimulationParameters& inBuf, SimParamMap& outMap);

    SimParamMap::iterator findFirst(const std::vector<std::string>& keys);

    void mapToBuf() {mapToBuf(_map, _buf);}
    void mapToBuf(const SimParamMap& inMap) {mapToBuf(inMap, _buf);}
    void mapToBuf(lm::input::SimulationParameters& outBuf) {mapToBuf(_map, outBuf);}
    void mapToBuf(const SimParamMap& inMap, lm::input::SimulationParameters& outBuf);

    bool rFB(const lm::input::SimulationParameters& inBuf); // rFB = read From Buf
    bool rFF(const lm::io::hdf5::Hdf5File& file); // rFF = read From File
    bool rFM(const SimParamMap& inMap); // rFM = read From Map

    void setBuf(const lm::input::SimulationParameters& newBuf) {_buf.CopyFrom(newBuf);}
    void setMap(const SimParamMap& newMap) {_map = newMap;}

// const qualified pass-throughs to the underlying SimulationParameters buf and SimParamMap
    SimParamMap::const_iterator find(const std::string& key) const {return _map.find(key);}
    SimParamMap::size_type count(const std::string& key) const {return _map.count(key);}

// pass-throughs to the underlying SimulationParameters buf and SimParamMap
    std::string& operator[](const std::string& key) {return _map[key];}
    SimParamMap::iterator find(const std::string& key) {return _map.find(key);}
    SimParamMap::iterator begin() {return _map.begin();}
    SimParamMap::iterator end() {return _map.end();}

protected:
    lm::input::SimulationParameters _buf;
    SimParamMap _map;
};

}
}

#endif /* LM_OPTION_SIMULATIONPARAMETERS */
