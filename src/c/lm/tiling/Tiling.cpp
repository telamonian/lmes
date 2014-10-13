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
#include "lm/ClassFactory.h"
#include "lm/io/Tilings.pb.h"
#include "lm/tiling/Tiling.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

namespace lm {
namespace tiling {

// base class OParam methods
Tiling::Tiling(): tiling(NULL)
{
}

Tiling::~Tiling()
{
    if (tiling!=NULL) delete tiling; tiling = NULL;
}

void Tiling::init(const lm::io::Tilings::Tiling& tilingRef)
{
    tiling = new lm::io::Tilings::Tiling(tilingRef);
    setArrangement(tiling->arrangement());
}

lm::io::Tilings::Arrangement Tiling::getArrangment()
{
    return tiling->arrangement();
}

void Tiling::setArrangement(lm::io::Tilings::Arrangement newArr)
{
    if (tiling->arrangement()!=newArr)
    {
        reverse();
    }
}

void Tiling::reverse()
{
    tiling->set_arrangement(tiling->arrangement()==lm::io::Tilings::ASCENDING ? lm::io::Tilings::DESCENDING : lm::io::Tilings::ASCENDING);
    int revLoops = tiling->bin_borders_size()/2;
    for (int i=0;i<revLoops;++i)
    {
        tiling->mutable_bin_borders()->SwapElements(i, tiling->bin_borders_size()-(i+1));
    }
}

double Tiling::getLowerLimit(uint borderIndex)
{
    return tiling->bin_borders(borderIndex - 1);
}

double Tiling::getUpperLimit(uint borderIndex)
{
    return tiling->bin_borders(borderIndex);
}

// derived class methods
bool TilingBin::registered=TilingBin::registerClass();
bool TilingBin::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::tiling::Tiling","lm::tiling::TilingBin",&TilingBin::allocateObject);
    lm::tiling::Tilings::tilingClassMap[0] = "lm::tiling::TilingBin";
    return true;
}
void* TilingBin::allocateObject()
{
    return new TilingBin();
}

TilingBin::TilingBin(): Tiling() {}

void TilingBin::init(const lm::io::Tilings::Tiling& tilingRef)
{
    // call parent method
    Tiling::init(tilingRef);
}

}
}
