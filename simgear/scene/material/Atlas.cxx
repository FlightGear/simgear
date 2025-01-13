// Atlas.cxx -- class for a material-based texture atlas
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

#include <osgDB/ReadFile>

#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>

using namespace simgear;


// Constructor
Atlas::Atlas(osg::ref_ptr<const SGReaderWriterOptions> options) {

    _options = options;

    _textureLookup1 = new osg::Uniform(osg::Uniform::Type::FLOAT_VEC4, "fg_textureLookup1", Atlas::MAX_MATERIALS);
    _textureLookup2 = new osg::Uniform(osg::Uniform::Type::FLOAT_VEC4, "fg_textureLookup2", Atlas::MAX_MATERIALS);
    _dimensions = new osg::Uniform(osg::Uniform::Type::FLOAT_VEC4, "fg_dimensionsArray", Atlas::MAX_MATERIALS);
    _materialParams1 = new osg::Uniform(osg::Uniform::Type::FLOAT_VEC4, "fg_materialParams1", Atlas::MAX_MATERIALS);
    _materialParams2 = new osg::Uniform(osg::Uniform::Type::FLOAT_VEC4, "fg_materialParams2", Atlas::MAX_MATERIALS);
    _shoreAtlastIndex = new osg::Uniform(osg::Uniform::Type::INT, "fg_shoreAtlasIndex");
    _bumpmapAmplitude  = new osg::Uniform(osg::Uniform::Type::FLOAT_VEC4, "fg_bumpmapAmplitude", Atlas::MAX_MATERIALS);
    _heightAmplitude  = new osg::Uniform(osg::Uniform::Type::FLOAT_VEC4, "fg_heightAmplitude", Atlas::MAX_MATERIALS);
    _PBRParams  = new osg::Uniform(osg::Uniform::Type::FLOAT_VEC4, "fg_materialPBRParams", Atlas::MAX_MATERIALS);
    _emission = new osg::Uniform(osg::Uniform::Type::FLOAT_VEC4, "fg_materialPBREmission", Atlas::MAX_MATERIALS);

    _image = new osg::Texture2DArray();
    _image->setMaxAnisotropy(SGSceneFeatures::instance()->getTextureFilter());
    _image->setResizeNonPowerOfTwoHint(false);
    _image->setWrap(osg::Texture::WRAP_S,osg::Texture::REPEAT);
    _image->setWrap(osg::Texture::WRAP_T,osg::Texture::REPEAT);

    _imageIndex = 0u; // Index into the image
    _materialLookupIndex = 0u; // Index into the material lookup

    // Add hardcoded atlas images.
    unsigned int standardTextureCount = size(Atlas::STANDARD_TEXTURES);
    _internalFormat = GL_RGB;
    for (; _imageIndex < standardTextureCount; _imageIndex++) {
        // Copy the texture into the atlas in the appropriate place
        osg::ref_ptr<osg::Image> subtexture = osgDB::readRefImageFile(Atlas::STANDARD_TEXTURES[_imageIndex], options);

        if (subtexture && subtexture->valid()) {

            if (_imageIndex == 0) {
                // The first subtexture determines the texture format.
                _internalFormat = subtexture->getInternalTextureFormat();
                SG_LOG(SG_TERRAIN, SG_DEBUG, "Internal Texture format for atlas: " << _internalFormat);
            }

            if ((subtexture->s() != 2048) || (subtexture->t() != 2048)) {
                subtexture->scaleImage(2048,2048,1);
            }

            if (subtexture->getInternalTextureFormat() != _internalFormat) {
                SG_LOG(SG_TERRAIN, SG_ALERT, "Atlas image " << subtexture->getFileName() << " has internal format " << subtexture->getInternalTextureFormat() << " rather than " << _internalFormat << " (6407=RGB 6408=RGBA)");
            }

            _image->setImage(_imageIndex,subtexture);
            _textureMap[Atlas::STANDARD_TEXTURES[_imageIndex]] = _imageIndex;
        }
    }
}

