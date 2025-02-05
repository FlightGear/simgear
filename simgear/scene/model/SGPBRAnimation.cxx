/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Fernando García Liñán
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SGPBRAnimation.hxx"

#include <osgDB/FileUtils>

#include <simgear/misc/inputcolor.hxx>
#include <simgear/misc/inputvalue.hxx>
#include <simgear/scene/model/ConditionNode.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>

namespace {

void loadTextureAndApplyToStateSet(osg::StateSet* ss,
                                   unsigned int unit,
                                   const std::string& texture_name,
                                   const osgDB::FilePathList& texture_path_list)
{
    std::string texture_file = osgDB::findFileInPath(texture_name, texture_path_list);
    if (!texture_file.empty()) {
        osg::Texture2D* texture2D = SGLoadTexture2D(texture_file);
        if (texture2D) {
            ss->setTextureAttributeAndModes(unit, texture2D, osg::StateAttribute::ON
                                            | osg::StateAttribute::OVERRIDE);
        }
    } else {
        SG_LOG(SG_IO, SG_DEV_WARN, "PBR animation: requested texture '"
               << texture_name << "' not found. Searched paths:");
        for (const auto &path : texture_path_list) {
            SG_LOG(SG_IO, SG_DEV_WARN, " - " << path);
        }
    }
}

void removeTextureFromStateSet(osg::StateSet* ss, unsigned int unit)
{
    while (ss->getTextureAttribute(unit, osg::StateAttribute::TEXTURE)) {
        ss->removeTextureAttribute(unit, osg::StateAttribute::TEXTURE);
    }
}

} // anonymous namespace

