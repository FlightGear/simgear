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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "SGEnlargeBoundingBox.hxx"

#include <osg/Drawable>
#include <osg/Version>

SGEnlargeBoundingBox::SGEnlargeBoundingBox(float offset) :
  _offset(offset)
{
}

SGEnlargeBoundingBox::SGEnlargeBoundingBox(const SGEnlargeBoundingBox& cb,
                                           const osg::CopyOp& copyOp) :
  osg::Drawable::ComputeBoundingBoxCallback(cb, copyOp),
  _offset(cb._offset)
{
}

osg::BoundingBox
SGEnlargeBoundingBox::computeBound(const osg::Drawable& drawable) const
{
  osg::BoundingBox bound = drawable.computeBoundingBox();

  if (!bound.valid())
    return bound;
  return osg::BoundingBox(bound._min - osg::Vec3(_offset, _offset, _offset),
                          bound._max + osg::Vec3(_offset, _offset, _offset));
}
