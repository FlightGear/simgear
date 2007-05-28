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

#ifndef SG_ENLARGE_BOUNDING_BOX_HXX
#define SG_ENLARGE_BOUNDING_BOX_HXX

#include <osg/Drawable>

// Helper class to enlarge a bounding box of a drawable.
// Is usefull to enlarge the mounding box of single point lights that would
// be victim to small feature culling otherwise.
class SGEnlargeBoundingBox : public osg::Drawable::ComputeBoundingBoxCallback {
public:
  SGEnlargeBoundingBox(float offset = 0);
  SGEnlargeBoundingBox(const SGEnlargeBoundingBox& cb, const osg::CopyOp&);
  META_Object(osg, SGEnlargeBoundingBox);
  virtual osg::BoundingBox computeBound(const osg::Drawable& drawable) const;

private:
  float _offset;
};

#endif
