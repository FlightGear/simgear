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
#include <simgear/screen/extensions.hxx>

#include "ShaderGeometry.hxx"

using namespace osg;
using namespace osgDB;

namespace simgear
{
void ShaderGeometry::drawImplementation(RenderInfo& renderInfo) const
{
    osg::State& state = *renderInfo.getState();
    const Extensions* extensions = getExtensions(state.getContextID(),true);

    for(TreeBin::TreeList::const_iterator t = _trees.begin(); t != _trees.end(); ++t)
    {
        extensions->glVertexAttrib1f(1, (float) t->texture_index/varieties);
        glColor4f(t->position.x(), t->position.y(), t->position.z(), t->scale);
        _geometry->draw(renderInfo);
    }
}

BoundingBox ShaderGeometry::computeBound() const
{
    BoundingBox geom_box = _geometry->getBound();
    BoundingBox bb;
    for(TreeBin::TreeList::const_iterator itr = _trees.begin();
        itr != _trees.end();
        ++itr) {
         bb.expandBy(geom_box.corner(0)*itr->scale +
                     osg::Vec3( itr->position.x(), itr->position.y(), itr->position.z() ));
         bb.expandBy(geom_box.corner(7)*itr->scale +
                     osg::Vec3( itr->position.x(), itr->position.y(), itr->position.z() ));
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
        osg::Drawable* drawable = fr.readDrawable();
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
            SGVec3f v;
            int t;
            float s;
            if (fr[0].getFloat(v.x()) && fr[1].getFloat(v.y())
                && fr[2].getFloat(v.z()) && fr[3].getInt(t) && fr[4].getFloat(s)) {
                    fr += 4;
                    //SGVec3f* v = new SGVec3f(v.x(), v.y(), v.z());
                    //TreeBin::Tree tree = new TreeBin::Tree(v, t, s);
                    geom._trees.push_back(TreeBin::Tree(v, t, s));
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
    for (TreeBin::TreeList::const_iterator iter
             = geom._trees.begin();
         iter != geom._trees.end();
         ++iter) {
        fw.indent() << iter->position.x() << " " << iter->position.y() << " " << iter->position.z() << " "
                    << iter->texture_index << " " << iter->scale << std::endl;
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
