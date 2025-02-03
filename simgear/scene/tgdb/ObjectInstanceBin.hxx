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

#ifndef OBJECT_INSTANCE_BIN_HXX
#define OBJECT_INSTANCE_BIN_HXX

#include <osg/Node>
#include <simgear/math/SGVec3.hxx>
#include <simgear/math/SGVec4.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

// these correspond to object-instancing*.eff
const int INSTANCE_POSITIONS = 6;            // (x,y,z)
const int INSTANCE_ROTATIONS_AND_SCALES = 7; // (hdg, pitch, roll, scale)
const int INSTANCE_CUSTOM_ATTRIBS = 10;

namespace simgear {
class ObjectInstanceBin final
{
public:
    struct ObjectInstance {
        // Object with position, rotation scale and customAttribs
        ObjectInstance(const SGVec3f& p, const SGVec3f& r = SGVec3f(0.0f, 0.0f, 0.0f), const float& s = 1.0f, const SGVec4f& c = SGVec4f(0.0f, 0.0f, 0.0f, 0.0f)) : position(p), rotation(r), scale(s), customAttribs(c) {}

        SGVec3f position;
        SGVec3f rotation; // hdg, pitch, roll
        float scale;
        SGVec4f customAttribs;
    };

    typedef std::vector<ObjectInstance> ObjectInstanceList;

    ObjectInstanceBin() = default;
    ObjectInstanceBin(const std::string modelFileName, const std::string effect = "Effects/model-pbr", const SGPath& STGFilePath = SGPath("dynamically-generated"), const SGPath& instancesFilePath = SGPath());

    ~ObjectInstanceBin() = default;     // non-virtual intentional

    void insert(const ObjectInstance& light);
    void insert(const SGVec3f& p, const SGVec3f& r = SGVec3f(0.0f, 0.0f, 0.0f), const float& s = 1.0f, const SGVec4f& c = SGVec4f(0.0f, 0.0f, 0.0f, 0.0f));

    const std::string getModelFileName() const;
    const SGPath getSTGFilePath() const;
    const std::string getEffect() const;
    unsigned getNumInstances() const;
    const ObjectInstance& getInstance(unsigned i) const;
    const bool hasCustomAttributes() const;

private:
    SGPath _STGFilePath;
    std::string _modelFileName;
    std::string _effect;
    ObjectInstanceList _objectInstances;

    // List of effects that take extra custom attributes
    const std::set<std::string> customInstancingEffects = {"Effects/object-instancing-colored"};
};

osg::ref_ptr<osg::Node> createObjectInstances(ObjectInstanceBin& objectInstances, const osg::Matrix& transform, const osg::ref_ptr<SGReaderWriterOptions> options);
}; // namespace simgear
#endif
