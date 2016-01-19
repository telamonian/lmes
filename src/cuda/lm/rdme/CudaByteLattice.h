/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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

#ifndef LM_RDME_CUDABYTELATTICE_H_
#define LM_RDME_CUDABYTELATTICE_H_

#include <vector>
#include <map>
#include "lm/Exceptions.h"
#include "lm/Cuda.h"
#include "lm/rdme/ByteLattice.h"
#include "lm/rdme/Lattice.h"

namespace lm {
namespace rdme {

class CudaByteLattice : public ByteLattice
{
public:
    CudaByteLattice(lattice_coord_t size, si_dist_t spacing, uint particlesPerSite) throw(std::bad_alloc,InvalidArgException,Exception,CUDAException);
    CudaByteLattice(lattice_size_t xSize, lattice_size_t ySize, lattice_size_t zSize, si_dist_t spacing, uint particlesPerSite) throw(std::bad_alloc,InvalidArgException,Exception,CUDAException);
	virtual ~CudaByteLattice() throw(std::bad_alloc);
	
    virtual void copyToGPU() throw(CUDAException);
    virtual void copyFromGPU() throw(CUDAException);
    virtual void * getGPUMemorySrc();
    virtual void * getGPUMemoryDest();
    virtual void swapSrcDest();
    virtual void * getGPUMemorySiteTypes();

	// Override methods that can cause the GPU memory to become stale.
	virtual void setSiteType(lattice_size_t x, lattice_size_t y, lattice_size_t z, site_t site) throw(InvalidSiteException);
	virtual void setSiteType(lattice_size_t index, site_t site) throw(InvalidSiteException);
	virtual void addParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, particle_t particle) throw(InvalidSiteException,InvalidParticleException);
	virtual void addParticle(lattice_size_t index, particle_t particle) throw(InvalidSiteException,InvalidParticleException);
    virtual void removeParticles(lattice_size_t x,lattice_size_t y,lattice_size_t z) throw(InvalidSiteException);
    virtual void removeParticles(lattice_size_t index) throw(InvalidSiteException);
	virtual void removeAllParticles();
    virtual void deserializeParticlesFrom(const void* srcBuffer, size_t bufferSize, SerializationDataOrder dataOrdering, bool inflate);

protected:
    virtual void allocateCudaMemory() throw(CUDAException);
    virtual void deallocateCudaMemory() throw(CUDAException);
	
protected:
    uint cudaParticlesCurrent;
    size_t cudaParticlesSize;
    void * cudaParticles[2];
    size_t cudaSiteTypesSize;
    void * cudaSiteTypes;
    bool isGPUMemorySynched;
};

}
}

#endif
