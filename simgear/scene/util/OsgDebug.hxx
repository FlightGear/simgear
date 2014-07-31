///@file
/// Helper for OSG related debugging
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef OSGDEBUG_HXX_
#define OSGDEBUG_HXX_

#include <string>

namespace osg { class Node; }
namespace simgear
{

  /**
   * Get parent path(s) of scene graph node as string.
   */
  std::string getNodePathString(const osg::Node* node);

} // namespace simgear


#endif /* OSGDEBUG_HXX_ */
