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

#include <map>
#include <vector>
#include <climits>
#include <cstring>
#include "lm/Types.h"
#include "lm/Exceptions.h"
#include "lm/rdme/Lattice.h"
#include "lm/rdme/ByteLattice.h"

namespace lm {
namespace rdme {

site_t ByteLattice::getMaxSiteType() const {return 255;}
particle_t ByteLattice::getMaxParticle() const {return 255;}
site_size_t ByteLattice::getMaxOccupancy() const {return PARTICLES_PER_WORD*wordsPerSite;}

ByteLattice::ByteLattice(lattice_coord_t size, si_dist_t spacing, uint particlesPerSite)
throw(std::bad_alloc,InvalidArgException,Exception)
:Lattice(size,spacing),wordsPerSite(particlesPerSite/PARTICLES_PER_WORD),particles(NULL),siteTypes(NULL)
{
    // Make sure that the memory layout is compatible with our CUDA assumptions.
    if (sizeof(uint8_t)*CHAR_BIT != 8) throw Exception("a byte lattice can only be created on architectures with an 8-bit char type.");
    if (sizeof(uint32_t)*CHAR_BIT != 32) throw Exception("a byte lattice can only be created on architectures with a 32-bit int type.");
    if (wordsPerSite*PARTICLES_PER_WORD != particlesPerSite) throw InvalidArgException("particlesPerSite", "must be evenly divisible by the number of particles per word");
    allocateMemory();
}

ByteLattice::ByteLattice(lattice_size_t xSize, lattice_size_t ySize, lattice_size_t zSize, si_dist_t spacing, uint particlesPerSite)
throw(std::bad_alloc,InvalidArgException,Exception)
:Lattice(xSize,ySize,zSize,spacing),wordsPerSite(particlesPerSite/PARTICLES_PER_WORD),particles(NULL),siteTypes(NULL)
{
    if (sizeof(uint8_t)*CHAR_BIT != 8) throw Exception("a byte lattice can only be created on architectures with an 8-bit char type.");
    if (sizeof(uint32_t)*CHAR_BIT != 32) throw Exception("a byte lattice can only be created on architectures with a 32-bit int type.");
    if (wordsPerSite*PARTICLES_PER_WORD != particlesPerSite) throw InvalidArgException("particlesPerSite", "must be evenly divisible by the number of particles per word");
    allocateMemory();
}

ByteLattice::~ByteLattice() //throw(std::bad_alloc)
{
    deallocateMemory();
}

void ByteLattice::getNeighboringSites(lattice_size_t index, lattice_size_t * neighboringIndices)
{
	lattice_size_t z = index/(size.x*size.y);
	lattice_size_t xy = index-(z*size.x*size.y);
	lattice_size_t y = xy/size.x;
	lattice_size_t x = xy-(y*size.x);

	lattice_size_t xySize = size.x*size.y;
	neighboringIndices[0] = (x>0)?(index-1):(index-1+size.x);
	neighboringIndices[1] = (x<(size.x-1))?(index+1):(index+1-size.x);
	neighboringIndices[2] = (y>0)?(index-size.x):(index-size.x+xySize);
	neighboringIndices[3] = (y<(size.y-1))?(index+size.x):(index+size.x-xySize);
	neighboringIndices[4] = (z>0)?(index-xySize):(index-xySize+numberSites);
	neighboringIndices[5] = (z<(size.z-1))?(index+xySize):(index+xySize-numberSites);
}

void ByteLattice::allocateMemory()
throw(std::bad_alloc)
{
    // Allocate the data for the particles.
    particles = new uint32_t[numberSites*wordsPerSite];
    memset(particles, 0, numberSites*wordsPerSite*sizeof(uint32_t));

    // Allocate the data for the site types.
    siteTypes = new uint8_t[numberSites];
    memset(siteTypes, 0, numberSites*sizeof(uint8_t));
}

void ByteLattice::deallocateMemory()
throw(std::bad_alloc)
{
    // Free any allocated memory.
    if (particles != NULL)
    {
        delete[] particles;
        particles = NULL;
    }
    if (siteTypes != NULL)
    {
        delete[] siteTypes;
        siteTypes = NULL;
    }
}

site_t ByteLattice::getSiteType(lattice_size_t x, lattice_size_t y, lattice_size_t z)
const throw(InvalidSiteException)
{
	// Make sure the arguments are valid.
	if (x >= size.x || y >= size.y || z >= size.z)
		throw InvalidSiteException(x,y,z);
	lattice_size_t latticeIndex = x+(y*size.x)+(z*size.x*size.y);
	
	return (site_t)siteTypes[latticeIndex];
}

site_t ByteLattice::getSiteType(lattice_size_t index)
const throw(InvalidSiteException)
{
	// Make sure the arguments are valid.
	if (index >= numberSites)
		throw InvalidSiteException(index);
	return (site_t)siteTypes[index];
}

void ByteLattice::setSiteType(lattice_size_t x, lattice_size_t y, lattice_size_t z, site_t siteType)
throw(InvalidSiteException)
{
    // Make sure the arguments are valid.
    if (x >= size.x || y >= size.y || z >= size.z)
        throw InvalidSiteException(x,y,z);
    lattice_size_t latticeIndex = x+(y*size.x)+(z*size.x*size.y);

    siteTypes[latticeIndex] = (uint8_t)siteType;
}

void ByteLattice::setSiteType(lattice_size_t index, site_t siteType)
throw(InvalidSiteException)
{
    // Make sure the arguments are valid.
	if (index >= numberSites)
		throw InvalidSiteException(index);
    siteTypes[index] = (uint8_t)siteType;
}

site_size_t ByteLattice::getOccupancy(lattice_size_t x, lattice_size_t y, lattice_size_t z) const throw(InvalidSiteException)
{
    // Make sure the arguments are valid.
    if (x >= size.x || y >= size.y || z >= size.z)
        throw InvalidSiteException(x,y,z);
    lattice_size_t latticeIndex = x+(y*size.x)+(z*size.x*size.y);
	
	// Count the number of particles at the site.
    site_size_t occupancy = 0;
	for (uint wi=0; wi<wordsPerSite; wi++)
	{
        uint8_t * byteParticles = (uint8_t *)(&particles[latticeIndex]);
        for (uint pi=0; pi<PARTICLES_PER_WORD; pi++, occupancy++)
        {
            if (byteParticles[pi] == 0) return occupancy;
        }
        latticeIndex += numberSites;
	}
	return occupancy;
}

site_size_t ByteLattice::getOccupancy(lattice_size_t index) const throw(InvalidSiteException)
{
    // Make sure the arguments are valid.
	if (index >= numberSites)
		throw InvalidSiteException(index);

	// Count the number of particles at the site.
    site_size_t occupancy = 0;
	for (uint wi=0; wi<wordsPerSite; wi++)
	{
        uint8_t * byteParticles = (uint8_t *)(&particles[index]);
        for (uint pi=0; pi<PARTICLES_PER_WORD; pi++, occupancy++)
        {
            if (byteParticles[pi] == 0) return occupancy;
        }
        index += numberSites;
	}
	return occupancy;
}

/**
 * Gets the current state of a give site in the lattice.
 * 
 * @param x	The zero based x index of the site to retrieve.
 * @param y	The zero based y index of the site to retrieve.
 * @param z	The zero based z index of the site to retrieve.
 * @return	The value in the lattice at the specified site.
 */
particle_t ByteLattice::getParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, site_size_t particleIndex) const
throw(InvalidSiteException,InvalidParticleException)
{
	// Make sure the arguments are valid.
    if (x >= size.x || y >= size.y || z >= size.z)
        throw InvalidSiteException(x,y,z);
	if (particleIndex >= getMaxOccupancy())
		throw InvalidParticleException(particleIndex);
    lattice_size_t latticeIndex = x+(y*size.x)+(z*size.x*size.y);
	
    // Figure out which word the particle is in.
    uint word = particleIndex/PARTICLES_PER_WORD;
    latticeIndex += numberSites*word;
    uint byteIndex = particleIndex%PARTICLES_PER_WORD;

	// Get the value.
    return ((uint8_t *)(&particles[latticeIndex]))[byteIndex];
}

particle_t ByteLattice::getParticle(lattice_size_t index, site_size_t particleIndex) const
throw(InvalidSiteException,InvalidParticleException)
{
	// Make sure the arguments are valid.
	if (index >= numberSites)
		throw InvalidSiteException(index);
	if (particleIndex >= getMaxOccupancy())
		throw InvalidParticleException(particleIndex);

    // Figure out which word the particle is in.
    uint word = particleIndex/PARTICLES_PER_WORD;
    index += numberSites*word;
    uint byteIndex = particleIndex%PARTICLES_PER_WORD;

	// Get the value.
    return ((uint8_t *)(&particles[index]))[byteIndex];
}

/**
 * Sets the current state of a give site in the lattice.
 * 
 * @param x	The zero based x index of the site to set.
 * @param y	The zero based y index of the site to set.
 * @param z	The zero based z index of the site to set.
 * @param 
 */
void ByteLattice::addParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, particle_t particle)
throw(InvalidSiteException,InvalidParticleException)
{
	// Make sure the arguments are valid.
    if (x >= size.x || y >= size.y || z >= size.z)
        throw InvalidSiteException(x,y,z);
    lattice_size_t latticeIndex = x+(y*size.x)+(z*size.x*size.y);

    // Go through the particles at the site.
    for (uint wi=0; wi<wordsPerSite; wi++)
    {
        uint8_t * byteParticles = (uint8_t *)(&particles[latticeIndex]);
        for (uint pi=0; pi<PARTICLES_PER_WORD; pi++)
        {
            if (byteParticles[pi] == 0)
            {
                byteParticles[pi] = (uint8_t)particle;
                return;
            }
        }
        latticeIndex += numberSites;
    }

    // The site must have been full.
    throw InvalidParticleException(getMaxOccupancy());
}

