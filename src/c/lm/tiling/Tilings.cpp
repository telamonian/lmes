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
#include "lm/input/Tilings.pb.h"
#include "lm/tiling/Tiling.h"
#include "lm/tiling/Tilings.h"

namespace lm {
namespace tiling {

TilingClassMap Tilings::tilingClassMap = Tilings::makeTilingClassMap();

Tilings::Tilings(): currentTilingID(-1), oparams(NULL)
{
}

Tilings::Tilings(const lm::input::Tilings& newTilingsBuf, const lm::oparam::OParams& newOParams) : currentTilingID(-1), oparams(NULL)
{
    init(newTilingsBuf, newOParams);
}

Tilings::~Tilings()
{
    clearTilingMap();
}

void Tilings::clearTilingMap()
{
    for (TilingMap::iterator m_it=begin();m_it!=end();++m_it)
    {
        if (m_it->second!=NULL) delete m_it->second; m_it->second = NULL;
    }
    tilingMap.clear();
}

bool Tilings::init(const lm::io::hdf5::Hdf5File* file, const lm::oparam::OParams& newOParams)
{
    setOParams(newOParams);
    if (rFFTilingsBuf(file))
    {
        init();
        return true;
    }
    else
    {
        return false;
    }
}

void Tilings::init(const lm::input::Tilings& newTilingsBuf, const lm::oparam::OParams& newOParams)
{
    setOParams(newOParams);
    setTilingsBuf(newTilingsBuf);
    if (getTilingsBuf()->has_current_tiling_id())
    {
        currentTilingID = getTilingsBuf()->current_tiling_id();
    }
    init();
}

// will need to have somehow initialized tilingsBuf before calling this version of readHDF5Input()
void Tilings::init()
{
    clearTilingMap();
    for (TilingIterator t_it=getTilingsBuf()->mutable_tilings()->begin();t_it!=getTilingsBuf()->mutable_tilings()->end();++t_it)
    {
        initTiling(&*t_it);
    }
}

void Tilings::initTiling(lm::input::Tiling* tiling)
{
    tilingMap[tiling->id()] = (static_cast<lm::tiling::Tiling*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::tiling::Tiling",lm::tiling::Tilings::tilingClassMap[tiling->type()])));
    tilingMap[tiling->id()]->init(tiling, *oparams);
}

// accessors
uint Tilings::getCurrentTilingID() const
{
    if (hasCurrentTilingID())
    {
        return tilingsBuf.current_tiling_id();
    }
    else
    {
        return tilingMap.begin()->first;
    }
}

bool Tilings::testBasinsPosition() const
{
    for (TilingMap::const_iterator it=begin();it!=end();it++)
    {
        if (not it->second->testBasinsPosition()) return false;
    }
    return true;
}

bool Tilings::testBasinsSize(lm::input::ReactionModel& reactionModel) const
{
    for (TilingMap::const_iterator it=begin();it!=end();it++)
    {
        if (not it->second->testBasinsSize(reactionModel)) return false;
    }
    return true;
}

// mutators
void Tilings::reverse()
{
    for (TilingMap::iterator m_it=begin();m_it!=end();++m_it)
    {
        m_it->second->reverse();
    }
}

bool Tilings::rFFTilingsBuf(const lm::io::hdf5::Hdf5File* file)
{
    if (file->hasTilings())
    {
        file->getTilings(getTilingsBuf());
        return true;
    }
    else
    {
        return false;
    }
}


}
}
