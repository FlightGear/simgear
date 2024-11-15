// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief osg::Operation to initialize the OpenVG context used for path rendering
 */

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

  /// No need to defer the destruction of the OpenVG context with an
  /// osg::GraphicsOperation. This function can be called directly.
  void vgShutdown();

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_VG_INITOPERATION_HXX_ */