void ByteLattice::addParticle(lattice_size_t index, particle_t particle)
throw(InvalidSiteException,InvalidParticleException)
{
	// Make sure the arguments are valid.
	if (index >= numberSites)
		throw InvalidSiteException(index);

    // Go through the particles at the site.
    for (uint wi=0; wi<wordsPerSite; wi++)
    {
        uint8_t * byteParticles = (uint8_t *)(&particles[index]);
        for (uint pi=0; pi<PARTICLES_PER_WORD; pi++)
        {
            if (byteParticles[pi] == 0)
            {
                byteParticles[pi] = (uint8_t)particle;
                return;
            }
        }
        index += numberSites;
    }

    // The site must have been full.
    throw InvalidParticleException(getMaxOccupancy());
}

void ByteLattice::removeParticles(lattice_size_t x,lattice_size_t y,lattice_size_t z)
throw(InvalidSiteException)
{
	// Make sure the arguments are valid.
    if (x >= size.x || y >= size.y || z >= size.z)
        throw InvalidSiteException(x,y,z);
    lattice_size_t latticeIndex = x+(y*size.x)+(z*size.x*size.y);
	
    // Reset all of the words for this site.
    for (uint wi=0; wi<wordsPerSite; wi++)
    {
        particles[latticeIndex] = 0;
        latticeIndex += numberSites;
    }
}

