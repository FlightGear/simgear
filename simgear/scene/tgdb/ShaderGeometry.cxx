/* -*-c++-*-
 *
 * Copyright (C) 2008 Stuart Buchanan
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

#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/ParameterOutput>

#include "ShaderGeometry.hxx"

#include <algorithm>

using namespace osg;
using namespace osgDB;

namespace simgear
{
void ShaderGeometry::addTree(const TreeBin::Tree& t)
{
    if (!getVertexArray()) {
        setVertexData(_geometry->getVertexData());
        setNormalData(_geometry->getNormalData());
        setTexCoordData(0, _geometry->getTexCoordData(0));
        setColorArray(new Vec4Array());
        setColorBinding(Geometry::BIND_PER_PRIMITIVE_SET);
        setVertexAttribArray(1, new FloatArray());
        setVertexAttribBinding(1, Geometry::BIND_PER_PRIMITIVE_SET);
    }
    Geometry::PrimitiveSetList& modelSets = _geometry->getPrimitiveSetList();
    size_t numSets = modelSets.size();
    Vec4Array *colors = static_cast<Vec4Array*>(getColorArray());
    FloatArray *vertexAttribs
        = static_cast<FloatArray*>(getVertexAttribArray(1));
    colors->insert(colors->end(), numSets, Vec4(t.position.osg(), t.scale));
    vertexAttribs->insert(vertexAttribs->end(), numSets,
                          (float)t.texture_index / varieties);
    _primitives.insert(_primitives.end(), modelSets.begin(), modelSets.end());
    dirtyDisplayList();
    dirtyBound();
}

BoundingBox ShaderGeometry::computeBound() const
{
    BoundingBox geom_box = _geometry->getBound();
    BoundingBox bb;
    unsigned numSets = _geometry->getNumPrimitiveSets();
    const Vec4Array* posScales = static_cast<const Vec4Array*>(getColorArray());
    if (!posScales)
        return bb;
    size_t numPosScales = posScales->size();
    for (int i = 0; i < numPosScales; i += numSets) {
        const Vec4& posScale((*posScales)[i]);
        const float scale = posScale.w();
        const Vec3 pos(posScale.x(), posScale.y(), posScale.z());
        for (unsigned j = 0; j < 7; ++j)
            bb.expandBy(geom_box.corner(j) * scale + pos);
    }
    return bb;
}

bool ShaderGeometry_readLocalData(Object& obj, Input& fr)
{
    bool iteratorAdvanced = false;

    ShaderGeometry& geom = static_cast<ShaderGeometry&>(obj);

    if ((fr[0].matchWord("geometry"))) {
        ++fr;
        iteratorAdvanced = true;
        osg::Geometry* drawable = dynamic_cast<osg::Geometry*>(fr.readDrawable());
        if (drawable) {
            geom._geometry = drawable;
        }
    }
    return iteratorAdvanced;
}

bool ShaderGeometry_writeLocalData(const Object& obj, Output& fw)
{
    const ShaderGeometry& geom = static_cast<const ShaderGeometry&>(obj);

    fw.indent() << "geometry" << std::endl;
    fw.writeObject(*geom._geometry);
    return true;
}

osgDB::RegisterDotOsgWrapperProxy shaderGeometryProxy
(
    new ShaderGeometry,
    "ShaderGeometry",
    "Object Drawable Geometry ShaderGeometry",
    &ShaderGeometry_readLocalData,
    &ShaderGeometry_writeLocalData
    );
}

