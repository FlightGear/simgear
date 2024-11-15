// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief osg::Operation to initialize the OpenVG context used for path rendering
 */

#include "VGInitOperation.hxx"
#include <vg/openvg.h>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  VGInitOperation::VGInitOperation():
    GraphicsOperation("canvas::VGInit", false)
  {

  }


  //----------------------------------------------------------------------------
  void VGInitOperation::operator()(osg::GraphicsContext* context)
  {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    // ATTENTION: If using another OpenVG implementation ensure it doesn't
    //            change any OpenGL state!
    vgCreateContextSH(vp[2], vp[3]);
  }

  //----------------------------------------------------------------------------
  void vgShutdown()
  {
    vgDestroyContextSH();
  }

} // namespace canvas
} // namespace simgear
