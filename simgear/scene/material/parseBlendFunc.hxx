///@file
/// Parse osg::BlendFunc from property nodes
///
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

#ifndef SG_PARSE_BLEND_FUNC_HXX_
#define SG_PARSE_BLEND_FUNC_HXX_

#include <simgear/props/propsfwd.hxx>
#include <osg/StateSet>

namespace simgear
{

  /**
   * Parse a blend function from the given property nodes and apply it to the
   * given osg::StateSet.
   *
   * @param ss          StateState which the blend function will be applied to
   * @param src
   * @param dest
   * @param src_rgb
   * @param dest_rgb
   * @param src_alpha
   * @param dest_alpha
   */
  bool parseBlendFunc( osg::StateSet* ss,
                       const SGPropertyNode* src = 0,
                       const SGPropertyNode* dest = 0,
                       const SGPropertyNode* src_rgb = 0,
                       const SGPropertyNode* dest_rgb = 0,
                       const SGPropertyNode* src_alpha = 0,
                       const SGPropertyNode* dest_alpha = 0 );

} // namespace simgear


#endif /* SG_PARSE_BLEND_FUNC_HXX_ */
