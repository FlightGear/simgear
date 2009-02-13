/* -*-c++-*-
 *
 * Copyright (C) 2009 Tim Moore
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef SIMGEAR_PRIMITIVEUTILS_HXX
#define SIMGEAR_PRIMITIVEUTILS_HXX 1

#include <osg/Vec3>
#include <osg/Drawable>

namespace simgear
{
struct Primitive
{
    int numVerts;
    osg::Vec3 vertices[4];
};

/**
 * Given a drawable and a primitive index (as returned from OSG
 * intersection queries), get the coordinates of the primitives
 * vertices.
 */
Primitive getPrimitive(osg::Drawable* drawable, unsigned primitiveIndex);
}
#endif
