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

#ifndef SG_CANVAS_MGR_H_
#define SG_CANVAS_MGR_H_

#include "canvas_fwd.hxx"
#include <simgear/props/PropertyBasedMgr.hxx>

namespace simgear
{
namespace canvas
{

  class CanvasMgr:
    public PropertyBasedMgr
  {
    public:

      /**
       * @param node    Root node of branch used to control canvasses
       */
      CanvasMgr(SGPropertyNode_ptr node);

      /**
       * Create a new canvas
       *
       * @param name    Name of the new canvas
       */
      CanvasPtr createCanvas(const std::string& name = "");

      /**
       * Get ::Canvas by index
       *
       * @param index Index of texture node in /canvas/by-index/
       */
      CanvasPtr getCanvas(size_t index) const;

      /**
       * Get ::Canvas by name
       *
       * @param name Value of child node "name" in
       *             /canvas/by-index/texture[i]/name
       */
      CanvasPtr getCanvas(const std::string& name) const;

    protected:

      virtual void elementCreated(PropertyBasedElementPtr element);
  };

} // namespace canvas
} // namespace simgear

#endif /* SG_CANVAS_MGR_H_ */
