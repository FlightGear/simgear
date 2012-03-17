// mat.cxx -- class to handle material properties
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <string.h>
#include <map>
#include <vector>
#include <string>

#include <boost/foreach.hpp>
#include "mat.hxx"

#include <osg/CullFace>
#include <osg/Material>
#include <osg/ShadeModel>
#include <osg/StateSet>
#include <osg/TexEnv>
#include <osg/Texture>
#include <osg/Texture2D>
#include <osgDB/ReaderWriter>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/FileUtils>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>

#include "Effect.hxx"
#include "Technique.hxx"
#include "Pass.hxx"

using std::map;
using std::string;
using namespace simgear;


////////////////////////////////////////////////////////////////////////
// Constructors and destructor.
////////////////////////////////////////////////////////////////////////

SGMaterial::_internal_state::_internal_state(Effect *e, bool l,
                                             const SGReaderWriterOptions* o)
    : effect(e), effect_realized(l), options(o)
{
}

SGMaterial::_internal_state::_internal_state(Effect *e, const string &t, bool l, 
                                             const SGReaderWriterOptions* o)
    : effect(e), effect_realized(l), options(o)
{
    texture_paths.push_back(std::make_pair(t,0));
}

void SGMaterial::_internal_state::add_texture(const std::string &t, int i)
{
    texture_paths.push_back(std::make_pair(t,i));
}

SGMaterial::SGMaterial( const SGReaderWriterOptions* options,
                        const SGPropertyNode *props,
                        SGPropertyNode *prop_root )
{
    init();
    read_properties( options, props, prop_root );
    buildEffectProperties(options);
}

SGMaterial::SGMaterial( const osgDB::Options* options,
                        const SGPropertyNode *props, 
                        SGPropertyNode *prop_root)
{
    osg::ref_ptr<SGReaderWriterOptions> opt;
    opt = SGReaderWriterOptions::copyOrCreate(options);
    init();
    read_properties(opt.get(), props, prop_root);
    buildEffectProperties(opt.get());
}

SGMaterial::~SGMaterial (void)
{
}


////////////////////////////////////////////////////////////////////////
// Public methods.
////////////////////////////////////////////////////////////////////////

