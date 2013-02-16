// osg::Operation to initialize the OpenVG context used for path rendering
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef CANVAS_VG_INITOPERATION_HXX_
#define CANVAS_VG_INITOPERATION_HXX_

#include <osg/GraphicsThread>

namespace simgear
{
namespace canvas
{

  /**
   * Deferred graphics operation to setup OpenVG which needs a valid OpenGL
   * context. Pass to osg::GraphicsContext::add and ensure it's executed before
   * doing any path rendering
   */
  class VGInitOperation:
    public osg::GraphicsOperation
  {
    public:

      VGInitOperation();
      virtual void operator()(osg::GraphicsContext* context);
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_VG_INITOPERATION_HXX_ */
