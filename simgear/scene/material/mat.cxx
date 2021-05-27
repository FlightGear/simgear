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
#include <mutex>

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
#include <simgear/io/iostreams/sgstream.hxx>

#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/vectorPropTemplates.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/material/matmodel.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/tgdb/SGTexturedTriangleBin.hxx>

#include "Effect.hxx"
#include "Technique.hxx"
#include "Pass.hxx"

using std::map;
using namespace simgear;

////////////////////////////////////////////////////////////////////////
// Constructors and destructor.
////////////////////////////////////////////////////////////////////////

SGMaterial::_internal_state::_internal_state(Effect *e, bool l,
                                             const SGReaderWriterOptions* o)
    : effect(e), effect_realized(l), options(o)
{
}

SGMaterial::_internal_state::_internal_state( Effect *e,
                                              const std::string &t,
                                              bool l,
                                              const SGReaderWriterOptions* o )
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
                        SGPropertyNode *prop_root,
                        AreaList *a,
                        SGSharedPtr<const SGCondition> c,
                        const std::string& n)
{
    init();
    areas = a;
    condition = c;
    region = n;
    read_properties( options, props, prop_root );
    buildEffectProperties(options);
}

SGMaterial::SGMaterial( const osgDB::Options* options,
                        const SGPropertyNode *props,
                        SGPropertyNode *prop_root,
                        AreaList *a,
                        SGSharedPtr<const SGCondition> c,
                        const std::string& n)
{
    osg::ref_ptr<SGReaderWriterOptions> opt;
    opt = SGReaderWriterOptions::copyOrCreate(options);
    areas = a;
    condition = c;
    region = n;
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
    float default_object_range = prop_root->getFloatValue("/sim/rendering/static-lod/rough", SG_OBJECT_RANGE_ROUGH);
    std::vector<bool> dds;
    std::vector<SGPropertyNode_ptr> textures = props->getChildren("texture");
    for (unsigned int i = 0; i < textures.size(); i++)
    {
        std::string tname = textures[i]->getStringValue();

        if (tname.empty()) {
            tname = "unknown.rgb";
        }

        SGPath tpath("Textures");
        tpath.append(tname);
        std::string fullTexPath = SGModelLib::findDataFile(tpath, options);
        if (fullTexPath.empty()) {
            SG_LOG(SG_GENERAL, SG_ALERT, "Cannot find texture \""
                    << tname << "\" in Textures folders.");
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
            std::string tname = textures[j]->getStringValue();
            if (tname.empty()) {
                tname = "unknown.rgb";
            }

            SGPath tpath("Textures");
            tpath.append(tname);
            std::string fullTexPath = SGModelLib::findDataFile(tpath, options);
            if (fullTexPath.empty()) {
                SG_LOG(SG_GENERAL, SG_ALERT, "Cannot find texture \""
                        << tname << "\" in Textures folders.");
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

    if (textures.empty() && texturesets.empty()) {
        SGPath tpath("Textures");
        tpath.append("Terrain");
        tpath.append("unknown.rgb");
        _internal_state st( NULL, tpath.utf8Str(), true, options );
        _status.push_back( st );
    }

    std::vector<SGPropertyNode_ptr> masks = props->getChildren("object-mask");
    for (unsigned int i = 0; i < masks.size(); i++)
    {
        std::string omname = masks[i]->getStringValue();

        if (! omname.empty()) {
            SGPath ompath("Textures");
            ompath.append(omname);
            std::string fullMaskPath = SGModelLib::findDataFile(ompath, options);

            if (fullMaskPath.empty()) {
                SG_LOG(SG_GENERAL, SG_ALERT, "Cannot find texture \""
                        << omname << "\" in Textures folders.");
            }
            else
            {
#if OSG_VERSION_LESS_THAN(3,4,0)
                osg::Image* image = osgDB::readImageFile(fullMaskPath, options);
#else
                osg::Image* image = osgDB::readRefImageFile(fullMaskPath, options);
#endif
                if (image && image->valid())
                {
                    Texture2DRef object_mask = new osg::Texture2D;

                    bool dds_mask = (ompath.lower_extension() == "dds");

                    if (i < dds.size() && dds[i] != dds_mask) {
                        // Texture format does not match mask format. This is relevant for
                        // the object mask, as DDS textures have an origin at the bottom
                        // left rather than top left. Therefore we flip a copy of the image
                        // (otherwise a second reference to the object mask would flip it
                        // back!).
                        SG_LOG(SG_GENERAL, SG_DEBUG, "Flipping object mask" << omname);
                        image = (osg::Image* ) image->clone(osg::CopyOp::SHALLOW_COPY);
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

    // Building properties
    building_coverage = props->getDoubleValue("building-coverage", 0.0);
    building_spacing = props->getDoubleValue("building-spacing-m", 5.0);

    std::string bt = props->getStringValue( "building-texture",
                                            "Textures/buildings.png" );
    building_texture = SGModelLib::findDataFile(bt, options);

    if (building_texture.empty()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Cannot find texture \"" << bt);
    }

    bt = props->getStringValue("building-lightmap", "Textures/buildings-lightmap.png");
    building_lightmap = SGModelLib::findDataFile(bt, options);

    if (building_lightmap.empty()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Cannot find texture \"" << bt);
    }

    building_small_ratio = props->getDoubleValue("building-small-ratio", 0.8);
    building_medium_ratio = props->getDoubleValue("building-medium-ratio", 0.15);
    building_large_ratio =  props->getDoubleValue("building-large-ratio", 0.05);

    building_small_pitch =  props->getDoubleValue("building-small-pitch", 0.8);
    building_medium_pitch =  props->getDoubleValue("building-medium-pitch", 0.2);
    building_large_pitch =  props->getDoubleValue("building-large-pitch", 0.1);

    building_small_min_floors = props->getIntValue("building-small-min-floors", 1);
    building_small_max_floors = props->getIntValue("building-small-max-floors", 3);
    building_medium_min_floors = props->getIntValue("building-medium-min-floors", 3);
    building_medium_max_floors = props->getIntValue("building-medium-max-floors", 8);
    building_large_min_floors = props->getIntValue("building-large-min-floors", 5);
    building_large_max_floors = props->getIntValue("building-large-max-floors", 20);

    building_small_min_width = props->getFloatValue("building-small-min-width-m", 15.0);
    building_small_max_width = props->getFloatValue("building-small-max-width-m", 60.0);
    building_small_min_depth = props->getFloatValue("building-small-min-depth-m", 10.0);
    building_small_max_depth = props->getFloatValue("building-small-max-depth-m", 20.0);

    building_medium_min_width = props->getFloatValue("building-medium-min-width-m", 25.0);
    building_medium_max_width = props->getFloatValue("building-medium-max-width-m", 50.0);
    building_medium_min_depth = props->getFloatValue("building-medium-min-depth-m", 20.0);
    building_medium_max_depth = props->getFloatValue("building-medium-max-depth-m", 50.0);

    building_large_min_width = props->getFloatValue("building-large-min-width-m", 50.0);
    building_large_max_width = props->getFloatValue("building-large-max-width-m", 75.0);
    building_large_min_depth = props->getFloatValue("building-large-min-depth-m", 50.0);
    building_large_max_depth = props->getFloatValue("building-large-max-depth-m", 75.0);

    building_range = props->getDoubleValue("building-range-m", default_object_range);

    // There are some constraints on the maximum building size that we can sensibly render.
    // Using values outside these ranges will result in the texture being stretched to fit,
    // which may not be desireable.  We will allow it, but display warnings.
    // We do not display warnings for large buildings as we assume the textures are sufficiently
    // generic to be stretched without problems.
    if (building_small_max_floors  >    3) SG_LOG(SG_GENERAL, SG_ALERT, "building-small-max-floors exceeds maximum (3). Texture will be stretched to fit.");
    if (building_medium_max_floors >    8) SG_LOG(SG_GENERAL, SG_ALERT, "building-medium-max-floors exceeds maximum (8). Texture will be stretched to fit.");
    if (building_large_max_floors  >   22) SG_LOG(SG_GENERAL, SG_ALERT, "building-large-max-floors exceeds maximum (22). Texture will be stretched to fit.");
    if (building_small_max_width   > 192.0) SG_LOG(SG_GENERAL, SG_ALERT, "building-small-max-width-m exceeds maximum (192). Texture will be stretched to fit.");
    if (building_small_max_depth   > 192.0) SG_LOG(SG_GENERAL, SG_ALERT, "building-small-max-depth-m exceeds maximum (192). Texture will be stretched to fit.");
    if (building_medium_max_width  > 80.0) SG_LOG(SG_GENERAL, SG_ALERT, "building-medium-max-width-m exceeds maximum (80). Texture will be stretched to fit.");
    if (building_medium_max_depth  > 80.0) SG_LOG(SG_GENERAL, SG_ALERT, "building-medium-max-depth-m exceeds maximum (80). Texture will be stretched to fit.");

    cos_object_max_density_slope_angle  = cos(props->getFloatValue("object-max-density-angle-deg", 20.0) * osg::PI/180.0);
    cos_object_zero_density_slope_angle = cos(props->getFloatValue("object-zero-density-angle-deg", 30.0) * osg::PI/180.0);

    // Random vegetation properties
    wood_coverage = props->getDoubleValue("wood-coverage", 0.0);
    is_plantation = props->getBoolValue("plantation",false);
    tree_effect = props->getStringValue("tree-effect", "Effects/tree");
    tree_height = props->getDoubleValue("tree-height-m", 0.0);
    tree_width = props->getDoubleValue("tree-width-m", 0.0);
    tree_range = props->getDoubleValue("tree-range-m", default_object_range);
    tree_varieties = props->getIntValue("tree-varieties", 1);
    cos_tree_max_density_slope_angle  = cos(props->getFloatValue("tree-max-density-angle-deg", 30.0) * osg::PI/180.0);
    cos_tree_zero_density_slope_angle = cos(props->getFloatValue("tree-zero-density-angle-deg", 45.0) * osg::PI/180.0);

    const SGPropertyNode* treeTexNode = props->getChild("tree-texture");

    if (treeTexNode) {
        std::string treeTexPath = props->getStringValue("tree-texture");

        if (! treeTexPath.empty()) {
            SGPath treePath("Textures");
            treePath.append(treeTexPath);
            tree_texture = SGModelLib::findDataFile(treePath, options);

            if (tree_texture.empty()) {
                SG_LOG(SG_GENERAL, SG_ALERT, "Cannot find texture \""
                        << treeTexPath << "\" in Textures folders.");
            }
        }
    }

    // surface values for use with ground reactions
    _solid = props->getBoolValue("solid", _solid);
    _friction_factor = props->getDoubleValue("friction-factor", _friction_factor);
    _rolling_friction = props->getDoubleValue("rolling-friction", _rolling_friction);
    _bumpiness = props->getDoubleValue("bumpiness", _bumpiness);
    _load_resistance = props->getDoubleValue("load-resistance", _load_resistance);

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
        object_groups.push_back(new SGMatModelGroup(object_group_nodes[i], default_object_range));

    // read glyph table for taxi-/runway-signs
    std::vector<SGPropertyNode_ptr> glyph_nodes = props->getChildren("glyph");
    for (unsigned int i = 0; i < glyph_nodes.size(); i++) {
        const char *name = glyph_nodes[i]->getStringValue("name");
        if (name)
            glyphs[name] = new SGMaterialGlyph(glyph_nodes[i]);
    }

    // Read parameters entry, which is passed into the effect
    if (props->hasChild("parameters")) {
        parameters = props->getChild("parameters");
    } else {
        parameters = new SGPropertyNode();
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
    building_coverage = 0.0;

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
        if (!_status[i].effect.valid())
            return 0;
        _status[i].effect->realizeTechniques(_status[i].options.get());
        _status[i].effect_realized = true;
    }
    return _status[i].effect.get();
}

Effect* SGMaterial::get_one_effect(int texIndex)
{
    std::lock_guard<std::mutex> g(_lock);
    if (_status.empty()) {
        SG_LOG( SG_GENERAL, SG_WARN, "No effect available.");
        return 0;
    }

    int i = texIndex % _status.size();
    return get_effect(i);
}

Effect* SGMaterial::get_effect()
{
    std::lock_guard<std::mutex> g(_lock);
    return get_effect(0);
}


osg::Texture2D* SGMaterial::get_one_object_mask(int texIndex)
{
    if (_status.empty()) {
        SG_LOG( SG_GENERAL, SG_WARN, "No mask available.");
        return 0;
    }

    // Note that the object mask is closely linked to the texture/effect
    // so we index based on the texture index,
    unsigned int i = texIndex % _status.size();
    if (i < _masks.size()) {
        return _masks[i].get();
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
    copyProperties(parameters, paramProp);

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
    for (auto& matState : _status) {
        SGPropertyNode_ptr effectProp = new SGPropertyNode();
        copyProperties(propRoot, effectProp);
        SGPropertyNode* effectParamProp = effectProp->getChild("parameters", 0);
        for (unsigned int i = 0; i < matState.texture_paths.size(); i++) {
            SGPropertyNode* texProp = makeChild(effectParamProp, "texture", matState.texture_paths[i].second);
            makeChild(texProp, "image")->setStringValue(matState.texture_paths[i].first);
            makeChild(texProp, "filter")
                ->setStringValue(mipmap ? "linear-mipmap-linear" : "nearest");
            makeChild(texProp, "wrap-s")
                ->setStringValue(wrapu ? "repeat" : "clamp-to-edge");
            makeChild(texProp, "wrap-t")
                ->setStringValue(wrapv ? "repeat" : "clamp-to-edge");
        }
        makeChild(effectParamProp, "xsize")->setDoubleValue(xsize);
        makeChild(effectParamProp, "ysize")->setDoubleValue(ysize);
        makeChild(effectParamProp, "scale")->setValue(SGVec3d(xsize,ysize,0.0));
        makeChild(effectParamProp, "light-coverage")->setDoubleValue(light_coverage);

        matState.effect = makeEffect(effectProp, false, options);
        if (matState.effect.valid())
            matState.effect->setUserData(user.get());
    }
}

SGMaterialGlyph* SGMaterial::get_glyph (const std::string& name) const
{
    map<std::string, SGSharedPtr<SGMaterialGlyph> >::const_iterator it;
    it = glyphs.find(name);
    if (it == glyphs.end())
        return 0;

    return it->second;
}

bool SGMaterial::valid(SGVec2f loc) const
{
	SG_LOG( SG_TERRAIN, SG_BULK, "Checking materials for location ("
			<< loc.x() << ","
			<< loc.y() << ")");

	// Check location first again the areas the material is valid for
	AreaList::const_iterator i = areas->begin();

	if (i == areas->end()) {
		// No areas defined, so simply check against condition
		if (condition) {
			return condition->test();
		} else {
			return true;
		}
	}

	for (; i != areas->end(); i++) {

		SG_LOG( SG_TERRAIN, SG_BULK, "Checking area ("
				<< i->x() << ","
				<< i->y() << ") width:"
				<< i->width() << " height:"
				<< i->height());
		// Areas defined, so check that the tile location falls within it
		// before checking against condition
		if (i->contains(loc.x(), loc.y())) {
			if (condition) {
				return condition->test();
			} else {
				return true;
			}
		}
	}

	return false;
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
