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

#include "lm/Types.h"
#include "lm/Exceptions.h"
#include "lm/rdme/ByteLattice.h"
#include "lm/rdme/CudaByteLattice.h"
#include "lm/rdme/Lattice.h"

namespace lm {
namespace rdme {

CudaByteLattice::CudaByteLattice(lattice_coord_t size, si_dist_t latticeSpacing, uint particlesPerSite)
throw(std::bad_alloc,InvalidArgException,Exception,CUDAException)
:ByteLattice(size,latticeSpacing,particlesPerSite),cudaParticlesCurrent(0),cudaParticlesSize(0),cudaSiteTypesSize(0),cudaSiteTypes(NULL),isGPUMemorySynched(false)
{
    // Initialize the pointers.
    cudaParticles[0] = NULL;
    cudaParticles[1] = NULL;

    // Make sure the lattice dimensions are divisible by 32.
    if (size.x%32 != 0 || size.y%32 != 0 || size.z%32 != 0) throw InvalidArgException("size","each dimension of a CUDA lattice must be divisible by 32");
    allocateCudaMemory();
}

CudaByteLattice::CudaByteLattice(lattice_size_t xSize, lattice_size_t ySize, lattice_size_t zSize, si_dist_t latticeSpacing, uint particlesPerSite)
throw(std::bad_alloc,InvalidArgException,Exception,CUDAException)
:ByteLattice(xSize,ySize,zSize,latticeSpacing,particlesPerSite),cudaParticlesCurrent(0),cudaParticlesSize(0),cudaSiteTypesSize(0),cudaSiteTypes(NULL),isGPUMemorySynched(false)
{
    // Initialize the pointers.
    cudaParticles[0] = NULL;
    cudaParticles[1] = NULL;

    // Make sure the lattice dimensions are divisible by 32.
    if (size.x%32 != 0 || size.y%32 != 0 || size.z%32 != 0) throw InvalidArgException("size","each dimension of a CUDA lattice must be divisible by 32");
    allocateCudaMemory();
}

CudaByteLattice::~CudaByteLattice()
throw(std::bad_alloc)
{
    deallocateCudaMemory();
}

void CudaByteLattice::allocateCudaMemory()
throw(CUDAException)
{
    // Allocate memory on the CUDA device.
    cudaParticlesSize=numberSites*wordsPerSite*sizeof(uint32_t);
    CUDA_EXCEPTION_CHECK(cudaMalloc(&cudaParticles[0], cudaParticlesSize)); //TODO: track memory usage.
    CUDA_EXCEPTION_CHECK(cudaMalloc(&cudaParticles[1], cudaParticlesSize)); //TODO: track memory usage.
    cudaSiteTypesSize=numberSites*sizeof(uint8_t);
    CUDA_EXCEPTION_CHECK(cudaMalloc(&cudaSiteTypes, cudaSiteTypesSize)); //TODO: track memory usage.
}

void CudaByteLattice::deallocateCudaMemory()
throw(CUDAException)
{
    // If we have any allocated device memory, free it.
    if (cudaParticles[0] != NULL)
    {
        CUDA_EXCEPTION_CHECK(cudaFree(cudaParticles[0])); //TODO: track memory usage.
        cudaParticles[0] = NULL;
    }
    if (cudaParticles[1] != NULL)
    {
        CUDA_EXCEPTION_CHECK(cudaFree(cudaParticles[1])); //TODO: track memory usage.
        cudaParticles[1] = NULL;
    }
    cudaParticlesSize = 0;
    if (cudaSiteTypes != NULL)
    {
        CUDA_EXCEPTION_CHECK(cudaFree(cudaSiteTypes)); //TODO: track memory usage.
        cudaSiteTypes = NULL;
        cudaSiteTypesSize = 0;
    }
}

void CudaByteLattice::copyToGPU()
throw(CUDAException)
{
	if (!isGPUMemorySynched)
	{
		CUDA_EXCEPTION_CHECK(cudaMemcpy(cudaParticles[cudaParticlesCurrent], particles, cudaParticlesSize, cudaMemcpyHostToDevice));
        CUDA_EXCEPTION_CHECK(cudaMemcpy(cudaSiteTypes, siteTypes, cudaSiteTypesSize, cudaMemcpyHostToDevice));
		isGPUMemorySynched = true;
	}
}

void CudaByteLattice::copyFromGPU()
throw(CUDAException)
{
	CUDA_EXCEPTION_CHECK(cudaMemcpy(particles, cudaParticles[cudaParticlesCurrent], cudaParticlesSize, cudaMemcpyDeviceToHost));
    CUDA_EXCEPTION_CHECK(cudaMemcpy(siteTypes, cudaSiteTypes, cudaSiteTypesSize, cudaMemcpyDeviceToHost));
	isGPUMemorySynched = true;
}

void * CudaByteLattice::getGPUMemorySrc()
{
    return cudaParticles[cudaParticlesCurrent];
}

void * CudaByteLattice::getGPUMemoryDest()
{
    return cudaParticles[cudaParticlesCurrent==0?1:0];
}

void CudaByteLattice::swapSrcDest()
{
    cudaParticlesCurrent = cudaParticlesCurrent==0?1:0;
}

void * CudaByteLattice::getGPUMemorySiteTypes()
{
    return cudaSiteTypes;
}

void CudaByteLattice::setSiteType(lattice_size_t x, lattice_size_t y, lattice_size_t z, site_t site) throw(InvalidSiteException)
{
    ByteLattice::setSiteType(x,y,z,site);
    isGPUMemorySynched = false;
}

void CudaByteLattice::addParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, particle_t particle) throw(InvalidSiteException,InvalidParticleException)
{
    ByteLattice::addParticle(x,y,z,particle);
	isGPUMemorySynched = false;
}

void CudaByteLattice::removeParticles(lattice_size_t x,lattice_size_t y,lattice_size_t z) throw(InvalidSiteException)
{
    ByteLattice::removeParticles(x,y,z);
    isGPUMemorySynched = false;
}

void CudaByteLattice::setSiteType(lattice_size_t index, site_t site) throw(InvalidSiteException)
{
    ByteLattice::setSiteType(index,site);
    isGPUMemorySynched = false;
}

void CudaByteLattice::addParticle(lattice_size_t index, particle_t particle) throw(InvalidSiteException,InvalidParticleException)
{
    ByteLattice::addParticle(index,particle);
	isGPUMemorySynched = false;
}

void CudaByteLattice::removeParticles(lattice_size_t index) throw(InvalidSiteException)
{
    ByteLattice::removeParticles(index);
    isGPUMemorySynched = false;
}

void CudaByteLattice::removeAllParticles()
{
    ByteLattice::removeAllParticles();
	isGPUMemorySynched = false;
}

void CudaByteLattice::deserializeParticlesFrom(const void* srcBuffer, size_t bufferSize, SerializationDataOrder dataOrdering, bool inflate)
{
    ByteLattice::deserializeParticlesFrom(srcBuffer, bufferSize, dataOrdering, inflate);
    isGPUMemorySynched = false;
}

}
}
