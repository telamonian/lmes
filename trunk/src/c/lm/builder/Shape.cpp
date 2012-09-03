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
#include "lm/rdme/Lattice.h"


namespace lm {
namespace builder {

Shape::Shape(ShapeType shapeType, bounding_box boundingBox, site_t type)
:shapeType(shapeType),boundingBox(boundingBox),type(type)
{
}

Shape::~Shape()
{
}

bool Shape::boundingBoxesIntersect(Shape * query)
{
    bool xOverlap = (query->boundingBox.max.x >= boundingBox.min.x && query->boundingBox.min.x <= boundingBox.max.x);
    bool yOverlap = (query->boundingBox.max.y >= boundingBox.min.y && query->boundingBox.min.y <= boundingBox.max.y);
    bool zOverlap = (query->boundingBox.max.z >= boundingBox.min.z && query->boundingBox.min.z <= boundingBox.max.z);
    return xOverlap && yOverlap && zOverlap;
}

void Shape::discretizeTo(lm::rdme::Lattice * lattice)
{
	// Get the starting and ending subvolumes coordinates.
	lattice_size_t x1 = (lattice_size_t)floor(boundingBox.min.x/lattice->getSpacing());
	lattice_size_t x2 = ((lattice_size_t)floor(boundingBox.max.x/lattice->getSpacing()))+1;
	lattice_size_t y1 = (lattice_size_t)floor(boundingBox.min.y/lattice->getSpacing());
	lattice_size_t y2 = ((lattice_size_t)floor(boundingBox.max.y/lattice->getSpacing()))+1;
	lattice_size_t z1 = (lattice_size_t)floor(boundingBox.min.z/lattice->getSpacing());
	lattice_size_t z2 = ((lattice_size_t)floor(boundingBox.max.z/lattice->getSpacing()))+1;

	// Go through each subvolume that contains a portion of this shape.
	for (lattice_size_t x = x1; x<=x2; x++)
		for (lattice_size_t y = y1; y<=y2; y++)
			for (lattice_size_t z = z1; z<=z2; z++)
			{
				// If the center of this subvolume is in the cuboid, mark it as part of the shape.
				point c((((double)x)+0.5)*lattice->getSpacing(), (((double)y)+0.5)*lattice->getSpacing(), (((double)z)+0.5)*lattice->getSpacing());
				if (contains(c)) lattice->setSiteType(x, y, z, type);
			}
}

}
}
