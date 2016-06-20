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
#include "lm/input/Tilings.pb.h"
#include "lm/tiling/Tiling.h"

namespace lm {
namespace tiling {

typedef std::map<uint,std::string> TilingClassMap;
typedef google::protobuf::RepeatedPtrField<lm::input::Tiling>::iterator TilingIterator;
typedef std::map<uint,lm::tiling::Tiling*> TilingMap;

class Tilings
{
public:
    typedef TilingMap::iterator iterator;
    typedef TilingMap::const_iterator const_iterator;

    // operators
    lm::tiling::Tiling* operator[](uint i) {return tilingMap[i];}

    // constructors/destructors/initializers
    Tilings();
    Tilings(const lm::input::Tilings& tilings, const lm::oparam::OParams& newOParams);
    ~Tilings();
    void clearTilingMap();
    bool init(const lm::io::hdf5::Hdf5File* file, const lm::oparam::OParams& newOParams);
    void init(const lm::input::Tilings& tilings, const lm::oparam::OParams& newOParams);
    void init();
    void initTiling(lm::input::Tiling* tiling);

    // accessors
    const_iterator begin() const {return tilingMap.begin();}
    const_iterator end() const {return tilingMap.end();}
    bool hasCurrentTilingID() const {return tilingsBuf.has_current_tiling_id();}
    const lm::tiling::Tiling& getCurrentTiling() const {return *tilingMap.at(getCurrentTilingID());}
    uint getCurrentTilingID() const;
    lm::input::Tilings* getTilingsBuf() {return &tilingsBuf;}

    // method that work with basins
    bool testBasinsPosition() const;
    bool testBasinsSize(lm::input::ReactionModel& reactionModel) const;

    // mutators
    iterator begin() {return tilingMap.begin();}
    iterator end() {return tilingMap.end();}
    void reverse(); // reverse order of list of edges
    bool rFFTilingsBuf(const lm::io::hdf5::Hdf5File* file); // rFF = read From File
    void setCurrentTilingID(uint newCurrentTilingID) {currentTilingID = newCurrentTilingID;}
    void setOParams(const lm::oparam::OParams& newOParams) {oparams = &newOParams;}
    void setTilingsBuf(const lm::input::Tilings& newTilingsBuf) {*getTilingsBuf() = newTilingsBuf;}

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
    const lm::oparam::OParams* oparams;

private:
    TilingMap tilingMap;
    lm::input::Tilings tilingsBuf;
};

}
}

#endif /* LM_TILING_TILINGS */
