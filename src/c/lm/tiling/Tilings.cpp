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

namespace lm {
namespace tiling {

TilingClassMap Tilings::tilingClassMap = Tilings::makeTilingClassMap();

Tilings::Tilings()
{
}

Tilings::Tilings(const lm::io::Tilings& newTilingsBuf)
{
    init(newTilingsBuf);
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
        tilingMap.erase(m_it);
    }
}

bool Tilings::init(lm::io::hdf5::Hdf5File* file)
{
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

void Tilings::init(const lm::io::Tilings& newTilingsBuf)
{
    setTilingsBuf(newTilingsBuf);
    init();
}

// will need to have somehow initialized tilingsBuf before calling this version of init()
void Tilings::init()
{
    clearTilingMap();
    for (TilingIterator t_it=getTilingsBuf()->tilings().begin();t_it!=getTilingsBuf()->tilings().end();++t_it) initTiling(*t_it);
}

void Tilings::initTiling(const lm::io::Tilings::Tiling& tiling)
{
    tilingMap[tiling.id()] = (static_cast<lm::tiling::Tiling*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::tiling::Tiling",lm::tiling::Tilings::tilingClassMap[tiling.type()])));
    tilingMap[tiling.id()]->init(tiling);
}

void Tilings::reverse()
{
    for (TilingMap::iterator m_it=begin();m_it!=end();++m_it)
    {
        m_it->second->reverse();
    }
}

bool Tilings::rFFTilingsBuf(lm::io::hdf5::Hdf5File* file)
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
