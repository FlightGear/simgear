// Helper for OSG related debugging
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

#include "OsgDebug.hxx"
#include <osg/Node>

namespace simgear
{

  //----------------------------------------------------------------------------
  std::string getNodePathString(const osg::Node* node)
  {
    if( !node )
      return "(nullptr)";

    std::string str;
    const osg::NodePathList& paths = node->getParentalNodePaths();
    for(size_t i = 0; i < paths.size(); ++i)
    {
      if( !str.empty() )
        str += ":";

      const osg::NodePath& path = paths.at(i);
      for(size_t j = 0; j < path.size(); ++j)
      {
        const osg::Node* node = path[j];
        str += "/'" + node->getName() + "'";
      }
    }

    return str;
  }

} // namespace simgear