void ByteLattice::removeParticles(lattice_size_t index)
throw(InvalidSiteException)
{
	// Make sure the arguments are valid.
	if (index >= numberSites)
		throw InvalidSiteException(index);

    // Reset all of the words for this site.
    for (uint wi=0; wi<wordsPerSite; wi++)
    {
        particles[index] = 0;
        index += numberSites;
    }
}

void ByteLattice::removeAllParticles()
{
    memset(particles, 0, numberSites*wordsPerSite*sizeof(uint32_t));
}

void ByteLattice::nativeSerialize(void * destBuffer, void * latticePtr, size_t bufferSize)
{
    ByteLattice * lattice = (ByteLattice *)latticePtr;
    if (bufferSize != lattice->size.x*lattice->size.y*lattice->size.z*lattice->getMaxOccupancy()) throw lm::InvalidArgException("bufferSize", "the buffer size was not equal to the lattice data size");
    memcpy(destBuffer, lattice->particles, bufferSize);
}

void ByteLattice::copyNativeToRowMajorByte(void * destBuffer, void * sourceBuffer, lattice_size_t xSize, lattice_size_t ySize, lattice_size_t zSize, uint particlesPerSite, size_t bufferSize)
{
    if (bufferSize != xSize*ySize*zSize*particlesPerSite) throw lm::InvalidArgException("bufferSize", "the buffer size was not equal to the lattice data size");
    uint wordsPerSite = particlesPerSite/PARTICLES_PER_WORD;
    if (wordsPerSite*PARTICLES_PER_WORD != particlesPerSite) throw InvalidArgException("particlesPerSite", "must be evenly divisible by the number of particles per word");

    // Cast the buffers as appropriate.
    uint32_t * sourceParticles = (uint32_t *)sourceBuffer;
    uint8_t * destParticles = (uint8_t *)destBuffer;


    // Walk through the source buffer and copy the first word of particles.
    uint latticeIndex=0;
    for (uint w=0; w<wordsPerSite; w++)
    {
        for (uint z=0; z<zSize; z++)
        {
            for (uint y=0; y<ySize; y++)
            {
                for (uint x=0; x<xSize; x++, latticeIndex++)
                {
                    uint8_t * byteParticles = (uint8_t *)(&sourceParticles[latticeIndex]);
                    uint destIndex = x*ySize*zSize*particlesPerSite + y*zSize*particlesPerSite + z*particlesPerSite;
                    destParticles[destIndex+w*PARTICLES_PER_WORD] = byteParticles[0];
                    destParticles[destIndex+w*PARTICLES_PER_WORD+1] = byteParticles[1];
                    destParticles[destIndex+w*PARTICLES_PER_WORD+2] = byteParticles[2];
                    destParticles[destIndex+w*PARTICLES_PER_WORD+3] = byteParticles[3];
                }
            }
        }
    }
}

