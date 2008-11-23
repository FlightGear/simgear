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
    if (!_cloudsprites.size()) return;
    
    osg::State& state = *renderInfo.getState();
    osg::Matrix vm = state.getModelViewMatrix();
    
    //TODO: It isn't clear whether this is worth the perf hit ATM.
    
    // Transform the viewing direction, represented by the eye space vector (0,0,-1, 0), into model-space
    // (here we simply take the opposite direction and reverse the ordering when sorting)
    osg::Vec3f view_dir(vm(0, 2), vm(1, 2), vm(2, 2));      // Caveat: OpenSceneGraph matrices are transposed!

    float p = view_dir*_cloudsprites[0]->position.osg();
    // Do a single iteration of a bubble sort, sorting
    // back to front.
    for(int i = 0; i < _cloudsprites.size() - 1; i++)
    {
        float q = view_dir*_cloudsprites[i+1]->position.osg();
        if (p > q) {  
            CloudSprite c = *_cloudsprites[i];
            *_cloudsprites[i] = *_cloudsprites[i+1];
            *_cloudsprites[i+1] = c;
        }
        else
            p = q;
    }

    const Extensions* extensions = getExtensions(state.getContextID(),true);

    for(CloudSpriteList::const_iterator t = _cloudsprites.begin(); t != _cloudsprites.end(); ++t)
    {
        extensions->glVertexAttrib1f(TEXTURE_INDEX_X, (GLfloat) (*t)->texture_index_x/varieties_x);
        extensions->glVertexAttrib1f(TEXTURE_INDEX_Y, (GLfloat) (*t)->texture_index_y/varieties_y);
        extensions->glVertexAttrib1f(WIDTH, (GLfloat) (*t)->width);
        extensions->glVertexAttrib1f(HEIGHT, (GLfloat) (*t)->height);
        extensions->glVertexAttrib1f(SHADE, (GLfloat) (*t)->shade);
        extensions->glVertexAttrib1f(CLOUD_HEIGHT, (GLfloat) (*t)->cloud_height);
        glColor4f((*t)->position.x(), (*t)->position.y(), (*t)->position.z(), 1.0);
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
         bb.expandBy(geom_box.corner(0)*(*itr)->width +
                 osg::Vec3( (*itr)->position.x(), (*itr)->position.y(), (*itr)->position.z() ));
         bb.expandBy(geom_box.corner(7)*(*itr)->height +
                 osg::Vec3( (*itr)->position.x(), (*itr)->position.y(), (*itr)->position.z() ));
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
            float w, h, s, ch;
            if (fr[0].getFloat(v.x()) && fr[1].getFloat(v.y())
                && fr[2].getFloat(v.z()) && fr[3].getInt(tx) && fr[4].getInt(ty) &&  
                fr[5].getFloat(w) && fr[6].getFloat(h)&& fr[7].getFloat(s) && fr[8].getFloat(ch)) {
                    fr += 5;
                    //SGVec3f* v = new SGVec3f(v.x(), v.y(), v.z());
                    geom._cloudsprites.push_back(new CloudShaderGeometry::CloudSprite(v, tx, ty, w, h,s,ch));
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
    for (CloudShaderGeometry::CloudSpriteList::const_iterator itr
             = geom._cloudsprites.begin();
         itr != geom._cloudsprites.end();
         ++itr) {
             fw.indent() << (*itr)->position.x() << " " << (*itr)->position.y() << " " 
                     << (*itr)->position.z() << " " << (*itr)->texture_index_x << " "
                     << (*itr)->texture_index_y << " "  << (*itr)->width << " " 
                     << (*itr)->height << " " << (*itr)->shade 
                     << (*itr)->cloud_height << " "<< std::endl;
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
