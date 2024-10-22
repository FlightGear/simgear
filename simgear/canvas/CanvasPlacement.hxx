// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Base class for canvas placements
 */

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

      Placement(const Placement&) = delete;
      Placement& operator=(const Placement&) = delete;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_PLACEMENT_HXX_ */