void ByteLattice::copyRowMajorByteToNative(void * destBuffer, void * sourceBuffer, lattice_size_t xSize, lattice_size_t ySize, lattice_size_t zSize, uint particlesPerSite, size_t bufferSize)
{
    if (bufferSize != xSize*ySize*zSize*particlesPerSite) throw lm::InvalidArgException("bufferSize", "the buffer size was not equal to the lattice data size");
    uint wordsPerSite = particlesPerSite/PARTICLES_PER_WORD;
    if (wordsPerSite*PARTICLES_PER_WORD != particlesPerSite) throw InvalidArgException("particlesPerSite", "must be evenly divisible by the number of particles per word");

    // Cast the buffers as appropriate.
    uint32_t * destParticles = (uint32_t *)destBuffer;
    uint8_t * sourceParticles = (uint8_t *)sourceBuffer;

    // Walk through the dest buffer and copy into it the particles one word at a time.
    uint destIndex=0;
    for (uint w=0; w<wordsPerSite; w++)
    {
        for (uint z=0; z<zSize; z++)
        {
            for (uint y=0; y<ySize; y++)
            {
                for (uint x=0; x<xSize; x++, destIndex++)
                {
                    uint8_t * destByteParticles = (uint8_t *)(&destParticles[destIndex]);
                    uint sourceIndex = x*ySize*zSize*particlesPerSite + y*zSize*particlesPerSite + z*particlesPerSite + w*PARTICLES_PER_WORD;
                    destByteParticles[0] = sourceParticles[sourceIndex];
                    destByteParticles[1] = sourceParticles[sourceIndex+1];
                    destByteParticles[2] = sourceParticles[sourceIndex+2];
                    destByteParticles[3] = sourceParticles[sourceIndex+3];
                }
            }
        }
    }
}

void ByteLattice::copySitesRowMajorByteToNative(void * destBuffer, void * sourceBuffer, lattice_size_t xSize, lattice_size_t ySize, lattice_size_t zSize, size_t bufferSize)
{
    if (bufferSize != xSize*ySize*zSize) throw lm::InvalidArgException("bufferSize", "the buffer size was not equal to the lattice data size");

    // Cast the buffers as appropriate.
    uint8_t * destSites = (uint8_t *)destBuffer;
    uint8_t * sourceSites = (uint8_t *)sourceBuffer;

    // Walk through the dest buffer and copy into it sites.
    uint destIndex=0;
	for (uint z=0; z<zSize; z++)
	{
		for (uint y=0; y<ySize; y++)
		{
			for (uint x=0; x<xSize; x++, destIndex++)
			{
				uint sourceIndex = x*ySize*zSize + y*zSize + z;
				destSites[destIndex] = destSites[sourceIndex];
			}
		}
	}
}

void ByteLattice::setFromRowMajorByteData(void * buffer, size_t bufferSize)
{
    copyRowMajorByteToNative(particles, buffer, size.x, size.y, size.z, wordsPerSite*PARTICLES_PER_WORD, bufferSize);
}

void ByteLattice::setSitesFromRowMajorByteData(void * buffer, size_t bufferSize)
{
    copySitesRowMajorByteToNative(siteTypes, buffer, size.x, size.y, size.z, bufferSize);
}

