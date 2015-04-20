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

#include <algorithm>

#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/ParameterOutput>

#include "CloudShaderGeometry.hxx"

#include <simgear/props/props.hxx>

using namespace osg;
using namespace osgDB;
using namespace simgear;

namespace
{
struct SpriteComp
{
    bool operator() (const CloudShaderGeometry::SortData::SortItem& lhs,
                     const CloudShaderGeometry::SortData::SortItem& rhs) const
    {
        return lhs.depth > rhs.depth;
    }
};
}
namespace simgear
{
void CloudShaderGeometry::drawImplementation(RenderInfo& renderInfo) const
{
    if (_cloudsprites.empty()) return;
    
    osg::State& state = *renderInfo.getState();
    
    int frameNumber = state.getFrameStamp()->getFrameNumber();
    unsigned int contextID = state.getContextID();    
    SortData& sortData = _sortData[contextID];
    Geometry* g = _geometry->asGeometry();
    
    // If the cloud is already sorted, then it is likely to still be sorted.
    // Therefore we can avoid re-sorting it for a period. If it is still
    // sorted after that period, then we can wait for a longer period before
    // checking again. In this way, only clouds that are changing regularly
    // are sorted.        
    osg::Vec3Array* v = dynamic_cast<osg::Vec3Array*>(g->getVertexArray());
    if ((v->size() > 4) && 
        (frameNumber - sortData.skip_limit >= sortData.frameSorted)) {
        Matrix mvp = state.getModelViewMatrix() * state.getProjectionMatrix();
        
        osg::Vec4Array* c = dynamic_cast<osg::Vec4Array*>(g->getColorArray());
        osg::Vec2Array* t = dynamic_cast<osg::Vec2Array*>(g->getTexCoordArray(0));
        Vec3f av[4];
        Vec4f ac[4];
        Vec2f at[4];        
        
        // Perform a single pass bubble sort of the array, 
        // keeping track of whether we've had to make any changes
        bool sorted = true;                      
        for (unsigned int i = 4; i < v->size(); i = i + 4) {
            // The position of the sprite is stored in the colour
            // array, with the exception of the w() coordinate
            // which is the z-scaling parameter.
            Vec4f a = (*c)[i-4];
            Vec4f aPos = Vec4f(a.x(), a.y(), a.z(), 1.0f) * mvp;
            Vec4f b = (*c)[i];
            Vec4f bPos = Vec4f(b.x(), b.y(), b.z(), 1.0f) * mvp;
            
            if ((aPos.z()/aPos.w()) < (bPos.z()/bPos.w() - 0.0001)) {
                // a is non-trivially closer than b, so should be rendered
                // later. Swap them around
                for (int j = 0; j < 4; j++) {
                    av[j] = (*v)[i+j-4];
                    ac[j] = (*c)[i+j-4];
                    at[j] = (*t)[i+j-4];
                    
                    (*v)[i+j -4] = (*v)[i+j];
                    (*c)[i+j -4] = (*c)[i+j];
                    (*t)[i+j -4] = (*t)[i+j];
                    
                    (*v)[i+j] = av[j];
                    (*c)[i+j] = ac[j];
                    (*t)[i+j] = at[j];
                }
                
                // Indicate that the arrays were not sorted
                // so we should check them next iteration
                sorted = false;
            }
        }
        
        if (sorted) {
            // This cloud is sorted, so no need to re-sort.
            
            sortData.skip_limit = sortData.skip_limit * 2;
            if (sortData.skip_limit > 30) {
                // Jitter the skip frames to avoid synchronized sorts
                // which will cause periodic frame-rate drops
                sortData.skip_limit += sg_random() * 10;
            }
            if (sortData.skip_limit > 500) {
                // Maximum of every 500 frames (10-20 seconds)
                sortData.skip_limit = 500 + sg_random() * 10;
            }
        } else {
            sortData.skip_limit = 1;
        }
        
        sortData.frameSorted = frameNumber;
    }

#if OSG_VERSION_LESS_THAN(3,3,4)
    const Extensions* extensions = getExtensions(state.getContextID(),true);
#else
    const GLExtensions* extensions = GLExtensions::Get(state.getContextID(), true);
#endif
    GLfloat ua1[3] = { (GLfloat) alpha_factor,
                       (GLfloat) shade_factor,
                       (GLfloat) cloud_height };
    GLfloat ua2[3] = { (GLfloat) bottom_factor,
                       (GLfloat) middle_factor,
                       (GLfloat) top_factor };
                       
    extensions->glVertexAttrib3fv(USR_ATTR_1, ua1 );
    extensions->glVertexAttrib3fv(USR_ATTR_2, ua2 );
    _geometry->draw(renderInfo);    
}

void CloudShaderGeometry::addSprite(const SGVec3f& p, int tx, int ty,
                                    float w, float h, float cull)
{
    // Only add the sprite if it is further than the cull distance to all other sprites
    // except for the center sprite.	
    for (CloudShaderGeometry::CloudSpriteList::iterator iter = _cloudsprites.begin();
         iter != _cloudsprites.end();
         ++iter) 
    {
        if ((iter != _cloudsprites.begin()) &&
	    (distSqr(iter->position, p) < cull)) {
            // Too close - cull it
            return;
        }
    }
    
    _cloudsprites.push_back(CloudSprite(p, tx, ty, w, h));
}

void CloudShaderGeometry::generateGeometry()
{
    // Generate a set of geometries as a QuadStrip based on the list of sprites
    int numsprites = _cloudsprites.size();
    
    // Create front and back polygons so we don't need to screw around
    // with two-sided lighting in the shader.
    osg::ref_ptr<osg::Vec3Array> v = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> n = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> c = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec2Array> t = new osg::Vec2Array;
    
    int idx = 0;

    for (CloudShaderGeometry::CloudSpriteList::iterator iter = _cloudsprites.begin();
         iter != _cloudsprites.end();
         ++iter) 
    {
    
        float cw = 0.5f * iter->width;
        float ch = 0.5f * iter->height;        
        
        // Create the vertices
        v->push_back(osg::Vec3(0.0f, -cw, -ch));
        v->push_back(osg::Vec3(0.0f,  cw, -ch));
        v->push_back(osg::Vec3(0.0f,  cw, ch));
        v->push_back(osg::Vec3(0.0f, -cw, ch));
        
        // The normals aren't actually used in lighting,
        // but we set them per vertex as this is more
        // efficient than an overall binding on some
        // graphics cards.
        n->push_back(osg::Vec3(1.0f, -1.0f, -1.0f));
        n->push_back(osg::Vec3(1.0f,  1.0f, -1.0f));
        n->push_back(osg::Vec3(1.0f,  1.0f,  1.0f));
        n->push_back(osg::Vec3(1.0f, -1.0f,  1.0f));

        // Set the texture coords for each vertex
        // from the texture index, and the number
        // of textures in the image    
        int x = iter->texture_index_x;
        int y = iter->texture_index_y;
        
        t->push_back(osg::Vec2( (float) x       / varieties_x, (float) y / varieties_y));
        t->push_back(osg::Vec2( (float) (x + 1) / varieties_x, (float) y / varieties_y));
        t->push_back(osg::Vec2( (float) (x + 1) / varieties_x, (float) (y + 1) / varieties_y));
        t->push_back(osg::Vec2( (float) x       / varieties_x, (float) (y + 1) / varieties_y));
        
        // The color isn't actually use in lighting, but instead to indicate the center of rotation
        c->push_back(osg::Vec4(iter->position.x(), iter->position.y(), iter->position.z(), zscale));
        c->push_back(osg::Vec4(iter->position.x(), iter->position.y(), iter->position.z(), zscale));
        c->push_back(osg::Vec4(iter->position.x(), iter->position.y(), iter->position.z(), zscale));
        c->push_back(osg::Vec4(iter->position.x(), iter->position.y(), iter->position.z(), zscale));
        
        idx++;      
    }
    
    //Quads now created, add it to the geometry.
    osg::Geometry* geom = new osg::Geometry;
    geom->setVertexArray(v);
    geom->setTexCoordArray(0, t);
    geom->setNormalArray(n);
    geom->setNormalBinding(Geometry::BIND_PER_VERTEX);
    geom->setColorArray(c);
    geom->setColorBinding(Geometry::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,numsprites*4));
    _geometry = geom;
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
            float w, h;
            if (fr[0].getFloat(v.x()) && fr[1].getFloat(v.y())
                && fr[2].getFloat(v.z()) && fr[3].getInt(tx) && fr[4].getInt(ty) &&  
                fr[5].getFloat(w) && fr[6].getFloat(h)) {
                    fr += 5;
                    //SGVec3f* v = new SGVec3f(v.x(), v.y(), v.z());
                    geom._cloudsprites.push_back(CloudShaderGeometry::CloudSprite(v, tx, ty, w, h));
            } else {
                ++fr;
            }
        }
        geom.generateGeometry();
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
             fw.indent() << itr->position.x() << " " << itr->position.y() << " " 
                     << itr->position.z() << " " << itr->texture_index_x << " "
                     << itr->texture_index_y << " "  << itr->width << " " 
                     << itr->height << " " << std::endl;
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
