/* -*-c++-*-
 *
 * Copyright (C) 2024 Fahim Dalvi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <simgear_config.h>
#endif

#include "ObjectInstanceBin.hxx"

#include <osg/Drawable>
#include <osg/Geometry>
#include <osg/NodeVisitor>
#include <osg/StateSet>
#include <osg/VertexAttribDivisor>
#include <osgDB/ReadFile>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/RenderConstants.hxx>

using namespace osg;
namespace simgear {

struct ObjectInstanceBoundingBoxCallback : public Drawable::ComputeBoundingBoxCallback {
    ObjectInstanceBoundingBoxCallback() {}
    ObjectInstanceBoundingBoxCallback(const ObjectInstanceBoundingBoxCallback&, const CopyOp&) {}
    META_Object(simgear, ObjectInstanceBoundingBoxCallback);
    virtual BoundingBox computeBound(const Drawable& drawable) const
    {
        // Get bounding box of a single object
        osg::BoundingBox single_object_bound = drawable.computeBoundingBox();
        float _bounding_diameter = 0.0;
        if (single_object_bound.valid())
            _bounding_diameter = single_object_bound.radius() * 2;

        BoundingBox bb;
        const Geometry* geometry = static_cast<const Geometry*>(&drawable);
        const Vec3Array* instancePositions = static_cast<const Vec3Array*>(geometry->getVertexAttribArray(INSTANCE_POSITIONS));
        const Vec4Array* instanceRotationsAndScales = static_cast<const Vec4Array*>(geometry->getVertexAttribArray(INSTANCE_ROTATIONS_AND_SCALES));

        float maxScale = 1.0f;

        for (unsigned int v = 0; v < instancePositions->size(); ++v) {
            Vec3 pt = (*instancePositions)[v];
            float currentScale = (*instanceRotationsAndScales)[v][3];
            if (currentScale > maxScale) {
                maxScale = currentScale;
            }
            bb.expandBy(pt);
        }

        bb = BoundingBox(bb._min - osg::Vec3(_bounding_diameter * maxScale, _bounding_diameter * maxScale, _bounding_diameter * maxScale),
                         bb._max + osg::Vec3(_bounding_diameter * maxScale, _bounding_diameter * maxScale, _bounding_diameter * maxScale));

        return bb;
    }
};

class InstancingVisitor : public osg::NodeVisitor
{
public:
    typedef std::set<osg::ref_ptr<osg::Drawable>> DrawableSet;
    typedef std::set<osg::ref_ptr<EffectGeode>> EffectGeodeSet;

    InstancingVisitor(osg::Vec3Array* positions, osg::Vec4Array* rotationsAndScales, osg::Vec4Array* customAttribs) : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {
        _positions = positions;
        _rotationsAndScales = rotationsAndScales;
        _customAttribs = customAttribs;
    }

    void setPropsOnDrawable(osg::Drawable* drawable)
    {
        osg::Geometry* geometry = dynamic_cast<osg::Geometry*>(drawable);
        if (!geometry) {
            return;
        }
        geometry->setUseDisplayList(false);
        geometry->setUseVertexBufferObjects(true);
        geometry->setDataVariance(osg::Object::STATIC);
        geometry->setComputeBoundingBoxCallback(new ObjectInstanceBoundingBoxCallback);

        geometry->setVertexAttribArray(INSTANCE_POSITIONS, _positions, Array::BIND_PER_VERTEX, 1);
        geometry->setVertexAttribArray(INSTANCE_ROTATIONS_AND_SCALES, _rotationsAndScales, Array::BIND_PER_VERTEX, 1);
        if (_customAttribs != NULL) {
            geometry->setVertexAttribArray(INSTANCE_CUSTOM_ATTRIBS, _customAttribs, Array::BIND_PER_VERTEX, 1);
        }

        if (geometry->getNumPrimitiveSets() > 0) {
            for (unsigned int i = 0; i < geometry->getNumPrimitiveSets(); i++) {
                osg::DrawArrays* drawArrays = static_cast<DrawArrays*>(geometry->getPrimitiveSet(i));
                drawArrays->setNumInstances(_positions->size());
            }
        }
    }

    virtual void apply(osg::Node& node)
    {
        traverse(node);
    }

    virtual void apply(osg::Geode& node)
    {
        EffectGeode* eg = dynamic_cast<EffectGeode*>(&node);
        if (eg) {
            // Only modify Drawables that have an associated EffectGeode
            for (unsigned int i = 0; i < node.getNumDrawables(); ++i) {
                osg::Drawable* drawable = node.getDrawable(i);
                if (drawable) {
                    _drawableSet.insert(drawable);
                    setPropsOnDrawable(drawable);
                }
            }

            _effectGeodeSet.insert(eg);
        }

        traverse(node);
    }


    int getNumDrawables()
    {
        return _drawableSet.size();
    }

    DrawableSet getDrawables()
    {
        return _drawableSet;
    }

    int getNumEffectGeodes()
    {
        return _effectGeodeSet.size();
    }

    EffectGeodeSet getEffectGeodes()
    {
        return _effectGeodeSet;
    }

private:
    osg::Vec3Array* _positions;
    osg::Vec4Array* _rotationsAndScales;
    osg::Vec4Array* _customAttribs;

    DrawableSet _drawableSet;
    EffectGeodeSet _effectGeodeSet;
};

void ObjectInstanceBin::insert(const ObjectInstance& obj)
{
    _objectInstances.push_back(obj);
}
void ObjectInstanceBin::insert(const SGVec3f& p, const SGVec3f& r, const float& s, const SGVec4f& c)
{
    insert(ObjectInstance(p, r, s, c));
}

unsigned ObjectInstanceBin::getNumInstances() const
{
    return _objectInstances.size();
}

const std::string ObjectInstanceBin::getModelFileName() const
{
    return _modelFileName;
}

const SGPath ObjectInstanceBin::getSTGFilePath() const
{
    return _STGFilePath;
}

const std::string ObjectInstanceBin::getEffect() const
{
    return _effect;
}

const ObjectInstanceBin::ObjectInstance& ObjectInstanceBin::getInstance(unsigned i) const
{
    return _objectInstances[i];
}

const bool ObjectInstanceBin::hasCustomAttributes() const
{
    return (customInstancingEffects.find(_effect) != customInstancingEffects.end());
}

ObjectInstanceBin::ObjectInstanceBin(const std::string modelFileName, const std::string effect, const SGPath& STGFilePath, const SGPath& instancesFilePath)
{
    _modelFileName = modelFileName;
    _STGFilePath = STGFilePath;

    if (effect == "default") {
        _effect = "Effects/object-instancing";
    } else {
        _effect = effect;
    }

    if (instancesFilePath.isNull()) {
        // No parsing to be done, just return
        return;
    }

    sg_gzifstream stream(instancesFilePath);
    if (!stream.is_open()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to open " << instancesFilePath);
        return;
    }

    // Every instanced object is defined by one of the following:
    // - 3 (position)
    // - 4 (position+scale)
    // - 6 (position+rotation)
    // - 7 (position+rotation+scale) props
    // In case of objects defined with an effect specified in the
    //  `customInstancingEffects` set, the above definition is
    //  followed by 4 mandatory elements for custom attributes.
    // Therefore, options become
    // - 7 (position+customAttributes)
    // - 8 (position+scale+customAttributes)
    // - 10 (position+rotation+customAttributes)
    // - 11 (position+rotation+scale+customAttributes)
    float props[11];

    const bool hasCustomAttributes = this->hasCustomAttributes();

    while (!stream.eof()) {
        // read a line.  Each line defines a single instance and its properties,
        // and may have a comment, starting with #
        std::string line;
        std::getline(stream, line);

        // strip comments
        std::string::size_type hash_pos = line.find('#');
        if (hash_pos != std::string::npos)
            line.resize(hash_pos);

        if (line.empty()) {
            continue; // skip blank lines
        }

        // and process further
        std::stringstream in(line);

        int number_of_props = 0;
        while (!in.eof()) {
            in >> props[number_of_props++];
        }

        if (!hasCustomAttributes) {
            if (number_of_props == 3) {
                insert(SGVec3f(props[0], props[1], props[2]));
            } else if (number_of_props == 4) {
                insert(SGVec3f(props[0], props[1], props[2]), SGVec3f(0.0f, 0.0f, 0.0f), props[3]);
            } else if (number_of_props == 6) {
                insert(SGVec3f(props[0], props[1], props[2]), SGVec3f(props[3], props[4], props[5]));
            } else if (number_of_props == 7) {
                insert(SGVec3f(props[0], props[1], props[2]), SGVec3f(props[3], props[4], props[5]), props[6]);
            } else {
                SG_LOG(SG_TERRAIN, SG_WARN, "Error parsing instanced object entry in: " << instancesFilePath << " line: \"" << line << "\"");
                continue;
            }
        } else {
            if (number_of_props == 7) {
                insert(SGVec3f(props[0], props[1], props[2]), SGVec3f(0.0f, 0.0f, 0.0f), 1.0f, SGVec4f(props[number_of_props - 4], props[number_of_props - 3], props[number_of_props - 2], props[number_of_props - 1]));
            } else if (number_of_props == 8) {
                insert(SGVec3f(props[0], props[1], props[2]), SGVec3f(0.0f, 0.0f, 0.0f), props[3], SGVec4f(props[number_of_props - 4], props[number_of_props - 3], props[number_of_props - 2], props[number_of_props - 1]));
            } else if (number_of_props == 10) {
                insert(SGVec3f(props[0], props[1], props[2]), SGVec3f(props[3], props[4], props[5]), 1.0f, SGVec4f(props[number_of_props - 4], props[number_of_props - 3], props[number_of_props - 2], props[number_of_props - 1]));
            } else if (number_of_props == 11) {
                insert(SGVec3f(props[0], props[1], props[2]), SGVec3f(props[3], props[4], props[5]), props[6], SGVec4f(props[number_of_props - 4], props[number_of_props - 3], props[number_of_props - 2], props[number_of_props - 1]));
            } else {
                SG_LOG(SG_TERRAIN, SG_WARN, "Error parsing instanced object entry in: " << instancesFilePath << " line: \"" << line << "\"");
                continue;
            }
        }
    }

    stream.close();
}

// From ReaderWriterSTG.cxx
SGReaderWriterOptions* sharedOptions(const std::string& filePath, const osgDB::Options* options)
{
    osg::ref_ptr<SGReaderWriterOptions> sharedOptions;
    sharedOptions = SGReaderWriterOptions::copyOrCreate(options);
    sharedOptions->getDatabasePathList().clear();

    if (!filePath.empty()) {
        SGPath path = filePath;
        path.append("..");
        path.append("..");
        path.append("..");
        sharedOptions->getDatabasePathList().push_back(path.utf8Str());
    }

    // ensure Models directory synced via TerraSync is searched before the copy in
    // FG_ROOT, so that updated models can be used.
    std::string terrasync_root = options->getPluginStringData("SimGear::TERRASYNC_ROOT");
    if (!terrasync_root.empty())
        sharedOptions->getDatabasePathList().push_back(terrasync_root);

    std::string fg_root = options->getPluginStringData("SimGear::FG_ROOT");
    sharedOptions->getDatabasePathList().push_back(fg_root);

    // TODO how should we handle this for OBJECT_SHARED?
    sharedOptions->setModelData(
        sharedOptions->getModelData()
            ? sharedOptions->getModelData()->clone()
            : 0);

    return sharedOptions.release();
}

osg::ref_ptr<osg::Node> createObjectInstances(ObjectInstanceBin& objectInstances, const osg::Matrix& transform, const osg::ref_ptr<SGReaderWriterOptions> options)
{
    // Setup options for instancing with the correct effect
    // Options are shared-objects like
    osg::ref_ptr<SGReaderWriterOptions> opt;


    opt = sharedOptions(objectInstances.getSTGFilePath().dir(), options);

    if (SGPath(objectInstances.getModelFileName()).lower_extension() == "ac")
        opt->setInstantiateEffects(true);
    else
        opt->setInstantiateEffects(false);

    opt->setDefaultEffect(objectInstances.getEffect());
    opt->setObjectCacheHint(osgDB::Options::CACHE_NONE);

    const bool hasCustomAttributes = objectInstances.hasCustomAttributes();

    if (objectInstances.getNumInstances() <= 0) {
        SG_LOG(SG_TERRAIN, SG_ALERT, objectInstances.getSTGFilePath() << " has zero instances.");
        return 0;
    }

    // Load the model to be instanced
    osg::ref_ptr<osg::Node> model = osgDB::readRefNodeFile(objectInstances.getModelFileName(), opt.get());

    if (!model.valid()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, objectInstances.getSTGFilePath() << ": Failed to load '" << objectInstances.getModelFileName() << "'");
        return NULL;
    }

    if (SGPath(objectInstances.getModelFileName()).lower_extension() == "ac")
        model->setNodeMask(~simgear::MODELLIGHT_BIT);

    if (SGPath(objectInstances.getModelFileName()).lower_extension() == "xml") {
        SG_LOG(SG_TERRAIN, SG_WARN, objectInstances.getSTGFilePath() << ": Models "
                                                                     << "defined using XML files ('" << objectInstances.getModelFileName() << "' are not supported for instancing. Instances may not be rendered "
                                                                     << "correctly");
    }

    // Get parameters for instances
    osg::Vec3Array* positions = new osg::Vec3Array;
    osg::Vec4Array* rotationsAndScales = new osg::Vec4Array;
    osg::Vec4Array* customAttribs = NULL;
    if (hasCustomAttributes) {
        customAttribs = new osg::Vec4Array;
    }

    for (unsigned int objectIdx = 0; objectIdx < objectInstances.getNumInstances(); objectIdx++) {
        const auto obj = objectInstances.getInstance(objectIdx);
        positions->push_back(toOsg(obj.position) * transform);
        rotationsAndScales->push_back(toOsg(SGVec4f(obj.rotation[0], obj.rotation[1], obj.rotation[2], obj.scale)));
        if (hasCustomAttributes) {
            // Pass custom attributes
            customAttribs->push_back(toOsg(obj.customAttribs));
        }
    }

    // Modify loaded model with instancing parameters
    InstancingVisitor visitor(positions, rotationsAndScales, customAttribs);
    model->accept(visitor);

    if (visitor.getNumDrawables() > 1) {
        SG_LOG(SG_TERRAIN, SG_WARN, objectInstances.getSTGFilePath() << ": Model '" << objectInstances.getModelFileName() << "' has more than one drawable. "
                                                                     << "It is highly recommended that models used for instancing have "
                                                                     << "only one drawable/object.");
    }

    return model;
}

}; // namespace simgear
