// Canvas with 2D rendering API
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

#include "CanvasMgr.hxx"
#include "Canvas.hxx"

#include <boost/bind.hpp>

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
  CanvasMgr::CanvasMgr( SGPropertyNode_ptr node,
                        SystemAdapterPtr system_adapter ):
    PropertyBasedMgr(node, "texture", &canvasFactory),
    _system_adapter(system_adapter)
  {

  }

  //----------------------------------------------------------------------------
  CanvasPtr CanvasMgr::createCanvas(const std::string& name)
  {
    return boost::static_pointer_cast<Canvas>( createElement(name) );
  }

  //----------------------------------------------------------------------------
  CanvasPtr CanvasMgr::getCanvas(size_t index) const
  {
    return boost::static_pointer_cast<Canvas>( getElement(index) );
  }

  //----------------------------------------------------------------------------
  void CanvasMgr::elementCreated(PropertyBasedElementPtr element)
  {
    CanvasPtr canvas = boost::static_pointer_cast<Canvas>(element);
    canvas->setSystemAdapter(_system_adapter);
    canvas->setCanvasMgr(this);
  }

} // namespace canvas
} // namespace simgear
