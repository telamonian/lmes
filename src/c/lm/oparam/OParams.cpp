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
//#include <algorithm>
#include "lm/io/OrderParameters.pb.h"
#include "lm/ClassFactory.h"
#include "lm/oparam/OParam.h"
#include "lm/oparam/OParams.h"

namespace lm {
namespace oparam {

OPClassMap OParams::opClassMap = OParams::makeOPClassMap();

OParams::OParams(): size_(0)
{
}

OParams::~OParams()
{
    clearOPMap();
}

void OParams::clearOPMap()
{
    for (OPMap::iterator m_it=begin();m_it!=end();++m_it)
    {
        if (m_it->second!=NULL) delete m_it->second; m_it->second = NULL;
        opMap.erase(m_it);
    }
}

bool OParams::init(const lm::io::hdf5::Hdf5File* file)
{
    if(rFFOParamsBuf(file))
    {
        init();
        return true;
    }
    else
    {
        return false;
    }
}

void OParams::init(const lm::io::OrderParameters& newOParamsBuf)
{
    setOParamsBuf(newOParamsBuf);
    init();
}

// will need to have somehow initialized oparamsBuf before calling this version of init()
void OParams::init()
{
    clearOPMap();
    // for_each doesn't work with member functions, and part of the fix for this (bind1st) doesn't work with functions that take const reference arguments. C++ everyone!
    // std::for_each(ops.order_parameters().begin(), ops.order_parameters().end(), std::bind1st(std::mem_fun(&OParams::initOParam),this));
    for (OPIterator op_it=getOParamsBuf()->order_parameters().begin();op_it!=getOParamsBuf()->order_parameters().end();++op_it)
    {
        initOParam(*op_it);
        size_++;
    }
}

void OParams::initOParam(const lm::io::OrderParameters::OrderParameter& oparam)
{
    opMap[oparam.id()] = (static_cast<lm::oparam::OParam*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::oparam::OParam",lm::oparam::OParams::opClassMap[oparam.type()])));
    opMap[oparam.id()]->init(oparam);
}

void OParams::initValues(uint* speciesCounts, double time)
{
    for (OPMap::iterator m_it=begin();m_it!=end();++m_it)
    {
        m_it->second->initValues(speciesCounts, time);
    }
}

void OParams::calc(uint* speciesCounts, double time)
{
    for (OPMap::iterator m_it=begin();m_it!=end();++m_it)
    {
        m_it->second->calc(speciesCounts, time);
    }
}

bool OParams::rFFOParamsBuf(const lm::io::hdf5::Hdf5File* file)
{
    if (file->hasOrderParameters())
    {
        file->getOrderParameters(getOParamsBuf());
        return true;
    }
    else
    {
        return false;
    }
}

}
}