std::map<particle_t,uint> ByteLattice::getParticleCounts()
{
    std::map<particle_t,uint> particleCountMap;
    for (lattice_size_t index=0; index<numberSites*wordsPerSite; index++)
    {
        uint8_t * byteParticles = (uint8_t *)(&particles[index]);
        for (uint b=0; b<PARTICLES_PER_WORD; b++)
        {
            // Count the particle.
            particle_t particle = byteParticles[b];
            if (particle > 0)
            {
                if (particleCountMap.count(particle) == 0)
                    particleCountMap[particle] = 1;
                else
                    particleCountMap[particle]++;
            }
            // Otherwise, if the particle position is empty, move to the next site.
            else if (particle == 0)
            {
                break;
            }
        }
    }
    return particleCountMap;
}

std::vector<particle_loc_t> ByteLattice::findParticles(particle_t minParticleType, particle_t maxParticleType)
{
    std::vector<particle_loc_t> ret;

    // Walk through the lattice.
    uint index=0;
    for (uint w=0; w<wordsPerSite; w++)
    {
        for (uint z=0; z<size.z; z++)
        {
            for (uint y=0; y<size.y; y++)
            {
                for (uint x=0; x<size.x; x++, index++)
                {
                    uint8_t * byteParticles = (uint8_t *)(&particles[index]);
                    for (uint b=0; b<PARTICLES_PER_WORD; b++)
                    {
                        if (byteParticles[b] >= minParticleType && byteParticles[b] <= maxParticleType)
                            ret.push_back(particle_loc_t(byteParticles[b], x, y, z, w*PARTICLES_PER_WORD+b));
                    }
                }
            }
        }
    }
    return ret;
}

