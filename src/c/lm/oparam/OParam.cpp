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
#inlcude "lm/io/FFluxParameters.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/oparam/OParam.h"
#include "lm/ClassFactory.h"

namespace lm {
namespace oparam {


bool OParamLinear::registered=OParamLinear::registerClass();

bool OParamLinear::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::oparam::OParam","lm::oparam::OParamLinear",&OParamLinear::allocateObject);
    return true;
}

void* OParamLinear::allocateObject()
{
    return new OParamLinear();
}


OParam::OParam(): val() {}

OParamLinear::OParamLinear(lm::io::FFluxParameters::OrderParameter& op): OParam(), op(op), size(op.species_id_size()), speciesID(op.species_id().data()), speciesCoefficient(op.species_coefficient().data()) {}

double OParamLinear::calc(lm::io::TrajectoryState& state)
{
    double ret = 0;
    for (int i=0;i<size;++i)
    {
        ret += (double)(state.cme_state().species_counts().species_count(speciesID[i]) * speciesCoefficient[i]);
    }
    return ret;
}

}
}