void
SGMaterial::read_properties(const SGReaderWriterOptions* options,
                            const SGPropertyNode *props,
                            SGPropertyNode *prop_root)
{
    std::vector<bool> dds;
    std::vector<SGPropertyNode_ptr> textures = props->getChildren("texture");
    for (unsigned int i = 0; i < textures.size(); i++)
    {
        string tname = textures[i]->getStringValue();

        if (tname.empty()) {
            tname = "unknown.rgb";
        }

        SGPath tpath("Textures.high");
        tpath.append(tname);
        string fullTexPath = SGModelLib::findDataFile(tpath.str(), options);
        if (fullTexPath.empty()) {
            tpath = SGPath("Textures");
            tpath.append(tname);
            fullTexPath = SGModelLib::findDataFile(tpath.str(), options);
        }

        if (tpath.lower_extension() == "dds") {
            dds.push_back(true);
        } else {
            dds.push_back(false);
        }

        if (!fullTexPath.empty() ) {
            _internal_state st( NULL, fullTexPath, false, options );
            _status.push_back( st );
        }
    }

    std::vector<SGPropertyNode_ptr> texturesets = props->getChildren("texture-set");
    for (unsigned int i = 0; i < texturesets.size(); i++)
    {
        _internal_state st( NULL, false, options );
        std::vector<SGPropertyNode_ptr> textures = texturesets[i]->getChildren("texture");
        for (unsigned int j = 0; j < textures.size(); j++)
        {
            string tname = textures[j]->getStringValue();
            if (tname.empty()) {
                tname = "unknown.rgb";
            }

            SGPath tpath("Textures.high");
            tpath.append(tname);
            string fullTexPath = SGModelLib::findDataFile(tpath.str(), options);
            if (fullTexPath.empty()) {
                tpath = SGPath("Textures");
                tpath.append(tname);
                fullTexPath = SGModelLib::findDataFile(tpath.str(), options);
            }

            if (j == 0) {
                if (tpath.lower_extension() == "dds") {
                    dds.push_back(true);
                } else {
                    dds.push_back(false);
                }
            }

            st.add_texture(fullTexPath, textures[j]->getIndex());
        }

        if (!st.texture_paths.empty() ) {
            _status.push_back( st );
        }
    }

    if (textures.size() == 0 && texturesets.size() == 0) {
        SGPath tpath("Textures");
        tpath.append("Terrain");
        tpath.append("unknown.rgb");
        _internal_state st( NULL, tpath.str(), true, options );
        _status.push_back( st );
    }
    
    std::vector<SGPropertyNode_ptr> masks = props->getChildren("object-mask");
    for (unsigned int i = 0; i < masks.size(); i++)
    {
        string omname = masks[i]->getStringValue();

        if (! omname.empty()) {
            SGPath ompath("Textures.high");
            ompath.append(omname);
            string fullMaskPath = SGModelLib::findDataFile(ompath.str(), options);

            if (fullMaskPath.empty()) {
                ompath = SGPath("Textures");
                ompath.append(omname);
                fullMaskPath = SGModelLib::findDataFile(ompath.str(), options);
            }

            if (fullMaskPath.empty()) {
                SG_LOG(SG_GENERAL, SG_ALERT, "Cannot find texture file \""
                        << ompath.str() << "\"");
            }
            else
            {
                osg::Image* image = osgDB::readImageFile(fullMaskPath, options);
                if (image && image->valid())
                {
                    osg::Texture2D* object_mask = new osg::Texture2D;

                    bool dds_mask = (ompath.lower_extension() == "dds");

                    if (dds[i] != dds_mask) {
                        // Texture format does not match mask format. This is relevant for
                        // the object mask, as DDS textures have an origin at the bottom
                        // left rather than top left, therefore we flip the object mask
                        // vertically.
                        image->flipVertical();
                    }

                    object_mask->setImage(image);

                    // We force the filtering to be nearest, as the red channel (rotation)
                    // in particular, doesn't make sense to be interpolated between pixels.
                    object_mask->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
                    object_mask->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

                    object_mask->setDataVariance(osg::Object::STATIC);
                    object_mask->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
                    object_mask->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
                    _masks.push_back(object_mask);
                }
            }
        }
    }

    xsize = props->getDoubleValue("xsize", 0.0);
    ysize = props->getDoubleValue("ysize", 0.0);
    wrapu = props->getBoolValue("wrapu", true);
    wrapv = props->getBoolValue("wrapv", true);
    mipmap = props->getBoolValue("mipmap", true);
    light_coverage = props->getDoubleValue("light-coverage", 0.0);
    wood_coverage = props->getDoubleValue("wood-coverage", 0.0);
    tree_height = props->getDoubleValue("tree-height-m", 0.0);
    tree_width = props->getDoubleValue("tree-width-m", 0.0);
    tree_range = props->getDoubleValue("tree-range-m", 0.0);
    tree_varieties = props->getIntValue("tree-varieties", 1);

    const SGPropertyNode* treeTexNode = props->getChild("tree-texture");
    
    if (treeTexNode) {
        string treeTexPath = props->getStringValue("tree-texture");

        if (! treeTexPath.empty()) {
            SGPath treePath("Textures.high");
            treePath.append(treeTexPath);
            tree_texture = SGModelLib::findDataFile(treePath.str(), options);

            if (tree_texture.empty()) {
                treePath = SGPath("Textures");
                treePath.append(treeTexPath);
                tree_texture = SGModelLib::findDataFile(treePath.str(), options);
            }
        }
    }

    // surface values for use with ground reactions
    solid = props->getBoolValue("solid", true);
    friction_factor = props->getDoubleValue("friction-factor", 1.0);
    rolling_friction = props->getDoubleValue("rolling-friction", 0.02);
    bumpiness = props->getDoubleValue("bumpiness", 0.0);
    load_resistance = props->getDoubleValue("load-resistance", 1e30);

    // Taken from default values as used in ac3d
    ambient[0] = props->getDoubleValue("ambient/r", 0.2);
    ambient[1] = props->getDoubleValue("ambient/g", 0.2);
    ambient[2] = props->getDoubleValue("ambient/b", 0.2);
    ambient[3] = props->getDoubleValue("ambient/a", 1.0);

    diffuse[0] = props->getDoubleValue("diffuse/r", 0.8);
    diffuse[1] = props->getDoubleValue("diffuse/g", 0.8);
    diffuse[2] = props->getDoubleValue("diffuse/b", 0.8);
    diffuse[3] = props->getDoubleValue("diffuse/a", 1.0);

    specular[0] = props->getDoubleValue("specular/r", 0.0);
    specular[1] = props->getDoubleValue("specular/g", 0.0);
    specular[2] = props->getDoubleValue("specular/b", 0.0);
    specular[3] = props->getDoubleValue("specular/a", 1.0);

    emission[0] = props->getDoubleValue("emissive/r", 0.0);
    emission[1] = props->getDoubleValue("emissive/g", 0.0);
    emission[2] = props->getDoubleValue("emissive/b", 0.0);
    emission[3] = props->getDoubleValue("emissive/a", 1.0);

    shininess = props->getDoubleValue("shininess", 1.0);

    if (props->hasChild("effect"))
        effect = props->getStringValue("effect");

    std::vector<SGPropertyNode_ptr> object_group_nodes =
            ((SGPropertyNode *)props)->getChildren("object-group");
    for (unsigned int i = 0; i < object_group_nodes.size(); i++)
        object_groups.push_back(new SGMatModelGroup(object_group_nodes[i]));

    // read glyph table for taxi-/runway-signs
    std::vector<SGPropertyNode_ptr> glyph_nodes = props->getChildren("glyph");
    for (unsigned int i = 0; i < glyph_nodes.size(); i++) {
        const char *name = glyph_nodes[i]->getStringValue("name");
        if (name)
            glyphs[name] = new SGMaterialGlyph(glyph_nodes[i]);
    }

    // Read conditions node
    const SGPropertyNode *conditionNode = props->getChild("condition");
    if (conditionNode) {
        condition = sgReadCondition(prop_root, conditionNode);
    }
}



