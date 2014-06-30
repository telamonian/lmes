/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012 Roberts Group,
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

#ifndef LM_RDME_LATTICE_H_
#define LM_RDME_LATTICE_H_

//using namespace std; //TODO: workaround for cuda compiler bug, remove this line when fixed.
#include <iostream>
#include <vector>
#include <map>
#include "lm/Types.h"
#include "lm/Exceptions.h"

// Type to store a lattice index.
typedef uint32_t            lattice_size_t;

// Type to store a lattice coordinate.
struct lattice_coord_t {
    lattice_coord_t(lattice_size_t x=0, lattice_size_t y=0, lattice_size_t z=0):x(x),y(y),z(z){}
    lattice_size_t x;
    lattice_size_t y;
    lattice_size_t z;
};

// Type to store a particle index in a site.
typedef uint32_t            site_size_t;

// Type to store a lattice site type.
typedef uint32_t            site_t;

// Type to store a lattice particle type.
typedef uint32_t            particle_t;

// Type to store a particle and position.
struct particle_loc_t {
    particle_loc_t(particle_t p=0, lattice_size_t x=0, lattice_size_t y=0, lattice_size_t z=0, site_size_t index=0):p(p),x(x),y(y),z(z),index(index){}
    particle_t p;
    lattice_size_t x;
    lattice_size_t y;
    lattice_size_t z;
    site_size_t index;
};

namespace lm {
namespace rdme {

class InvalidSiteException : public InvalidArgException
{
public:
    InvalidSiteException(lattice_size_t x, lattice_size_t y, lattice_size_t z) : InvalidArgException("lattice site index") {}
    InvalidSiteException(lattice_size_t index) : InvalidArgException("lattice site index") {}
};

class InvalidParticleException : public InvalidArgException
{
public:
    InvalidParticleException(site_size_t particleIndex) : InvalidArgException("particleIndex", "Invalid particle index", particleIndex) {}
};

class Lattice
{
public:
    enum SerializationDataOrder {ROW_MAJOR=0, COLUMN_MAJOR=1, NATIVE_ORDER=2};

public:
    // Lattice limits.
    virtual site_t getMaxSiteType() const =0;
    virtual particle_t getMaxParticle() const =0;
    virtual site_size_t getMaxOccupancy() const =0;
	
public:
    Lattice(lattice_coord_t size, si_dist_t spacing);
	Lattice(lattice_size_t xSize, lattice_size_t ySize, lattice_size_t zSize, si_dist_t spacing);
	virtual ~Lattice();
	virtual lattice_coord_t getSize() const;
	virtual lattice_size_t getXSize() const;
	virtual lattice_size_t getYSize() const;
	virtual lattice_size_t getZSize() const;
	virtual lattice_size_t getNumberSites() const;
	virtual si_dist_t getSpacing() const;
	
	virtual void getNeighboringSites(lattice_size_t index, lattice_size_t * neighboringIndices)=0;

	// Lattice site methods.
	virtual site_t getSiteType(lattice_size_t x, lattice_size_t y, lattice_size_t z) const throw(InvalidSiteException)=0;
	virtual site_t getSiteType(lattice_size_t index) const throw(InvalidSiteException)=0;
	virtual void setSiteType(lattice_size_t x, lattice_size_t y, lattice_size_t z, site_t site) throw(InvalidSiteException)=0;
	virtual void setSiteType(lattice_size_t index, site_t site) throw(InvalidSiteException)=0;
	std::vector<lattice_coord_t> getNearbySites(lattice_size_t xc, lattice_size_t yc, lattice_size_t zc, uint minDistance, uint maxDistance);

	// Particle methods.
	virtual site_size_t getOccupancy(lattice_size_t x, lattice_size_t y, lattice_size_t z) const throw(InvalidSiteException)=0;
	virtual site_size_t getOccupancy(lattice_size_t index) const throw(InvalidSiteException)=0;
	virtual particle_t getParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, site_size_t particleIndex) const throw(InvalidSiteException,InvalidParticleException)=0;
	virtual particle_t getParticle(lattice_size_t index, site_size_t particleIndex) const throw(InvalidSiteException,InvalidParticleException)=0;
	virtual void addParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, particle_t particle) throw(InvalidSiteException,InvalidParticleException)=0;
	virtual void addParticle(lattice_size_t index, particle_t particle) throw(InvalidSiteException,InvalidParticleException)=0;
    virtual void removeParticles(lattice_size_t x,lattice_size_t y,lattice_size_t z) throw(InvalidSiteException)=0;
    virtual void removeParticles(lattice_size_t index) throw(InvalidSiteException)=0;
	virtual void removeAllParticles();
	
	/**
	 * Particle searching methods.
	 */

	// Search for a particle from the start of the lattice.
	/*virtual particle_loc_t findParticle(particle_t particle)=0;
	virtual particle_loc_t findNextParticle(particle_loc_t previousParticle)=0;
	
	// Search for a particle starting from a given location.
	virtual particle_loc_t findNearbyParticle(particle_loc_t particle)=0;
	*/
	
	// Counts all of the particles on the lattice.
	virtual std::map<particle_t,uint> getParticleCounts()=0;
	
	//Finds all of the particle of a given range of types.
	virtual std::vector<particle_loc_t> findParticles(particle_t minParticleType, particle_t maxParticleType)=0;

	virtual void print() const;

    // Methods to serialize the data.
    virtual size_t serializeParticlesSize()=0;
    virtual void serializeParticlesTo(void* destBuffer, size_t bufferSize, SerializationDataOrder dataOrdering)=0;
    virtual void deserializeParticlesFrom(const void* srcBuffer, size_t bufferSize, SerializationDataOrder dataOrdering)=0;
    virtual size_t serializeSitesSize()=0;
    virtual void serializeSitesTo(void* destBuffer, size_t bufferSize, SerializationDataOrder dataOrdering)=0;
    virtual void deserializeSitesFrom(const void* srcBuffer, size_t bufferSize, SerializationDataOrder dataOrdering)=0;

protected:
	lattice_coord_t size;
	lattice_size_t numberSites;
    si_dist_t spacing;
};


}
}

#endif
