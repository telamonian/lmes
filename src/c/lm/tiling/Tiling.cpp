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
#include "lm/io/Tilings.pb.h"
#include "lm/tiling/Tiling.h"
#include "lm/tiling/Tilings.h"
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
    setArrangement(tilingBuf->arrangement(0));
}

lm::io::Tilings::Arrangement Tiling::getArrangement() const
{
    return tilingBuf->arrangement(0);
}

void Tiling::setArrangement(lm::io::Tilings::Arrangement newArr)
{
    if (tilingBuf->arrangement(0)!=newArr)
    {
        reverse();
    }
}

void Tiling::reverse()
{
    tilingBuf->set_arrangement(0, tilingBuf->arrangement(0)==lm::io::Tilings::ASCENDING ? lm::io::Tilings::DESCENDING : lm::io::Tilings::ASCENDING);
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