////////////////////////////////////////////////////////////////////////
// Private methods.
////////////////////////////////////////////////////////////////////////

void 
SGMaterial::init ()
{
    _status.clear();
    xsize = 0;
    ysize = 0;
    wrapu = true;
    wrapv = true;

    mipmap = true;
    light_coverage = 0.0;

    solid = true;
    friction_factor = 1;
    rolling_friction = 0.02;
    bumpiness = 0;
    load_resistance = 1e30;

    shininess = 1.0;
    for (int i = 0; i < 4; i++) {
        ambient[i]  = (i < 3) ? 0.2 : 1.0;
        specular[i] = (i < 3) ? 0.0 : 1.0;
        diffuse[i]  = (i < 3) ? 0.8 : 1.0;
        emission[i] = (i < 3) ? 0.0 : 1.0;
    }
    effect = "Effects/terrain-default";
}

Effect* SGMaterial::get_effect(int i)
{    
    if(!_status[i].effect_realized) {
        _status[i].effect->realizeTechniques(_status[i].options.get());
        _status[i].effect_realized = true;
    }
    return _status[i].effect.get();
}

Effect* SGMaterial::get_effect(SGTexturedTriangleBin triangleBin)
{
    if (_status.size() == 0) {
        SG_LOG( SG_GENERAL, SG_WARN, "No effect available.");
        return 0;
    }
    
    int i = triangleBin.getTextureIndex() % _status.size();
    return get_effect(i);
}

Effect* SGMaterial::get_effect()
{
    return get_effect(0);
}


