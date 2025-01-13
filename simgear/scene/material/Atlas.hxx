// Atlas.hxx -- class for a material-based texture atlas
//
// Copyright (C) 2022 Stuart Buchanan
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#include <simgear/compiler.h>

#include <simgear/bvh/BVHMaterial.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/scene/material/mat.hxx>
#include <osg/Texture2DArray>
#include <osg/Texture1D>

#include <memory>
#include <string>   // Standard C++ string library
#include <map>      // STL associative "array"
#include <vector>   // STL "array"

namespace simgear
{

// Atlas of textures
class Atlas : public osg::Referenced
{
public:

    // Constructor
    Atlas (osg::ref_ptr<const SGReaderWriterOptions> options );

    // Mapping of landclass numbers to indexes within the atlas
    // materialLookup
    typedef std::map<int, int> AtlasIndex;
    typedef std::map<unsigned, SGSharedPtr<SGMaterial> >  AtlasMap;

    // Mapping of texture filenames to their index in the Atlas image itself.
    typedef std::map<std::string, unsigned int> TextureMap;

    // The Texture array itself
    typedef osg::ref_ptr<osg::Texture2DArray> AtlasImage;
    typedef std::map<int, bool> WaterAtlas;

    void addUniforms(osg::StateSet* stateset);
    void addMaterial(int landclass, bool water, bool sea, SGSharedPtr<SGMaterial> mat);

    // Maximum number of material entries in the atlas
    static const unsigned int MAX_MATERIALS = 64;

    // Lookups into the Atlas from landclass
    bool isWater(int landclass) { return _waterAtlas[landclass]; };
    bool isSea(int landclass) { return _seaAtlas[landclass];}
    int getIndex(int landclass) { return _index[landclass]; };

    AtlasMap getBVHMaterialMap() { return _bvhMaterialMap; };
    AtlasImage getImage() { return _image; };

private:
    AtlasIndex _index;
    AtlasImage _image;
    GLint _internalFormat;

    osg::ref_ptr<const SGReaderWriterOptions> _options;

    osg::ref_ptr<osg::Uniform> _textureLookup1;
    osg::ref_ptr<osg::Uniform> _textureLookup2;
    osg::ref_ptr<osg::Uniform> _dimensions;
    osg::ref_ptr<osg::Uniform> _shoreAtlastIndex;

    osg::ref_ptr<osg::Uniform> _materialParams1;
    osg::ref_ptr<osg::Uniform> _materialParams2;

    osg::ref_ptr<osg::Uniform> _PBRParams;
    osg::ref_ptr<osg::Uniform> _emission;
    osg::ref_ptr<osg::Uniform> _heightAmplitude;
    osg::ref_ptr<osg::Uniform> _bumpmapAmplitude;


    unsigned int _imageIndex; // Index into the image
    unsigned int _materialLookupIndex; // Index into the material lookup

    WaterAtlas _waterAtlas;
    WaterAtlas _seaAtlas;
    TextureMap _textureMap;
    AtlasMap _bvhMaterialMap;

    // Maximum number of textures per texture-set for the Atlas.
    static const unsigned int MAX_TEXTURES = 22;

    // Standard textures, used by water shader in particular.
    // Indexes are hardcoded in Shaders/ws30-water.frag
    inline static const std::string STANDARD_TEXTURES[] = {
        "Textures/Terrain/water.png",
        "Textures/Water/water-reflection-ws30.png",
        "Textures/Water/waves-ver10-nm-ws30.png",
        "Textures/Water/water_sine_nmap-ws30.png",
        "Textures/Water/water-reflection-grey-ws30.png",
        "Textures/Water/sea_foam-ws30.png",
        "Textures/Water/perlin-noise-nm.png",

        // The following two textures are large and don't have an alpha channel.  Ignoring for now.
        //"Textures/Globe/ocean_depth_1.png",
        //"Textures/Globe/globe_colors.jpg",
        "Textures/Terrain/packice-overlay.png"
    };
};

}
