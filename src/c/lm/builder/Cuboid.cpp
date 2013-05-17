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

#include "lm/builder/Shape.h"
#include "lm/builder/Sphere.h"
#include "lm/builder/Cuboid.h"
#include "lm/rdme/Lattice.h"

namespace lm {
namespace builder {

Cuboid::Cuboid(point p1, point p2, site_t type)
:Shape(CUBOID,bounding_box(minBounds(p1,p2), maxBounds(p1,p2)), type),p1(minBounds(p1,p2)),p2(maxBounds(p1,p2))
{
}

Cuboid::~Cuboid()
{
}

bool Cuboid::intersects(Shape * query)
{
    if (!boundingBoxesIntersect(query)) return false;

    if (query->getShapeType() == SPHERE)
    {
        return false;
    }

    return false;
}

bool Cuboid::contains(point query)
{
	return (query.x >= p1.x && query.x <= p2.x && query.y >= p1.y && query.y <= p2.y && query.z >= p1.z && query.z <= p2.z);
}

bool Cuboid::contains(Shape * query)
{
    if (!boundingBoxesIntersect(query)) return false;

    if (query->getShapeType() == SPHERE)
    {
        Sphere * querySphere = (Sphere *)query;
        point c = querySphere->getCenter();
        si_dist_t r=querySphere->getRadius();

        if (c.x >= p1.x+r && c.x <= p2.x-r && c.y >= p1.y+r && c.y <= p2.y-r && c.z >= p1.z+r && c.z <= p2.z-r) return true;
        return false;
    }

    return false;
}

double Cuboid::getVolume()
{
    return fabs((p2.x-p1.x)*(p2.y-p1.y)*(p2.z-p1.z));
}

}
}
