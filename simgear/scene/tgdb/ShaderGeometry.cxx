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
    Vec4Array *colors = static_cast<Vec4Array*>(getColorArray());
    FloatArray *vertexAttribs
        = static_cast<FloatArray*>(getVertexAttribArray(1));
    colors->push_back(Vec4(t.position.osg(), t.scale));
    vertexAttribs->push_back((float)t.texture_index / varieties);
    dirtyDisplayList();
    dirtyBound();
}

// The good bits from osg::Geometry::drawImplementation
void ShaderGeometry::drawImplementation(osg::RenderInfo& renderInfo) const
{
    osg::State& state = *renderInfo.getState();
    const Extensions* extensions = getExtensions(state.getContextID(), true);
    state.setVertexPointer(_vertexData.array->getDataSize(),
                           _vertexData.array->getDataType(), 0,
                           _vertexData.array->getDataPointer());
    if (_normalData.array.valid())
        state.setNormalPointer(_normalData.array->getDataType(), 0,
                               _normalData.array->getDataPointer());
    else
        state.disableNormalPointer();
    Array* texArray = _texCoordList[0].array.get();
    state.setTexCoordPointer(0, texArray->getDataSize(),
                             texArray->getDataType(), 0,
                             texArray->getDataPointer());
    const Vec4Array* colors = static_cast<const Vec4Array*>(getColorArray());
    const FloatArray* vertexAttribs
        = static_cast<const FloatArray*>(getVertexAttribArray(1));
    Vec4Array::const_iterator citer = colors->begin(), cend = colors->end();
    FloatArray::const_iterator viter = vertexAttribs->begin();
    const Geometry::PrimitiveSetList& modelSets
        = _geometry->getPrimitiveSetList();
    for (; citer != cend; ++citer, ++viter) {
        const Vec4& color = *citer;
        const float attrib = *viter;
        glColor4fv(color.ptr());
        extensions->glVertexAttrib1f(1, attrib);
        for (Geometry::PrimitiveSetList::const_iterator piter = modelSets.begin(),
                 e = modelSets.end();
             piter != e;
            ++piter)
            (*piter)->draw(state, false);
    }
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

