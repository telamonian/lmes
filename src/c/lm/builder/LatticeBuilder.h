/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
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
 * Author(s): Andrew Magis, Elijah Roberts
 */

#ifndef LM_BUILDER_LATTICEBUILDER_H_
#define LM_BUILDER_LATTICEBUILDER_H_

#include <map>
#include <utility>
#include <vector>
#include "lm/Types.h"
#include "lm/builder/Shape.h"
#include "lm/rdme/Lattice.h"
#include "lm/rng/RandomGenerator.h"

using std::vector;
using lm::rng::RandomGenerator;

namespace lm {

namespace io {
class SpatialModel;
}

namespace rdme {
class Lattice;
}

namespace builder {

class LatticeBuilder
{
protected:
	struct ParticlePlacement
	{
    	particle_t particleType;
    	site_t siteType;
    	uint count;
    	ParticlePlacement(particle_t particleType, site_t siteType, uint count):particleType(particleType),siteType(siteType),count(count) {}
	};

public:
    LatticeBuilder(si_dist_t xLen, si_dist_t yLen, si_dist_t zLen, si_dist_t collisionGridSpacing, uint32_t seedTop, uint32_t seedBottom=0);
    virtual ~LatticeBuilder();
    virtual void addRegion(Shape * shape);
    virtual bool placeObject(Shape * shape);
    virtual void removeObject(Shape * s);
    virtual bool placeSphere(point center, si_dist_t radius, site_t type);
    virtual void removeSphere(point center, si_dist_t radius, site_t type);
    virtual uint placeRandomSphere(si_dist_t radius, site_t type, site_t region);
    virtual void placeRandomSpheres(uint count, si_dist_t radius, site_t type, site_t region);
    virtual void fillWithRandomSpheres(double volumeFraction, si_dist_t radius, site_t type, site_t region);
    virtual void getSpatialModel(lm::io::SpatialModel * spatialModel);

    virtual void addParticles(particle_t particleType, site_t siteType, uint count);

    virtual void discretizeTo(lm::rdme::Lattice * lattice, site_t obstacleSiteType, double fractionObstacleSitesOccupied);

protected:
    virtual void discretizeObstaclesTo(lm::rdme::Lattice * lattice, site_t obstacleSiteType, double fractionObstacleSitesOccupied);

protected:
    std::vector<site_t> definedRegions;
    std::map<site_t,std::vector<Shape *> > regionShapes;
    std::map<site_t,bounding_box> regionBounds;
    std::vector<Shape *> objects;
    std::vector<ParticlePlacement> particles;
    RandomGenerator * rng;
    si_dist_t xLen, yLen, zLen;
    Shape *** collisionGrid;
    uint * collisionGridSize;
    uint * collisionGridOccupancy;
    si_dist_t collisionGridSpacing;
    double recipCollisionGridSpacing;
    uint collisionGridXSize, collisionGridYSize, collisionGridZSize;
};

}
}

#endif