/*
bool ByteLattice::findParticle(lattice_particle_t particleToFind, lattice_size_t* x, lattice_size_t* y, lattice_size_t* z, uint* particleIndex)
{
	if (x == NULL || y == NULL || z == NULL || particleIndex == NULL)
		throw InvalidArgException("location pointer was NULL");

	lattice_size_t xySize = xSize*ySize;
	lattice_size_t numberSites = getNumberSites();
	for (lattice_size_t latticeIndex = 0; latticeIndex<numberSites; latticeIndex++)
		for (uint i=0, shift=0; i<maxParticlesPerSite; i++, shift+=bitsPerParticle)
		{
			// See if this is the particle we are looking for.
			lattice_particle_t particle = (lattice[latticeIndex]&(particleMask<<shift))>>shift;
			if (particle == particleToFind)
			{
				*x = latticeIndex%xSize;
				*y = (latticeIndex/xSize)%ySize;
				*z = latticeIndex/xySize;
				*particleIndex = i;
				return true;
			}
			
			// Otherwise, if the partiucle position is empty, move to the next site.
			else if (particle == 0)
			{
				break;
			}
		}
	return false;
}


bool ByteLattice::findNextParticle(lattice_particle_t particleToFind, lattice_size_t* x, lattice_size_t* y, lattice_size_t* z, uint* particleIndex)
{
	if (x == NULL || y == NULL || z == NULL || particleIndex == NULL)
		throw InvalidArgException("location pointer was NULL");

	lattice_size_t xySize = xSize*ySize;
	lattice_size_t numberSites = getNumberSites();
	uint firstParticleToCheck = (*particleIndex)+1;
	uint firstParticleToCheckShift = firstParticleToCheck*bitsPerParticle;
	for (lattice_size_t latticeIndex=*x+(*y*xSize)+(*z*xSize*ySize); latticeIndex<numberSites; latticeIndex++,firstParticleToCheck=0,firstParticleToCheckShift=0)
		for (uint i=firstParticleToCheck, shift=firstParticleToCheckShift; i<maxParticlesPerSite; i++, shift+=bitsPerParticle)
		{
			// See if this is the particle we are looking for.
			lattice_particle_t particle = (lattice[latticeIndex]&(particleMask<<shift))>>shift;
			if (particle == particleToFind)
			{
				*x = latticeIndex%xSize;
				*y = (latticeIndex/xSize)%ySize;
				*z = latticeIndex/xySize;
				*particleIndex = i;
				return true;
			}
			
			// Otherwise, if the particle position is empty, move to the next site.
			else if (particle == 0)
			{
				break;
			}
		}
	return false;
}

bool ByteLattice::findNearbyParticle(lattice_particle_t particleToFind, lattice_size_t xi, lattice_size_t yi, lattice_size_t zi, lattice_size_t* xo, lattice_size_t* yo, lattice_size_t* zo, uint* particleIndex)
{
	// Make sure the arguments are valid.
	if (xo == NULL || yo == NULL || zo == NULL || particleIndex == NULL)
		throw InvalidArgException("Output location pointer was NULL.");
	if (xi >= xSize || yi >= ySize || zi >= zSize)
		throw InvalidSiteException(xi,yi,zi);
	
	// Search for the particle in ever increasing concentric boxes around the initial site.
	for (intv d=0; d<=(intv)(xSize/2) || d<=(intv)(ySize/2) || d<=(intv)(zSize/2); d++)
	{
		//std::cout << "Depth: " << d << "\n";
		
		// Go through each z plane.
		for (intv z=(intv)zi-d; z<=(intv)zi+d; z++)
		{
			// Figure out the lattice z index.
			lattice_size_t zl = (lattice_size_t)(z<0)?(z+(intv)zSize):((z>=(intv)zSize)?(z-(intv)zSize):z);
			
			//std::cout << "Z: " << z << "," << zl << "\n";
			
			// Go through each y row.
			for (intv y=(intv)yi-d; y<=(intv)yi+d; y++)
			{
				// Figure out the lattice y index.
				lattice_size_t yl = (lattice_size_t)(y<0)?(y+(intv)ySize):((y>=(intv)ySize)?(y-(intv)ySize):y);
				lattice_size_t latticeIndex = (yl*zSize)+(zl*xSize*ySize);
				
				// Go through each x column.
				int searchFullRow = (z==((intv)zi-d)||z==((intv)zi+d)||y==((intv)yi-d)||y==((intv)yi+d));
				for (intv x=(intv)xi-d; x<=(intv)xi+d; x+=(searchFullRow?(1):(2*d)))
				{
					// Figure out the lattice x index.
					lattice_size_t xl = (lattice_size_t)(x<0)?(x+(intv)xSize):((x>=(intv)xSize)?(x-(intv)xSize):x);
					
					// Go through each particle at the site.
					for (uint i=0, shift=0; i<maxParticlesPerSite; i++, shift+=bitsPerParticle)
					{
						// See if this is the particle we are looking for.
						lattice_particle_t particle = (lattice[latticeIndex+xl]&(particleMask<<shift))>>shift;
						if (particle == particleToFind)
						{
							*xo = xl;
							*yo = yl;
							*zo = zl;
							*particleIndex = i;
							return true;
						}
						
						// Otherwise, if the particle position is empty, move to the next site.
						else if (particle == 0)
						{
							break;
						}
					}
				}
			}
		}
	}
	return false;
}

*/

}
}

/*
 * 		// Go through each z plane.
		for (uintv z=(d<=zi)?(zi-d):0; z<=((d<zSize-zi)?(zi+d):(zSize-1)); z++)
		{
			// Go through each y row.
			for (uintv y=(d<=yi)?(yi-d):0; y<=((d<ySize-yi)?(yi+d):(ySize-1)); y++)
			{
				// Figure out the starting index of this row.
				lattice_size_t latticeIndex = (y*zSize)+(z*xSize*ySize);
				
				// Go through each x column.
				int searchFullRow = (z==(zi-d)||z==(zi+d)||y==(yi-d)||y==(yi+d));
				for (uintv x=((d<=xi)?(xi-d):(searchFullRow?(0):(xi+d))); x<=((d<xSize-xi)?(xi+d):(xSize-1)); x+=(searchFullRow?(1):(2*d)))
				{
					// Go through each particle at the site.
					for (uint i=0, shift=0; i<maxParticlesPerSite; i++, shift+=bitsPerParticle)
					{
						// See if this is the particle we are looking for.
						lattice_particle_t particle = (lattice[latticeIndex+x]&(particleMask<<shift))>>shift;
						if (particle == particleToFind)
						{
							*xo = x;
							*yo = y;
							*zo = z;
							*particleIndex = i;
							return true;
						}
						
						// Otherwise, if the particle position is empty, move to the next site.
						else if (particle == 0)
						{
							break;
						}
					}
				}
			}
		}
 */
