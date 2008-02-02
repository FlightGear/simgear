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

using namespace osg;
using namespace osgDB;

namespace simgear
{
void ShaderGeometry::drawImplementation(RenderInfo& renderInfo) const
{
    for(PositionSizeList::const_iterator itr = _trees.begin();
        itr != _trees.end();
        ++itr) {
        glColor4fv(itr->ptr());
        _geometry->draw(renderInfo);
    }
}

BoundingBox ShaderGeometry::computeBound() const
{
    BoundingBox geom_box = _geometry->getBound();
    BoundingBox bb;
    for(PositionSizeList::const_iterator itr = _trees.begin();
        itr != _trees.end();
        ++itr) {
        bb.expandBy(geom_box.corner(0)*(*itr)[3] +
                    Vec3((*itr)[0], (*itr)[1], (*itr)[2]));
        bb.expandBy(geom_box.corner(7)*(*itr)[3] +
                    Vec3((*itr)[0], (*itr)[1], (*itr)[2]));
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
        Drawable* drawable = fr.readDrawable();
        if (drawable) {
            geom._geometry = drawable;
        }
    }
    if ((fr.matchSequence("instances %i"))) {
        int entry = fr[0].getNoNestedBrackets();
        int capacity;
        fr[1].getInt(capacity);
        geom._trees.reserve(capacity);
        fr += 3;
        iteratorAdvanced = true;
        // skip {
        while (!fr.eof() && fr[0].getNoNestedBrackets() > entry) {
            Vec4 v;
            if (fr[0].getFloat(v.x()) && fr[1].getFloat(v.y())
                && fr[2].getFloat(v.z()) && fr[3].getFloat(v.w())) {
                    fr += 4;
                    geom._trees.push_back(v);
            } else {
                ++fr;
            }
        }
    }
    return iteratorAdvanced;
}

bool ShaderGeometry_writeLocalData(const Object& obj, Output& fw)
{
    const ShaderGeometry& geom = static_cast<const ShaderGeometry&>(obj);

    fw.indent() << "geometry" << std::endl;
    fw.writeObject(*geom._geometry);
    fw.indent() << "instances " << geom._trees.size() << std::endl;
    fw.indent() << "{" << std::endl;
    fw.moveIn();
    for (ShaderGeometry::PositionSizeList::const_iterator iter
             = geom._trees.begin();
         iter != geom._trees.end();
         ++iter) {
        fw.indent() << iter->x() << " " << iter->y() << " " << iter->z() << " "
                    << iter->w() << std::endl;
    }
    fw.moveOut();
    fw.indent() << "}" << std::endl;
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
