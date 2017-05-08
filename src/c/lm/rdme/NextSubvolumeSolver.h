/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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

#ifndef LM_RDME_NEXTSUBVOLUMESOLVER_H_
#define LM_RDME_NEXTSUBVOLUMESOLVER_H_

#include "lm/rdme/Lattice.h"
#include "lm/rdme/RDMESolver.h"
#include "lm/reaction/ReactionQueue.h"

using lm::reaction::ReactionQueue;

namespace lm {
namespace rdme {

class NextSubvolumeSolver : public RDMESolver
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

public:
    NextSubvolumeSolver();
    NextSubvolumeSolver(RandomGenerator::Distributions neededDists);
    virtual ~NextSubvolumeSolver();
    virtual void setDiffusionModel(const lm::input::DiffusionModel& dm);
    virtual void reset();
    virtual long long generateTrajectory(long long maxSteps);

protected:
    virtual void checkSpeciesCountsAgainstLattice();
    //virtual void writeLatticeData(double time, ByteLattice * lattice, lm::types::Lattice * latticeDataSet);
    //virtual void recordSpeciesCounts(double time, lm::io::SpeciesCounts * speciesCountsDataSet);
    //virtual void writeSpeciesCounts(lm::io::SpeciesCounts * speciesCountsDataSet);
    virtual int updateAllSubvolumePropensities(si_time_t time, int rngNext, double * expRngValues);
    virtual int updateSubvolumePropensity(si_time_t time, lattice_size_t s, int rngNext, double * expRngValues);
    virtual double calculateSubvolumePropensity(si_time_t time, lattice_size_t s);
    inline double calculateSubvolumeDiffusionPropensity(si_time_t time, lattice_size_t subvolume, site_t sourceSite);
    inline double calculateSubvolumeInfluxPropensity(si_time_t time, lattice_size_t subvolume);
    virtual int performSubvolumeEvent(si_time_t time, lattice_size_t s, int rngNext, double * uniRngValues, bool& affectedNeighbor, lattice_size_t& neighborSubvolume);
    inline bool performSubvolumeDiffusionEvent(si_time_t time, lattice_size_t subvolume, site_t sourceSite, double& rngValue, bool& affectedNeighbor, lattice_size_t& neighborSubvolume);
    inline bool performSubvolumeInfluxEvent(si_time_t time, lattice_size_t subvolume, double& rngValue);
    virtual void updateCurrentSubvolumeSpeciesCounts(uint r);
    virtual void  updateSpeciesCountsForSubvolume(lattice_size_t subvolume);
    void updateSubvolumeWithSpeciesCounts(lattice_size_t subvolume);
    void addParticles(lattice_size_t subvolume, particle_t particle, uint count);

protected:
    lattice_size_t numberSubvolumes;
    double latticeSpacingSquared;
    ReactionQueue* reactionQueue;
    int* currentSubvolumeSpeciesCounts;                           // numberSpecies
};

}
}

#endif
