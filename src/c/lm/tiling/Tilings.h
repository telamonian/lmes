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
#ifndef LM_TILING_TILINGS
#define LM_TILING_TILINGS

#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/Tilings.pb.h"
#include "lm/tiling/Tiling.h"

namespace lm {
namespace tiling {

typedef std::map<uint,std::string> TilingClassMap;
typedef google::protobuf::RepeatedPtrField<lm::io::Tilings::Tiling>::const_iterator TilingIterator;
typedef std::map<uint,lm::tiling::Tiling*> TilingMap;

class Tilings
{
public:
    // constructors/destructors/initializers
    Tilings();
    Tilings(const lm::io::Tilings& tilings);
    ~Tilings();
    void clearTilingMap();
    bool init(lm::io::hdf5::Hdf5File* file);
    void init(const lm::io::Tilings& tilings);
    void init();
    void initTiling(const lm::io::Tilings::Tiling& tiling);

    // operators
    lm::tiling::Tiling* operator[](uint i) {return tilingMap[i];}
    TilingMap::iterator begin() {return tilingMap.begin();}
    TilingMap::iterator end() {return tilingMap.end();}

    // accessors
    bool hasCurrentTilingID() {return getTilingsBuf()->has_current_tiling_id();}
    lm::tiling::Tiling* getCurrentTiling() {return tilingMap[getCurrentTilingID()];}
    uint getCurrentTilingID();
    lm::io::Tilings* getTilingsBuf() {return &tilingsBuf;}

    // mutators
    void reverse(); // reverse order of list of edges
    bool rFFTilingsBuf(lm::io::hdf5::Hdf5File* file); // rFF = read From File
    void setCurrentTilingID(uint newCurrentTilingID) {currentTilingID = newCurrentTilingID;}
    void setTilingsBuf(const lm::io::Tilings& newTilingsBuf) {*getTilingsBuf() = newTilingsBuf;}

    // static methods
    static TilingClassMap tilingClassMap;
    static TilingClassMap makeTilingClassMap()
    {
        std::map<uint,std::string> m;
        m[0] = "lm::tiling::TilingBin";
        return m;
    }
protected:
    uint currentTilingID;

private:
    TilingMap tilingMap;
    lm::io::Tilings tilingsBuf;
};

}
}

#endif /* LM_TILING_TILINGS */
