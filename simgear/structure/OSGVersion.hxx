// OSGVersion.hxx - transform OpenSceneGraph version to something useful
//
// Copyright (C) 2008  Tim Moore timoore@redhat.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
// Boston, MA  02110-1301, USA.

#ifndef SIMGEAR_OSGVERSION_HXX
#define SIMGEAR_OSGVERSION_HXX 1
#include <osg/Version>
#define SG_OSG_VERSION \
    ((OPENSCENEGRAPH_MAJOR_VERSION*10000)\
     + (OPENSCENEGRAPH_MINOR_VERSION*1000) + OPENSCENEGRAPH_PATCH_VERSION)

#define SG_OSG_VERSION_GREATER_EQUAL(MAJOR, MINOR, PATCH) \
  ((OPENSCENEGRAPH_MAJOR_VERSION > (MAJOR)) ||            \
   (OPENSCENEGRAPH_MAJOR_VERSION == (MAJOR) &&            \
    (OPENSCENEGRAPH_MINOR_VERSION > (MINOR) ||            \
     (OPENSCENEGRAPH_MINOR_VERSION == (MINOR) &&          \
      OPENSCENEGRAPH_PATCH_VERSION >= (PATCH)))))

#define SG_OSG_VERSION_GREATER_THAN(MAJOR, MINOR, PATCH)  \
  ((OPENSCENEGRAPH_MAJOR_VERSION > (MAJOR)) ||            \
   (OPENSCENEGRAPH_MAJOR_VERSION == (MAJOR) &&            \
    (OPENSCENEGRAPH_MINOR_VERSION > (MINOR) ||            \
     (OPENSCENEGRAPH_MINOR_VERSION == (MINOR) &&          \
      OPENSCENEGRAPH_PATCH_VERSION > (PATCH)))))

#define SG_OSG_VERSION_LESS_THAN(MAJOR, MINOR, PATCH)  \
  (!SG_OSG_VERSION_GREATER_EQUAL(MAJOR, MINOR, PATCH))

#define SG_OSG_VERSION_LESS_EQUAL(MAJOR, MINOR, PATCH)  \
  (!SG_OSG_VERSION_GREATER_THAN(MAJOR, MINOR, PATCH))

#define SG_OSG_MIN_VERSION_REQUIRED(MAJOR, MINOR, PATCH) \
  SG_OSG_VERSION_GREATER_EQUAL(MAJOR, MINOR, PATCH)

#endif
