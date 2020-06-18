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
void ShaderGeometry::addObject(const Vec3& position, float scale,
                               int texture_index)
{
    if (!_posScaleArray.valid()) {
        _posScaleArray = new Vec4Array();
        _vertexAttribArray = new FloatArray();
    }
    _posScaleArray->push_back(Vec4(position, scale));
    _vertexAttribArray->push_back((float)texture_index / varieties);
    dirtyBound();
}

void ShaderGeometry::drawImplementation(osg::RenderInfo& renderInfo) const
{
    State& state = *renderInfo.getState();
    const GLExtensions* extensions = GLExtensions::Get(state.getContextID(), true);

    Vec4Array::const_iterator citer = _posScaleArray->begin();
    Vec4Array::const_iterator cend = _posScaleArray->end();
    FloatArray::const_iterator viter = _vertexAttribArray->begin();
    for (; citer != cend; ++citer, ++viter) {
        const Vec4& color = *citer;
        const float attrib = *viter;
        glColor4fv(color.ptr());
        extensions->glVertexAttrib1f(1, attrib);
        _geometry->draw(renderInfo);
    }
}

BoundingBox ShaderGeometry::computeBoundingBox() const
{
    const BoundingBox& geom_box = _geometry->getBoundingBox();

    BoundingBox bb;
    const Vec4Array* posScales = _posScaleArray.get();
    if (!posScales)
        return bb;
//    size_t numPosScales = posScales->size();
    for (Vec4Array::const_iterator iter = _posScaleArray->begin(),
             e = _posScaleArray->end();
         iter != e;
         ++iter) {
        const Vec4& posScale = *iter;
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
//    int capacity = 0;
    if (fr.matchSequence("posScale %i {")) {
        int entry = fr[1].getNoNestedBrackets();
        int capacity;
        fr[1].getInt(capacity);
        Vec4Array* posScale = new Vec4Array;
        posScale->reserve(capacity);
        fr += 3;
        while (!fr.eof() && fr[0].getNoNestedBrackets() > entry) {
            Vec4 v;
            if (fr[0].getFloat(v.x()) && fr[1].getFloat(v.y())
                && fr[2].getFloat(v.z()) && fr[3].getFloat(v.w())) {
                fr += 4;
                posScale->push_back(v);
            }
            else ++fr;
        }
        ++fr;
        geom._posScaleArray = posScale;
    }
    if (fr.matchSequence("variety %i {")) {
        int entry = fr[1].getNoNestedBrackets();
        int capacity;
        fr[1].getInt(capacity);
        FloatArray* variety = new FloatArray;
        variety->reserve(capacity);
        fr += 3;
        while (!fr.eof() && fr[0].getNoNestedBrackets() > entry) {
            float val;
            if (fr[0].getFloat(val)) {
                ++fr;
                variety->push_back(val);
            }
            else ++fr;
        }
        ++fr;
        geom._vertexAttribArray = variety;
    }

    return iteratorAdvanced;
}

bool ShaderGeometry_writeLocalData(const Object& obj, Output& fw)
{
    const ShaderGeometry& geom = static_cast<const ShaderGeometry&>(obj);

    fw.indent() << "geometry" << std::endl;
    fw.writeObject(*geom._geometry);
    if (geom._posScaleArray.valid()) {
        fw.indent() << "posScale " << geom._posScaleArray->size() << " {\n";
        fw.moveIn();
        for (Vec4Array::const_iterator iter = geom._posScaleArray->begin();
             iter != geom._posScaleArray->end();
             ++iter) {
            fw.indent() << iter->x() << " " << iter->y() << " " << iter->z() << " "
                        << iter->w() << "\n";
        }
        fw.moveOut();
        fw.indent() << "}\n";
    }
    if (geom._vertexAttribArray.valid()) {
        fw.indent() << "variety" << geom._vertexAttribArray->size() << " {\n";
        fw.moveIn();
        for (FloatArray::const_iterator iter = geom._vertexAttribArray->begin();
             iter != geom._vertexAttribArray->end();
             ++iter) {
            fw.indent() << *iter << "\n";
        }
        fw.moveOut();
        fw.indent() << "}\n";
    }
    return true;
}

osgDB::RegisterDotOsgWrapperProxy shaderGeometryProxy
(
    new ShaderGeometry,
    "ShaderGeometry",
    "Object Drawable ShaderGeometry",
    &ShaderGeometry_readLocalData,
    &ShaderGeometry_writeLocalData
    );
}

