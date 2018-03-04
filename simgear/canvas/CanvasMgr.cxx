///@file
/// Canvas with 2D rendering API
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
