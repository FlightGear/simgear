// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas with 2D rendering API
 */

#include <simgear_config.h>

#include "CanvasMgr.hxx"
#include "Canvas.hxx"
#include "CanvasEventManager.hxx"

namespace simgear
{
namespace canvas
{

  /**
   * Canvas factory
   */
  CanvasPtr canvasFactory(SGPropertyNode* node)
  {
    return CanvasPtr(new Canvas(node));
  }

  //----------------------------------------------------------------------------
  CanvasMgr::CanvasMgr(SGPropertyNode_ptr node):
    PropertyBasedMgr(node, "texture", &canvasFactory)
  {

  }

  //----------------------------------------------------------------------------
  CanvasPtr CanvasMgr::createCanvas(const std::string& name)
  {
    return static_cast<Canvas*>( createElement(name).get() );
  }

  //----------------------------------------------------------------------------
  CanvasPtr CanvasMgr::getCanvas(size_t index) const
  {
    return static_cast<Canvas*>( getElement(index).get() );
  }

  //----------------------------------------------------------------------------
  CanvasPtr CanvasMgr::getCanvas(const std::string& name) const
  {
    return static_cast<Canvas*>( getElement(name).get() );
  }

  //----------------------------------------------------------------------------
  void CanvasMgr::elementCreated(PropertyBasedElementPtr element)
  {
    CanvasPtr canvas = static_cast<Canvas*>(element.get());
    canvas->setCanvasMgr(this);
  }

} // namespace canvas
} // namespace simgear
