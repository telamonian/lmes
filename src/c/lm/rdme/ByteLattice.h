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

#ifndef LM_RDME_BYTELATTICE_H_
#define LM_RDME_BYTELATTICE_H_

#include <map>
#include "lm/Types.h"
#include "lm/rdme/Lattice.h"

namespace lm {
namespace rdme {

class ByteLattice : public Lattice
{
public:
    // Lattice limits.
    virtual site_t getMaxSiteType() const;
    virtual particle_t getMaxParticle() const;
    virtual site_size_t getMaxOccupancy() const;

public:
    ByteLattice(lattice_coord_t size, si_dist_t spacing, uint particlesPerSite) throw(std::bad_alloc,InvalidArgException,Exception);
    ByteLattice(lattice_size_t xSize, lattice_size_t ySize, lattice_size_t zSize, si_dist_t spacing, uint particlesPerSite) throw(std::bad_alloc,InvalidArgException,Exception);
	virtual ~ByteLattice() throw(std::bad_alloc);
	
    virtual void getNeighboringSites(lattice_size_t index, lattice_size_t * neighboringIndices, bool periodic);

	// Lattice site methods.
	virtual site_t getSiteType(lattice_size_t x, lattice_size_t y, lattice_size_t z) const throw(InvalidSiteException);
	virtual site_t getSiteType(lattice_size_t subvolume) const throw(InvalidSiteException);
	virtual void setSiteType(lattice_size_t x, lattice_size_t y, lattice_size_t z, site_t siteType) throw(InvalidSiteException);
	virtual void setSiteType(lattice_size_t subvolume, site_t site) throw(InvalidSiteException);
	
	// Particle methods.
	virtual site_size_t getOccupancy(lattice_size_t x, lattice_size_t y, lattice_size_t z) const throw(InvalidSiteException);
	virtual site_size_t getOccupancy(lattice_size_t subvolume) const throw(InvalidSiteException);
	virtual particle_t getParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, site_size_t particleIndex) const throw(InvalidSiteException,InvalidParticleException);
	virtual particle_t getParticle(lattice_size_t subvolume, site_size_t particleIndex) const throw(InvalidSiteException,InvalidParticleException);
	virtual void addParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, particle_t particle) throw(InvalidSiteException,InvalidParticleException);
	virtual void addParticle(lattice_size_t subvolume, particle_t particle) throw(InvalidSiteException,InvalidParticleException);
	virtual void removeParticles(lattice_size_t x,lattice_size_t y,lattice_size_t z) throw(InvalidSiteException);
    virtual void removeParticles(lattice_size_t subvolume) throw(InvalidSiteException);
    virtual void removeAllParticles();
	
	// Particle searching.
    /*virtual particle_loc_t findParticle(particle_t particle);
    virtual particle_loc_t findNextParticle(particle_loc_t previousParticle);
    virtual particle_loc_t findNearbyParticle(particle_loc_t particle)=0;
	*/

    virtual std::map<particle_t,uint> getParticleCounts();
    virtual std::vector<particle_loc_t> findParticles(particle_t minParticleType, particle_t maxParticleType);
	
    // Methods to set the data directly.
    virtual size_t serializeParticlesSize();
    virtual void serializeParticlesTo(void* destBuffer, size_t bufferSize, SerializationDataOrder dataOrdering);
    virtual void deserializeParticlesFrom(const void* srcBuffer, size_t bufferSize, SerializationDataOrder dataOrdering);
    virtual size_t serializeSitesSize();
    virtual void serializeSitesTo(void* destBuffer, size_t bufferSize, SerializationDataOrder dataOrdering);
    virtual void deserializeSitesFrom(const void* srcBuffer, size_t bufferSize, SerializationDataOrder dataOrdering);

protected:
	virtual void allocateMemory() throw(std::bad_alloc);
	virtual void deallocateMemory() throw(std::bad_alloc);

protected:
    uint wordsPerSite;
    uint32_t * particles;
    uint8_t * siteTypes;

private:
    static const uint PARTICLES_PER_WORD = 4;
};

}
}

#endif