class UpdateCallback : public osg::NodeCallback {
public:
    UpdateCallback(const osgDB::FilePathList& texture_path_list,
                   const SGPropertyNode* config, SGPropertyNode* model_root) :
        _texture_path_list(texture_path_list)
    {
        // NOTE: The texture units that correspond to each texture type (e.g.
        // 0 for base color, 1 for normal map, etc.) must match the ones in:
        //  1. PBR Effect: $FG_ROOT/Effects/model-pbr.eff
        //  2. glTF loader: simgear/scene/model/ReaderWriterGLTF.cxx
        //  3. PBR animations: simgear/scene/model/SGPBRAnimation.cxx
        //  4. Canvas: flightgear/src/Canvas/texture_replace.cxx
        buildTextureEntry(config, model_root, 0, "base-color-texture-prop");
        buildTextureEntry(config, model_root, 1, "normalmap-texture-prop");
        buildTextureEntry(config, model_root, 2, "orm-texture-prop");
        buildTextureEntry(config, model_root, 3, "emissive-texture-prop");

        const SGPropertyNode* node;
        node = config->getChild("base-color-factor");
        if (node) {
            _base_color_factor_value = new simgear::RGBAColorValue(*model_root, *(const_cast<SGPropertyNode*>(node)));
            _base_color_factor_uniform = new osg::Uniform(osg::Uniform::FLOAT_VEC4, "base_color_factor");
        }
        node = config->getChild("metallic-factor");
        if (node) {
            _metallic_factor_value = new simgear::Value(*model_root, *(const_cast<SGPropertyNode*>(node)));
            _metallic_factor_uniform = new osg::Uniform(osg::Uniform::FLOAT, "metallic_factor");
        }
        node = config->getChild("roughness-factor");
        if (node) {
            _roughness_factor_value = new simgear::Value(*model_root, *(const_cast<SGPropertyNode*>(node)));
            _roughness_factor_uniform = new osg::Uniform(osg::Uniform::FLOAT, "roughness_factor");
        }
        node = config->getChild("emissive-factor");
        if (node) {
            _emissive_factor_value = new simgear::RGBColorValue(*model_root, *(const_cast<SGPropertyNode*>(node)));
            _emissive_factor_uniform = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "emissive_factor");
        }
    }

    void operator()(osg::Node* node, osg::NodeVisitor* nv) override
    {
        osg::StateSet* ss = node->getOrCreateStateSet();

        // Add the uniforms to the StateSet on the first run
        if (!_initialized) {
            if (_base_color_factor_uniform) {
                ss->addUniform(_base_color_factor_uniform, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }
            if (_metallic_factor_uniform) {
                ss->addUniform(_metallic_factor_uniform, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }
            if (_roughness_factor_uniform) {
                ss->addUniform(_roughness_factor_uniform, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }
            if (_emissive_factor_uniform) {
                ss->addUniform(_emissive_factor_uniform, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }
            _initialized = true;
        }

        // Update the uniform values. It is safe to evaluate expressions and
        // conditions here because we are inside an update callback.
        if (_base_color_factor_uniform && _base_color_factor_value.valid()) {
            _base_color_factor_uniform->set(toOsg(_base_color_factor_value->get_value()));
        }
        if (_metallic_factor_uniform && _metallic_factor_value.valid()) {
            _metallic_factor_uniform->set(float(_metallic_factor_value->get_value()));
        }
        if (_roughness_factor_uniform && _roughness_factor_value.valid()) {
            _roughness_factor_uniform->set(float(_roughness_factor_value->get_value()));
        }
        if (_emissive_factor_uniform && _emissive_factor_value.valid()) {
            _emissive_factor_uniform->set(toOsg(_emissive_factor_value->get_value()));
        }

        // Check if any textures need updating
        for (auto& texture : _textures) {
            std::string name = texture.prop->getStringValue();
            if (name != texture.name) {
                // The texture name has changed. Load this new texture from disk
                // and apply it to the model. Also update the cached name.
                texture.name = name;
                removeTextureFromStateSet(ss, texture.unit);
                loadTextureAndApplyToStateSet(ss, texture.unit, name, _texture_path_list);
            }
        }

        traverse(node, nv);
    }
private:
    struct TextureEntry {
        unsigned int unit;
        SGPropertyNode* prop;
        std::string name;
    };

    void buildTextureEntry(const SGPropertyNode* config, SGPropertyNode* model_root,
                           unsigned int unit, const std::string& prop_name)
    {
        const SGPropertyNode* node = config->getChild(prop_name);
        if (node) {
            SGPropertyNode* prop = model_root->getNode(node->getStringValue(), true);
            TextureEntry texture{unit, prop, ""};
            _textures.push_back(texture);
        }
    }

    bool _initialized{false};

    std::vector<TextureEntry> _textures;
    osgDB::FilePathList _texture_path_list;

    osg::ref_ptr<osg::Uniform> _base_color_factor_uniform;
    osg::ref_ptr<osg::Uniform> _metallic_factor_uniform;
    osg::ref_ptr<osg::Uniform> _roughness_factor_uniform;
    osg::ref_ptr<osg::Uniform> _emissive_factor_uniform;

    simgear::RGBAColorValue_ptr _base_color_factor_value;
    simgear::Value_ptr _metallic_factor_value;
    simgear::Value_ptr _roughness_factor_value;
    simgear::RGBColorValue_ptr _emissive_factor_value;
};

SGPBRAnimation::SGPBRAnimation(simgear::SGTransientModelData& modelData) :
    SGAnimation(modelData),
    _texture_path_list(modelData.getOptions()->getDatabasePathList())
{
    // add model directory to texture path - this allows short paths relative to
    // model root.
    SGPath modelDir(_modelData.getPath());
    _texture_path_list.insert(_texture_path_list.begin(), modelDir.dir());
}

SGPBRAnimation::~SGPBRAnimation() = default;

osg::Group* SGPBRAnimation::createAnimationGroup(osg::Group& parent)
{
    osg::Group* group = new osg::Group;
    group->setName("PBR animation group");
    SGSceneUserData::getOrCreateSceneUserData(group)->setLocation(getConfig()->getLocation());

    // NOTE: The texture units that correspond to each texture type (e.g.
    // 0 for base color, 1 for normal map, etc.) must match the ones in:
    //  1. PBR Effect: $FG_ROOT/Effects/model-pbr.eff
    //  2. glTF loader: simgear/scene/model/ReaderWriterGLTF.cxx
    //  3. PBR animations: simgear/scene/model/SGPBRAnimation.cxx
    osg::StateSet* ss = group->getOrCreateStateSet();
    const SGPropertyNode* texture_node;
    texture_node = getConfig()->getChild("base-color-texture");
    if (texture_node) {
        loadTextureAndApplyToStateSet(ss, 0, texture_node->getStringValue(), _texture_path_list);
    }
    texture_node = getConfig()->getChild("normalmap-texture");
    if (texture_node) {
        loadTextureAndApplyToStateSet(ss, 1, texture_node->getStringValue(), _texture_path_list);
    }
    texture_node = getConfig()->getChild("orm-texture");
    if (texture_node) {
        loadTextureAndApplyToStateSet(ss, 2, texture_node->getStringValue(), _texture_path_list);
    }
    texture_node = getConfig()->getChild("emissive-texture");
    if (texture_node) {
        loadTextureAndApplyToStateSet(ss, 3, texture_node->getStringValue(), _texture_path_list);
    }

    SGPropertyNode* input_root = getModelRoot();
    const SGPropertyNode* prop_base_node = getConfig()->getChild("property-base");
    if (prop_base_node) {
        input_root = getModelRoot()->getNode(prop_base_node->getStringValue(), true);
    }

    group->getOrCreateStateSet()->setDataVariance(osg::Object::DYNAMIC);
    group->setUpdateCallback(new UpdateCallback(_texture_path_list, getConfig(), input_root));

    if (getCondition()) {
        simgear::ConditionNode* cn = new simgear::ConditionNode;
        cn->setCondition(getCondition());
        osg::Group* modelGroup = new osg::Group;
        group->addChild(modelGroup);
        cn->addChild(group);
        cn->addChild(modelGroup);
        parent.addChild(cn);
        return modelGroup;
    } else {
        parent.addChild(group);
        return group;
    }
}
