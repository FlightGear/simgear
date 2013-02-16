// Copyright (C) 2009  Tim Moore timoore@redhat.com
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef SIMGEAR_TEXTUREBUILDER_HXX
#define SIMGEAR_TEXTUREBUILDER_HXX 1

#include <osg/StateSet>
#include <osg/Texture>
#include "EffectBuilder.hxx"

namespace simgear
{
class TextureBuilder : public EffectBuilder<osg::Texture>
{
public:
    // Hack to force inclusion of TextureBuilder.cxx in library
    static osg::Texture* buildFromType(Effect* effect, Pass* pass, const std::string& type,
                                       const SGPropertyNode*props,
                                       const SGReaderWriterOptions* options);
};

struct TextureUnitBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options);
};


bool makeTextureParameters(SGPropertyNode* paramRoot, const osg::StateSet* ss);
}
#endif
