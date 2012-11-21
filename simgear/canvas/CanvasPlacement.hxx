// Base class for canvas placements
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

#ifndef CANVAS_PLACEMENT_HXX_
#define CANVAS_PLACEMENT_HXX_

#include <simgear/props/propsfwd.hxx>

namespace simgear
{
namespace canvas
{

  class Placement
  {
    public:
      Placement(SGPropertyNode* node);
      virtual ~Placement() = 0;

      SGConstPropertyNode_ptr getProps() const;
      SGPropertyNode_ptr getProps();

      virtual bool childChanged(SGPropertyNode* child);

    protected:
      SGPropertyNode_ptr _node;

    private:
      Placement(const Placement&) /* = delete */;
      Placement& operator=(const Placement&) /* = delete */;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_PLACEMENT_HXX_ */
