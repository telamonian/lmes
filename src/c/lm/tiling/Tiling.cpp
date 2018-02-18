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
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/Tilings.pb.h"
#include "lm/tiling/Tiling.h"
#include "lm/tiling/Tilings.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/Types.h"

namespace lm {
namespace tiling {

// base class Tiling methods
Tiling::Tiling(): oparam(NULL),tilingMsg(NULL)
{
}

Tiling::~Tiling()
{
}

void Tiling::init(lm::input::Tiling* newTilingMsg, const lm::oparam::OParams& newOParams)
{
    setTilingMsg(newTilingMsg);
    setOrderParameter(*newOParams.at(getOrderParameterID()));
}

TilingEnums::SortOrder Tiling::calcSortOrder(bool reverseSort) const
{
    if (edges().last()>=edges().first()) return (reverseSort ? TilingEnums::DESCENDING : TilingEnums::ASCENDING);
    else                                 return (reverseSort ? TilingEnums::ASCENDING  : TilingEnums::DESCENDING);
}

TilingEnums::SortOrder Tiling::getSortOrder() const
{
    return tilingMsg->sort_orders(0);
}

double Tiling::getEdgeFixBounds(int edgeIndex) const
{
    // if edgeIndex is outside of the bounds of the edges() list, return a "pretend" edge shifted one unit out from the nearest actual edge
    // useful for certain calculations
    if      (edgeIndex < 0)                   return edges().first() + (getSortOrder()==TilingEnums::ASCENDING ? -1.0 :  1.0);
    else if (edgeIndex > edges().lastIndex()) return edges().last()  + (getSortOrder()==TilingEnums::ASCENDING ?  1.0 : -1.0);
    else                                      return edges(edgeIndex);
}

void Tiling::reverse()
{
//    tilingMsg->set_sort_orders(0, tilingMsg->sort_orders(0)==TilingEnums::ASCENDING ? TilingEnums::DESCENDING : TilingEnums::ASCENDING);

    tilingMsg->set_sort_orders(0, calcSortOrder(true));
    int revLoops = tilingMsg->edges_size()/2;
    for (int i=0;i<revLoops;++i)
    {
        tilingMsg->mutable_edges()->SwapElements(i, tilingMsg->edges_size()-(i+1));
    }

    set_is_reversed(!is_reversed());
}

uint Tiling::getTileIndex(double opVal) const
{
    EdgesT::const_iterator upper;
    upper = std::upper_bound(tilingMsg->edges().begin(), tilingMsg->edges().end(), opVal);
    return upper - tilingMsg->edges().begin();
}

void Tiling::setBasin(int64_t basinIndex)
{
    set_current_basin_id(basinIndex);
    if (getTileIndexFromBasin(basinIndex)!=0)
    {
        reverse();
    }
}

void Tiling::setOrderParameter(const lm::oparam::OParam& newOParam)
{
    oparam = &newOParam;
    setOrderParameterID(oparam->id());
}

void Tiling::setSortOrder(TilingEnums::SortOrder newOrder)
{
    // for a 1D tiling there are only two possible sort orders, so either leave things alone or call .reverse()
    if (tilingMsg->sort_orders(0)!=newOrder)
    {
        reverse();
    }
}

void Tiling::setTilingMsg(lm::input::Tiling* newTilingMsg)
{
    tilingMsg = newTilingMsg;

    _basins.setWrappedField(tilingMsg->mutable_basins());
    _edges.setWrappedField(tilingMsg->mutable_edges());

    tilingMsg->set_sort_orders(0, calcSortOrder());
    if (tilingMsg->is_reversed_size()==0) set_is_reversed(false);
}

bool Tiling::testBasinsPosition() const
{
    for (int i=0;i<basins().size();i++)
    {
        if (not testBasinPosition(i)) return false;
    }
    return true;
}

bool Tiling::testBasinPosition(int basinIndex) const
{
    int basinTileIndex = getTileIndexFromBasin(basinIndex);
    if (basinTileIndex!=0 and basinTileIndex!=getLastTileIndex())
    {
        THROW_EXCEPTION(ConsistencyException, "Basin %d in tiling ID %d located within tile with index %d.\n"
                        "All basins should be in first or last tile (ie in front of the zeroth edge or\n"
                        "behind the last edge)", basinIndex, id(), basinTileIndex);
    }
    return true;
}

bool Tiling::testBasinsSize(lm::input::ReactionModel& reactionModel) const
{
    for (int i=0;i<basins().size();i++)
    {
        if (not testBasinSize(i, reactionModel)) return false;
    }
    return true;
}

bool Tiling::testBasinSize(int basinIndex, lm::input::ReactionModel& reactionModel) const
{
    if (basins(basinIndex).species_count_size()!=reactionModel.number_species())
    {
        THROW_EXCEPTION(ConsistencyException, "Basin %d in tiling ID %d has %d species count entries. Should have %d",
                        basinIndex, id(), basins(basinIndex).species_count_size(), reactionModel.number_reactions());
    }
    return true;
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

void TilingLattice::init(lm::input::Tiling* newTilingMsg, const lm::oparam::OParams& oparams)
{
    // call parent method
    Tiling::init(newTilingMsg, oparams);
}

}
}
