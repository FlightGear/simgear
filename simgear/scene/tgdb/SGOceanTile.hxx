/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich 
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

#ifndef _SG_OceanTile_HXX
#define _SG_OceanTile_HXX

#include <osg/Node>

class SGBucket;
class SGMaterialLib;

// Generate an ocean tile
osg::Node* SGOceanTile(const SGBucket& b, SGMaterialLib *matlib, int latPoints = 5, int lonPoints = 5);

#endif // _SG_OBJ_HXX
