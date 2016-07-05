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
#ifndef LM_TILING_TILING
#define LM_TILING_TILING

#include "lm/EnumHelper.h"
#include "lm/input/Tilings.pb.h"
#include "lm/oparam/OParam.h"
#include "lm/oparam/OParams.h"
#include "lm/protowrap/Repeated.h"
#include "lm/Types.h"

//#include "lm/trajectory/TrajectoryLimits.h"

namespace lm {
namespace tiling {

typedef lm::protowrap::Repeated<int32_t> BasinT;
typedef lm::protowrap::Repeated<lm::input::Basin> BasinsT;
typedef lm::protowrap::Repeated<double> EdgesT;

// TODO: fix many CV qualifier problems in Tiling
class Tiling
{
public:
// typedefs
    typedef lm::input::TrajectoryLimit TrajectoryLimitBuf;

// initializers
    Tiling();
    virtual ~Tiling();
    virtual void init(lm::input::Tiling* tilingMsg, const lm::oparam::OParams& oparams);

// accessors
    const lm::input::Basin& currentBasin() const {return basins(current_basin_index());}
    TilingEnums::SortOrder calcSortOrder(bool reverseSort=false) const;
    TilingEnums::SortOrder getSortOrder() const;
    uint64_t getEdgeDims(uint dimIndex) const {return tilingMsg->edge_dims(dimIndex);}
    int getEdgesCount() const {return tilingMsg->edges_size();}
    double getEdgeFixBounds(int edgeIndex) const;
    int getLastEdgeIndex() const {return getEdgesCount() - 1;}
    int getLastTileIndex() const {return getEdgesCount();}
    uint getOrderParameterID() const {return getOrderParameterIDs(0);}    // 1D version of getOrderParameterIDs, for backwards compatibility
    uint getOrderParameterIDs(uint opIndex) const {return tilingMsg->order_parameter_ids(opIndex);}
    uint getTileIndex(double opVal) const;    // get the index of the tile for making a histogram based on the tiling
    const lm::input::Tiling& getTilingMsg() const {return *tilingMsg;}
    uint id() const {return tilingMsg->id();}

    // methods for working with basins
    int getTileIndexFromBasin(int basinIndex) const {return getTileIndex(getBasinOPVal(basinIndex));}
    double getBasinOPVal(int basinIndex) const {return oparam->calc(basins(basinIndex).species_count().data(), 0);}
    bool testBasinPosition(int basinIndex) const;
    bool testBasinsPosition() const;
    bool testBasinSize(int basinIndex, lm::input::ReactionModel& reactionModel) const;
    bool testBasinsSize(lm::input::ReactionModel& reactionModel) const;

// mutators
    void addOrderParameterIDs(uint opID) {tilingMsg->add_order_parameter_ids(opID);}
    void clearOrderParameterIDs() {tilingMsg->clear_order_parameter_ids();}
    EdgesT* mutable_edges() {return &_edges;}
    void reverse();
    void setBasin(int basinIndex);
    void setOrderParameterID(uint opID) {clearOrderParameterIDs(); addOrderParameterIDs(opID);} // 1D version of setOrderParameterIDs, for backwards compatibility
    virtual void setOrderParameter(const lm::oparam::OParam& newOParam);
    void setSortOrder(TilingEnums::SortOrder newOrder);
    virtual void setTilingMsg(lm::input::Tiling* newTilingMsg);

// pass-throughs
// accessors
    const BasinsT& basins() const {return _basins;}
    const lm::input::Basin& basins(int basinIndex) const {return _basins(basinIndex);}
    int32_t current_basin_index() const {return tilingMsg->current_basin_index();}
    bool is_reversed() const {return tilingMsg->is_reversed(0);}
    const EdgesT& edges() const {return _edges;}
    double edges(uint edgeIndex) const {return _edges(edgeIndex);}

// mutators
    void set_current_basin_index(int32_t newBasin) {tilingMsg->set_current_basin_index(newBasin);}
    void set_is_reversed(bool isReversed) {tilingMsg->clear_is_reversed(); tilingMsg->add_is_reversed(isReversed);}

public:
    const lm::oparam::OParam* oparam;

protected:
    lm::input::Tiling* tilingMsg;

    BasinsT _basins;
    EdgesT _edges;
};

class TilingLattice : public Tiling
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

    TilingLattice();
    virtual ~TilingLattice() {}
    virtual void init(lm::input::Tiling* newTilingMsg, const lm::oparam::OParams& oparams);
};

}
}

#endif /* LM_TILING_TILING */
