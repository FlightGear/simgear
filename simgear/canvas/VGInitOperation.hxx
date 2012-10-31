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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef CANVAS_VG_INITOPERATION_HXX_
#define CANVAS_VG_INITOPERATION_HXX_

#include <vg/openvg.h>
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

      VGInitOperation():
        GraphicsOperation("canvas::VGInit", false)
      {}

      virtual void operator()(osg::GraphicsContext* context)
      {
        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);

        // ATTENTION: If using another OpenVG implementation ensure it doesn't
        //            change any OpenGL state!
        vgCreateContextSH(vp[2], vp[3]);
      }

  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_VG_INITOPERATION_HXX_ */
