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
//#include "lm/trajectory/TrajectoryLimits.h"
#include "lm/Types.h"

namespace lm {
namespace tiling {

typedef google::protobuf::RepeatedField<double>::const_iterator EdgeIterator;

class Tiling
{
public:
// typedefs
    typedef lm::input::TrajectoryLimit TrajectoryLimitBuf;

// initializers
    Tiling();
    virtual ~Tiling();
    virtual void init(const lm::input::Tiling& tilingRef);

// accessors
    TrajectoryLimitBuf* addLimitBuf(lm::trajectory::TrajectoryLimits& tls, uint edgeIndex, TrajLimEnums::StoppingCondition stoppingCondition,
                                    bool rightOpenBins=true, int32_t limitID=lm::trajectory::TrajectoryLimits::DEFAULT_LIMIT_ID) const;
//    double getAscendingLimit(uint edgeIndex);
//    double getDescendingLimit(uint edgeIndex);
    EdgeIterator begin() const {return tilingBuf->edges().begin();}
    EdgeIterator end() const {return tilingBuf->edges().end();}
    TilingEnums::SortOrder getSortOrder() const;
    uint64_t getEdgeDims(uint dimIndex) const {return tilingBuf->edge_dims(dimIndex);}
    double getEdge(uint edgeIndex) const {return tilingBuf->edges(edgeIndex);}
    int getEdgesCount() const {return tilingBuf->edges_size();}
    double getLastEdge() const {return getEdge(getLastEdgeIndex());}
    uint getLastEdgeIndex() const {return getEdgesCount() - 1;}
    uint getID() const {return tilingBuf->id();}
    uint getOrderParameterID() const {return getOrderParameterIDs(0);}    // 1D version of getOrderParameterIDs, for backwards compatibility
    uint getOrderParameterIDs(uint opIndex) const {return tilingBuf->order_parameter_ids(opIndex);}
    uint64_t getRank() const {return tilingBuf->rank();}
    uint getTileIndex(double opVal);    // get the index of the tile for making a histogram based on the tiling

// mutators
    void addOrderParameterIDs(uint opID) {tilingBuf->add_order_parameter_ids(opID);}
    void clearOrderParameterIDs() {tilingBuf->clear_order_parameter_ids();}
    void reverse();
    void setSortOrder(TilingEnums::SortOrder sortOrder);
    void setOrderParameterID(uint opID) {clearOrderParameterIDs(); addOrderParameterIDs(opID);} // 1D version of setOrderParameterIDs, for backwards compatibility


protected:
    lm::input::Tiling* tilingBuf;
};

class TilingLattice : public Tiling
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

    TilingLattice();
    virtual ~TilingLattice() {}
    virtual void init(const lm::input::Tiling& tilingRef);
};

}
}

#endif /* LM_TILING_TILING */
