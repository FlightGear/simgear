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

#include "CloudShaderGeometry.hxx"

#include <simgear/props/props.hxx>

using namespace osg;
using namespace osgDB;

namespace simgear
{
void CloudShaderGeometry::drawImplementation(RenderInfo& renderInfo) const
{
    osg::State& state = *renderInfo.getState();
    const Extensions* extensions = getExtensions(state.getContextID(),true);

    for(CloudSpriteList::const_iterator t = _cloudsprites.begin(); t != _cloudsprites.end(); ++t)
    {
        extensions->glVertexAttrib1f(TEXTURE_INDEX_X, (GLfloat) t->texture_index_x/varieties_x);
        extensions->glVertexAttrib1f(TEXTURE_INDEX_Y, (GLfloat) t->texture_index_y/varieties_y);
        extensions->glVertexAttrib1f(WIDTH, (GLfloat) t->width);
        extensions->glVertexAttrib1f(HEIGHT, (GLfloat) t->height);
        extensions->glVertexAttrib1f(SHADE, (GLfloat) t->shade);
        glColor4f(t->position.x(), t->position.y(), t->position.z(), 1.0);
        _geometry->draw(renderInfo);
    }
}

BoundingBox CloudShaderGeometry::computeBound() const
{
    BoundingBox geom_box = _geometry->getBound();
    BoundingBox bb;
    for(CloudSpriteList::const_iterator itr = _cloudsprites.begin();
        itr != _cloudsprites.end();
        ++itr) {
         bb.expandBy(geom_box.corner(0)*itr->width +
                     osg::Vec3( itr->position.x(), itr->position.y(), itr->position.z() ));
         bb.expandBy(geom_box.corner(7)*itr->height +
                     osg::Vec3( itr->position.x(), itr->position.y(), itr->position.z() ));
    }
    return bb;
}

bool CloudShaderGeometry_readLocalData(Object& obj, Input& fr)
{
    bool iteratorAdvanced = false;

    CloudShaderGeometry& geom = static_cast<CloudShaderGeometry&>(obj);

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
        geom._cloudsprites.reserve(capacity);
        fr += 3;
        iteratorAdvanced = true;
        // skip {
        while (!fr.eof() && fr[0].getNoNestedBrackets() > entry) {
            SGVec3f v;
            int tx, ty;
            float w, h, s;
            if (fr[0].getFloat(v.x()) && fr[1].getFloat(v.y())
                && fr[2].getFloat(v.z()) && fr[3].getInt(tx) && fr[3].getInt(ty) &&  
                fr[4].getFloat(w) && fr[4].getFloat(h)&& fr[4].getFloat(s)) {
                    fr += 5;
                    //SGVec3f* v = new SGVec3f(v.x(), v.y(), v.z());
                    geom._cloudsprites.push_back(CloudShaderGeometry::CloudSprite(v, tx, ty, w, h,s));
            } else {
                ++fr;
            }
        }
    }
    return iteratorAdvanced;
}

bool CloudShaderGeometry_writeLocalData(const Object& obj, Output& fw)
{
    const CloudShaderGeometry& geom = static_cast<const CloudShaderGeometry&>(obj);

    fw.indent() << "geometry" << std::endl;
    fw.writeObject(*geom._geometry);
    fw.indent() << "instances " << geom._cloudsprites.size() << std::endl;
    fw.indent() << "{" << std::endl;
    fw.moveIn();
    for (CloudShaderGeometry::CloudSpriteList::const_iterator iter
             = geom._cloudsprites.begin();
         iter != geom._cloudsprites.end();
         ++iter) {
        fw.indent() << iter->position.x() << " " << iter->position.y() << " " 
                << iter->position.z() << " " << iter->texture_index_x << " "
                << iter->texture_index_y << " "  
                << iter->width << " " << iter->height << " " << iter->shade << std::endl;
    }
    fw.moveOut();
    fw.indent() << "}" << std::endl;
    return true;
}


osgDB::RegisterDotOsgWrapperProxy cloudShaderGeometryProxy
(
    new CloudShaderGeometry,
    "CloudShaderGeometry",
    "Object Drawable CloudShaderGeometry",
    &CloudShaderGeometry_readLocalData,
    &CloudShaderGeometry_writeLocalData
    );
}
