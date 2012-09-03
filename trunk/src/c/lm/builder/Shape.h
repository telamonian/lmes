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

#ifndef LM_BUILDER_SHAPE_H_
#define LM_BUILDER_SHAPE_H_

#include <cmath>
#include "lm/Math.h"
#include "lm/Types.h"
#include "lm/rdme/Lattice.h"

namespace lm {

namespace rdme {
class Lattice;
}

namespace builder {

// Type to store a position in space.
struct point {
    point(si_dist_t x=0.0, si_dist_t y=0.0, si_dist_t z=0.0):x(x),y(y),z(z){}
    si_dist_t x;
    si_dist_t y;
    si_dist_t z;

    si_dist_t distanceSquared(const point & p2)
    {
        si_dist_t dx = p2.x - x;
        si_dist_t dy = p2.y - y;
        si_dist_t dz = p2.z - z;
        return (dx*dx + dy*dy + dz*dz);
    }

    si_dist_t distance(const point & p2) {return sqrt(distanceSquared(p2));}
};

struct bounding_box {
    bounding_box(si_dist_t x1=0.0, si_dist_t y1=0.0, si_dist_t z1=0.0, si_dist_t x2=0.0, si_dist_t y2=0.0, si_dist_t z2=0.0):min(x1,y1,z1),max(x2,y2,z2){}
    bounding_box(point min, point max):min(min),max(max){}
    point min, max;

    bounding_box joinWith(bounding_box j)
    {
        return bounding_box(::min(j.min.x,min.x),::min(j.min.y,min.y),::min(j.min.z,min.z),::max(j.max.x,max.x),::max(j.max.y,max.y),::max(j.max.z,max.z));
    }
};

struct vector {
    vector(si_dist_t x=0.0, si_dist_t y=0.0, si_dist_t z=0.0):x(x),y(y),z(z){}
    si_dist_t x;
    si_dist_t y;
    si_dist_t z;
};

class Shape
{
public:
    enum ShapeType
    {
       SPHERE           = 1,
       HEMISPHERE       = 2,
       CYLINDER         = 3,
       CAPSULE          = 4,
       CUBOID           = 5,
       CAPSULE_SHELL    = 6
    };

public:
    Shape(ShapeType shapeType, bounding_box boundingBox, site_t type);
    virtual ~Shape();
    virtual bool boundingBoxesIntersect(Shape * query);
    virtual bool intersects(Shape * query) = 0;
    virtual bool contains(point query) = 0;
    virtual bool contains(Shape * query) = 0;
    virtual bounding_box getBoundingBox() {return boundingBox;}
    virtual site_t getType() {return type;}
    virtual ShapeType getShapeType() {return shapeType;}
    virtual double getVolume() = 0;

    virtual void discretizeTo(lm::rdme::Lattice * lattice);

protected:
    virtual point minBounds(point p1, point p2) {return point(min(p1.x,p2.x),min(p1.y,p2.y),min(p1.z,p2.z));}
    virtual point maxBounds(point p1, point p2) {return point(max(p1.x,p2.x),max(p1.y,p2.y),max(p1.z,p2.z));}
    ShapeType shapeType;
    bounding_box boundingBox;
    site_t type;

    friend class LatticeBuilder;
};

}
}

#endif

