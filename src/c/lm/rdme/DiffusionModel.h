/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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
 * Author(s): Elijah Roberts
 */

#ifndef LM_RDME_DIFFUSIONMODEL_H_
#define LM_RDME_DIFFUSIONMODEL_H_

#include "lm/Types.h"
#include "lm/types/BoundaryConditions.pb.h"
#include "lm/input/DiffusionModel.pb.h"
#include "robertslab/Types.h"

namespace lm {
namespace rdme {

class DiffusionModel
{
public:
    DiffusionModel(const lm::input::DiffusionModel& dm);
    virtual ~DiffusionModel();

    const uint numberSpecies;
    const uint numberReactions;
    uint numberSiteTypes;
    double* DF;                             // Diffusion matrix: numberSiteTypes x numberSiteTypes x numberSpecies
    bool* RL;								// Reaction location matrix: numberReactions x numberSiteTypes
    double latticeSpacing;
    uint latticeXSize;
    uint latticeYSize;
    uint latticeZSize;
    uint particlesPerSite;
    ndarray<uint8_t>* sites;
    lm::types::BoundaryConditions boundaryConditions;
    bool hasBoundaryInflux;
    double* boundaryInflux;
};

}
}

#endif
