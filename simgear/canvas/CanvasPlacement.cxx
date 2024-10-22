// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Base class for canvas placements
 */

#include <simgear_config.h>
#include "CanvasPlacement.hxx"
#include <simgear/props/props.hxx>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  Placement::Placement(SGPropertyNode* node):
    _node(node)
  {

  }

  //----------------------------------------------------------------------------
  Placement::~Placement()
  {

  }

  //----------------------------------------------------------------------------
  SGConstPropertyNode_ptr Placement::getProps() const
  {
    return _node;
  }

  //----------------------------------------------------------------------------
  SGPropertyNode_ptr Placement::getProps()
  {
    return _node;
  }

  //----------------------------------------------------------------------------
  bool Placement::childChanged(SGPropertyNode* child)
  {
    return false;
  }

} // namespace canvas
} // namespace simgear
