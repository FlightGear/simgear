// Parse osg::BlendFunc from property nodes
//
// Copyright (C) 2008 - 2010  Tim Moore timoore33@gmail.com
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

#include <simgear_config.h>
#include "parseBlendFunc.hxx"
#include "EffectBuilder.hxx"
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <osg/BlendFunc>

using osg::BlendFunc;

namespace simgear
{

  //----------------------------------------------------------------------------
  extern effect::EffectPropertyMap<BlendFunc::BlendFuncMode> blendFuncModes;

  //----------------------------------------------------------------------------
  bool parseBlendFunc( osg::StateSet* ss,
                       const SGPropertyNode* src,
                       const SGPropertyNode* dest,
                       const SGPropertyNode* src_rgb,
                       const SGPropertyNode* dest_rgb,
                       const SGPropertyNode* src_alpha,
                       const SGPropertyNode* dest_alpha )
  {
    if( !ss )
      return false;

    BlendFunc::BlendFuncMode src_mode = BlendFunc::ONE;
    BlendFunc::BlendFuncMode dest_mode = BlendFunc::ZERO;

    if( src )
      findAttr(blendFuncModes, src, src_mode);
    if( dest )
      findAttr(blendFuncModes, dest, dest_mode);

    if( src && dest
        && !(src_rgb || src_alpha || dest_rgb || dest_alpha)
        && src_mode == BlendFunc::SRC_ALPHA
        && dest_mode == BlendFunc::ONE_MINUS_SRC_ALPHA )
    {
      ss->setAttributeAndModes(
        StateAttributeFactory::instance()->getStandardBlendFunc()
      );
      return true;
    }

    BlendFunc* blend_func = new BlendFunc;
    if( src )
      blend_func->setSource(src_mode);
    if( dest )
      blend_func->setDestination(dest_mode);

    if( src_rgb )
    {
      BlendFunc::BlendFuncMode sourceRGBMode;
      findAttr(blendFuncModes, src_rgb, sourceRGBMode);
      blend_func->setSourceRGB(sourceRGBMode);
    }
    if( dest_rgb)
    {
      BlendFunc::BlendFuncMode destRGBMode;
      findAttr(blendFuncModes, dest_rgb, destRGBMode);
      blend_func->setDestinationRGB(destRGBMode);
    }
    if( src_alpha )
    {
      BlendFunc::BlendFuncMode sourceAlphaMode;
      findAttr(blendFuncModes, src_alpha, sourceAlphaMode);
      blend_func->setSourceAlpha(sourceAlphaMode);
    }
    if( dest_alpha)
    {
      BlendFunc::BlendFuncMode destAlphaMode;
      findAttr(blendFuncModes, dest_alpha, destAlphaMode);
      blend_func->setDestinationAlpha(destAlphaMode);
    }
    ss->setAttributeAndModes(blend_func);
    return true;
  }

} // namespace simgear