void Atlas::addMaterial(int landclass, bool isWater, bool isSea, SGSharedPtr<SGMaterial> mat) {

    SG_LOG(SG_TERRAIN, SG_DEBUG, "Atlas Landclass mapping: " << landclass << " : " << mat->get_names()[0]);
    _index[landclass] = _materialLookupIndex;
    _waterAtlas[landclass] = isWater;
    _seaAtlas[landclass] = isSea;
    unsigned int textureList[Atlas::MAX_TEXTURES];

    if (mat != NULL) {

        if (mat->get_num_textures(0) > Atlas::MAX_TEXTURES) {
            SG_LOG(SG_GENERAL, SG_ALERT, "Unable to build texture atlas for landclass " 
                << landclass << " aka " << mat->get_names()[0] 
                << " too many textures: " << mat->get_num_textures(0) 
                << " (maximum " << Atlas::MAX_TEXTURES << ")");
            return;
        }

        _dimensions->setElement(_materialLookupIndex, osg::Vec4f(mat->get_xsize(), mat->get_ysize(), 0.0, (double) mat->get_parameter("edge-hardness")));

        // The following are material parameters that are normally built into the Effect as Uniforms.  In the WS30
        // case we need to pass them as an array, indexed against the material.
        _materialParams1->setElement(_materialLookupIndex, osg::Vec4f(mat->get_parameter("transition_model"), mat->get_parameter("hires_overlay_bias"), mat->get_parameter("grain_strength"), mat->get_parameter("intrinsic_wetness")));
        _materialParams2->setElement(_materialLookupIndex, osg::Vec4f(mat->get_parameter("dot_density"), mat->get_parameter("dot_size"), mat->get_parameter("dust_resistance"), mat->get_parameter("rock_strata")));

        if (std::find(mat->get_names().begin(), mat->get_names().end(), "Sand") != mat->get_names().end()) {
            SG_LOG(SG_GENERAL, SG_DEBUG, "Found Sand material inserted into Atlas. Landclass " << landclass << ", index " << _materialLookupIndex);
            _shoreAtlastIndex->set((int) _materialLookupIndex);
        }

        _PBRParams->setElement(_materialLookupIndex, osg::Vec4f(mat->get_metallic(), mat->get_roughness(), mat->get_occlusion(), 0.0));
        _bumpmapAmplitude->setElement(_materialLookupIndex, mat->get_bumpmap_amplitude());
        _heightAmplitude->setElement(_materialLookupIndex, mat->get_height_amplitude());
        _emission->setElement(_materialLookupIndex, mat->get_emission());

        // Similarly, there are specifically 7 textures that are defined in the materials that need to be passed into
        // the shader as an array based on the material lookup.
        //
        // The mapping from terrain-default.eff / terrain-overlay.eff is as follows
        //
        //  TEXTURE NAME texture-unit  Material texture index Default value
        //  Primary texure      0             0               n/a
        //  gradient_texture    2            13               Textures/Terrain/rock_alt.png
        //  dot_texture         3            15               Textures/Terrain/sand6.png
        //  grain_texture       4            14               Textures/Terrain/grain_texture.png
        //  mix_texture         5            12               Textures/Terrain/void.png
        //  detail_texture      7            11               Textures/Terrain/void.png
        //  overlayPrimaryTex   7            20               Textures/Terrain/void.png
        //  overlaySecondaryTex 8            21               Textures/Terrain/void.png

        for (unsigned int i = 0; i < Atlas::MAX_TEXTURES; i++) {
            std::string texture = mat->get_one_texture(0,i);
            SG_LOG(SG_TERRAIN, SG_DEBUG, "Landclass " << landclass << " texture " << i << " : " << texture);

            if (texture.empty()) {
                // This is a rather horrible hardcoded mapping of the default textures defined in
                // terrain-default.eff and terrain-overlay.eff which are in effect defaults to
                // the material definitions
                if (i <  13) texture = std::string("Textures/Terrain/void.png");
                if (i == 13) texture = std::string("Textures/Terrain/rock_alt.png");
                if (i == 14) texture = std::string("Textures/Terrain/grain_texture.png");
                if (i == 15) texture = std::string("Textures/Terrain/sand6.png");
                if (i >  14) texture = std::string("Textures/Terrain/void.png");
            }

            SGPath texturePath = SGPath("Textures");
            std::string fullPath = SGModelLib::findDataFile(texture, _options, texturePath);

            if (fullPath.empty()) {
                SG_LOG(SG_GENERAL, SG_ALERT, "Cannot find texture \""
                        << texture << "\" in Textures folders when creating texture atlas");
                texture = std::string("Textures/Terrain/void.png");
                fullPath = SGModelLib::findDataFile(texture, _options, texturePath);
            }

            if (_textureMap.find(fullPath) == _textureMap.end()) {
                // Add any missing textures into the atlas image
                // Copy the texture into the atlas in the appropriate place
                osg::ref_ptr<osg::Image> subtexture = osgDB::readRefImageFile(fullPath, _options);

                if (subtexture && subtexture->valid()) {

                    if (subtexture->getInternalTextureFormat() != _internalFormat) {
                        SG_LOG(SG_TERRAIN, SG_ALERT, "Atlas image " << subtexture->getFileName() << " has internal format " << subtexture->getInternalTextureFormat() << " rather than " << _internalFormat << " (6407=RGB 6408=RGBA)");
                    }

                    if ((subtexture->s() != 2048) || (subtexture->t() != 2048)) {
                        subtexture->scaleImage(2048,2048,1);
                    }

                    _image->setImage(_imageIndex,subtexture);
                    _textureMap[fullPath] = _imageIndex;
                    ++_imageIndex;
                }
            }

            // At this point we know that the texture is present in the
            // atlas and referenced in the textureMap, so add it to the materialLookup
            textureList[i] = _textureMap[fullPath];
        }

        // We now have a textureList containing the full set of textures.  Pack the relevant ones into the Vec4 of the index Uniform.
        // This is a bit of a hack to maintain compatibility with the WS2.0 material definitions, as the material definitions use the 
        // 11-15th textures for the various overlay textures for terrain-default.eff, we do the same for ws30.eff
        _textureLookup1->setElement(_materialLookupIndex, osg::Vec4f( (float) (textureList[0] / 255.0), (float) (textureList[11] / 255.0), (float) (textureList[12] / 255.0), (float) (textureList[13] / 255.0)));
        _textureLookup2->setElement(_materialLookupIndex, osg::Vec4f( (float) (textureList[14] / 255.0), (float) (textureList[15] / 255.0), (float) (textureList[20] / 255.0), (float) (textureList[21] / 255.0)));
        _bvhMaterialMap[_materialLookupIndex] = mat;
    } else {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Attempt to add undefined material to Material Atlas: " << landclass);
    }

    ++_materialLookupIndex;
}    

void Atlas::addUniforms(osg::StateSet* stateset) {
    stateset->addUniform(_dimensions);
    stateset->addUniform(_textureLookup1);
    stateset->addUniform(_textureLookup2);
    stateset->addUniform(_materialParams1);
    stateset->addUniform(_materialParams2);
    stateset->addUniform(_PBRParams);
    stateset->addUniform(_emission);
    stateset->addUniform(_bumpmapAmplitude);    
    stateset->addUniform(_heightAmplitude);    
    stateset->addUniform(_shoreAtlastIndex);
}