osg::Texture2D* SGMaterial::get_object_mask(SGTexturedTriangleBin triangleBin)
{
    if (_status.size() == 0) {
        SG_LOG( SG_GENERAL, SG_WARN, "No mask available.");
        return 0;
    }
    
    // Note that the object mask is closely linked to the texture/effect
    // so we index based on the texture index, 
    unsigned int i = triangleBin.getTextureIndex() % _status.size();
    if (i < _masks.size()) {
        return _masks[i];
    } else {
        return 0;
    }
}

void SGMaterial::buildEffectProperties(const SGReaderWriterOptions* options)
{
    using namespace osg;
    ref_ptr<SGMaterialUserData> user = new SGMaterialUserData(this);
    SGPropertyNode_ptr propRoot = new SGPropertyNode();
    makeChild(propRoot, "inherits-from")->setStringValue(effect);
    SGPropertyNode* paramProp = makeChild(propRoot, "parameters");
    SGPropertyNode* materialProp = makeChild(paramProp, "material");
    makeChild(materialProp, "ambient")->setValue(SGVec4d(ambient));    
    makeChild(materialProp, "diffuse")->setValue(SGVec4d(diffuse));
    makeChild(materialProp, "specular")->setValue(SGVec4d(specular));
    makeChild(materialProp, "emissive")->setValue(SGVec4d(emission));
    makeChild(materialProp, "shininess")->setFloatValue(shininess);
    if (ambient[3] < 1 || diffuse[3] < 1 ||
        specular[3] < 1 || emission[3] < 1) {
        makeChild(paramProp, "transparent")->setBoolValue(true);
        SGPropertyNode* binProp = makeChild(paramProp, "render-bin");
        makeChild(binProp, "bin-number")->setIntValue(TRANSPARENT_BIN);
        makeChild(binProp, "bin-name")->setStringValue("DepthSortedBin");
    }
    BOOST_FOREACH(_internal_state& matState, _status)
    {
        SGPropertyNode_ptr effectProp = new SGPropertyNode();
        copyProperties(propRoot, effectProp);
        SGPropertyNode* effectParamProp = effectProp->getChild("parameters", 0);
        for (unsigned int i = 0; i < matState.texture_paths.size(); i++) {
            SGPropertyNode* texProp = makeChild(effectParamProp, "texture", matState.texture_paths[i].second);
            makeChild(texProp, "image")->setStringValue(matState.texture_paths[i].first);
            makeChild(texProp, "filter")
                ->setStringValue(mipmap ? "linear-mipmap-linear" : "nearest");
            makeChild(texProp, "wrap-s")
                ->setStringValue(wrapu ? "repeat" : "clamp");
            makeChild(texProp, "wrap-t")
                ->setStringValue(wrapv ? "repeat" : "clamp");
        }
        makeChild(effectParamProp, "xsize")->setDoubleValue(xsize);
        makeChild(effectParamProp, "ysize")->setDoubleValue(ysize);
        makeChild(effectParamProp, "scale")->setValue(SGVec3d(xsize,ysize,0.0));
        makeChild(effectParamProp, "light-coverage")->setDoubleValue(light_coverage);

        matState.effect = makeEffect(effectProp, false, options);
        matState.effect->setUserData(user.get());
    }
}

SGMaterialGlyph* SGMaterial::get_glyph (const string& name) const
{
    map<string, SGSharedPtr<SGMaterialGlyph> >::const_iterator it;
    it = glyphs.find(name);
    if (it == glyphs.end())
        return 0;

    return it->second;
}


////////////////////////////////////////////////////////////////////////
// SGMaterialGlyph.
////////////////////////////////////////////////////////////////////////

SGMaterialGlyph::SGMaterialGlyph(SGPropertyNode *p) :
    _left(p->getDoubleValue("left", 0.0)),
    _right(p->getDoubleValue("right", 1.0))
{
}

void
SGSetTextureFilter( int max) {
    SGSceneFeatures::instance()->setTextureFilter( max);
}

int
SGGetTextureFilter() {
    return SGSceneFeatures::instance()->getTextureFilter();
}
