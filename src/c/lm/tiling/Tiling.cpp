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
#include <algorithm>

#include "lm/ClassFactory.h"
#include "lm/EnumHelper.h"
#include "lm/io/Tilings.pb.h"
#include "lm/tiling/Tiling.h"
#include "lm/tiling/Tilings.h"
#include "lm/trajectory/TrajectoryLimits.h"
#include "lm/Types.h"

namespace lm {
namespace tiling {

// base class Tiling methods
Tiling::Tiling(): tilingBuf(NULL)
{
}

Tiling::~Tiling()
{
    if (tilingBuf!=NULL) delete tilingBuf; tilingBuf = NULL;
}

void Tiling::init(const lm::io::Tilings::Tiling& tilingRef)
{
    tilingBuf = new lm::io::Tilings::Tiling(tilingRef);
    setSortOrder(tilingBuf->sort_order(0));
}

// flips the stopping condition of the added limits around depending on whether the tiling's edges currently sort ascending or descending
Tiling::TrajectoryLimitBuf* Tiling::addLimitBuf(lm::trajectory::TrajectoryLimits& tls, uint edgeIndex, EH::StoppingCondition stoppingCondition, bool rightOpenBins, int32_t limitID) const
{
    // if the tiling sorts descending, flip the stopping condition around
    if (getSortOrder()==EH::DESCENDING)
    {
        switch (stoppingCondition)
        {
        case EH::MIN: stoppingCondition = EH::MAX; break;
        case EH::MAX: stoppingCondition = EH::MIN; break;
        case EH::DECREASING: stoppingCondition = EH::INCREASING; break;
        case EH::INCREASING: stoppingCondition = EH::DECREASING; break;
        default: break;
        }
    }
    
    // keep the includeEndpoint property of the added limit consistent with right-open bins on this tiling, or with left-open bins if rightOpenBins is false
    bool includeEndpoint;
    switch (stoppingCondition)
    {
    case EH::MIN: includeEndpoint = rightOpenBins; break;
    case EH::MAX: includeEndpoint = !rightOpenBins; break;
    case EH::DECREASING: includeEndpoint = rightOpenBins; break;
    case EH::INCREASING: includeEndpoint = !rightOpenBins; break;
    default: break;
    }

    return tls.addLimitBuf<EH::ORDER_PARAMETER>(getOrderParameterID(), getEdge(edgeIndex), stoppingCondition, includeEndpoint, limitID);
}

io::Tilings::SortOrder Tiling::getSortOrder() const
{
    return tilingBuf->sort_order(0);
}

void Tiling::setSortOrder(io::Tilings::SortOrder newArr)
{
    // for a 1D tiling there are only two possible sort orders, so either leave things alone or call .reverse()
    if (tilingBuf->sort_order(0)!=newArr)
    {
        reverse();
    }
}

void Tiling::reverse()
{
    tilingBuf->set_sort_order(0, tilingBuf->sort_order(0)==lm::io::Tilings::ASCENDING ? lm::io::Tilings::DESCENDING : lm::io::Tilings::ASCENDING);
    int revLoops = tilingBuf->edges_size()/2;
    for (int i=0;i<revLoops;++i)
    {
        tilingBuf->mutable_edges()->SwapElements(i, tilingBuf->edges_size()-(i+1));
    }
}

uint Tiling::getTileIndex(double opVal)
{
    EdgeIterator up;
    up = std::upper_bound(tilingBuf->edges().begin(), tilingBuf->edges().end(), opVal);
    return up - tilingBuf->edges().begin();
}

// derived class methods
bool TilingLattice::registered=TilingLattice::registerClass();
bool TilingLattice::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::tiling::Tiling","lm::tiling::TilingBin",&TilingLattice::allocateObject);
//    lm::tiling::Tilings::tilingClassMap[0] = "lm::tiling::TilingBin";
    return true;
}
void* TilingLattice::allocateObject()
{
    return new TilingLattice();
}

TilingLattice::TilingLattice(): Tiling() {}

void TilingLattice::init(const lm::io::Tilings::Tiling& tilingRef)
{
    // call parent method
    Tiling::init(tilingRef);
}

}
}
