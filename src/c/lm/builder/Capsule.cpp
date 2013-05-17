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
 * Author(s): Elijah Roberts
 */

#include "lm/builder/Shape.h"
#include "lm/builder/Sphere.h"
#include "lm/builder/Capsule.h"
#include "lm/rdme/Lattice.h"

namespace lm {
namespace builder {

Capsule::Capsule(point p1, point p2, si_dist_t radius, site_t type)
:Shape(CAPSULE,Capsule::calcBoundingBox(p1,p2,radius), type),p1(p1),p2(p2),radius(radius),length(p1.distance(p2)+radius+radius)
{
}

bounding_box Capsule::calcBoundingBox(point p1, point p2, si_dist_t radius)
{
	bounding_box bbox;
	bbox.min.x = (p1.x<p2.x)?(p1.x-radius):(p2.x-radius);
	bbox.max.x = (p1.x>p2.x)?(p1.x+radius):(p2.x+radius);
	bbox.min.y = (p1.y<p2.y)?(p1.y-radius):(p2.y-radius);
	bbox.max.y = (p1.y>p2.y)?(p1.y+radius):(p2.y+radius);
	bbox.min.z = (p1.z<p2.z)?(p1.z-radius):(p2.z-radius);
	bbox.max.z = (p1.z>p2.z)?(p1.z+radius):(p2.z+radius);
	return bbox;
}

Capsule::~Capsule()
{
}

bool Capsule::intersects(Shape * query)
{
    if (!boundingBoxesIntersect(query)) return false;

    if (query->getShapeType() == SPHERE)
    {
    	return true;
    }
    return false;
}

bool Capsule::contains(point query)
{
    // Transform the point as if the capsule was oriented in the +z direction with p1 at 0,0,r and p2 at 0,0,l-r.

    // Assume for now that the capsule is oriented with its long axis along the z axis.

    // Translate the sphere in the x-y plane.
	query.x -= p1.x;
	query.y -= p1.y;
	query.z -= (p1.z-radius);

    // See if the sphere is closer to the lower cap, the cylinder, or the upper cap.
    if (query.z < radius)
    {
        if (query.x*query.x+query.y*query.y+(query.z-(radius))*(query.z-(radius)) > (radius)*(radius)) return false;
        return true;
    }
    else if (query.z > length-radius)
    {
        if (query.x*query.x+query.y*query.y+(query.z-(length-radius))*(query.z-(length-radius)) > (radius)*(radius)) return false;
        return true;
    }
    else
    {
        // Make sure the sphere does not fall outside the cylinder.
        if (query.x*query.x+query.y*query.y > (radius)*(radius)) return false;
        return true;
    }
}

bool Capsule::contains(Shape * query)
{
	if (!boundingBoxesIntersect(query)) return false;


    if (query->getShapeType() == SPHERE)
    {
        Sphere * querySphere = (Sphere *)query;

        // Transform the sphere as if the capsule was oriented in the +z direction with p1 at 0,0,r and p2 at 0,0,l-r.
        point tc = querySphere->getCenter();
        si_dist_t tr=querySphere->getRadius();

        // Assume for now that the capsule is oriented with its long axis along the z axis.

        // Translate the sphere in the x-y plane.
        tc.x -= p1.x;
        tc.y -= p1.y;
        tc.z -= (p1.z-radius);

        // See if the sphere is closer to the lower cap, the cylinder, or the upper cap.
        if (tc.z < radius)
        {
            if (tc.x*tc.x+tc.y*tc.y+(tc.z-(radius))*(tc.z-(radius)) > (radius-tr)*(radius-tr)) return false;
            return true;
        }
        else if (tc.z > length-radius)
        {
            if (tc.x*tc.x+tc.y*tc.y+(tc.z-(length-radius))*(tc.z-(length-radius)) > (radius-tr)*(radius-tr)) return false;
            return true;
        }
        else
        {
            // Make sure the sphere does not fall outside the cylinder.
            if (tc.x*tc.x+tc.y*tc.y > (radius-tr)*(radius-tr)) return false;
            return true;
        }
    }
    return false;
}

double Capsule::getVolume()
{
    return ((4.0/3.0)*PI*radius*radius*radius)+(PI*radius*radius*(length-radius-radius));
}


}
}
