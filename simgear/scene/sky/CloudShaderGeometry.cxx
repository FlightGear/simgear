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
    if (!_cloudsprites.size()) return;
    
    osg::State& state = *renderInfo.getState();
    unsigned int contextID = state.getContextID();
    SortData& sortData = _sortData[contextID];
    int frameNumber = state.getFrameStamp()->getFrameNumber();

    if (!sortData.spriteIdx)
        sortData.spriteIdx = new SortData::SortItemList;
    if (sortData.spriteIdx->size() < _cloudsprites.size()) {
        for (unsigned i = sortData.spriteIdx->size(); i < (unsigned)_cloudsprites.size(); ++i)
            sortData.spriteIdx->push_back(SortData::SortItem(i, 0.0f));
        sortData.frameSorted = frameNumber - (sortData.skip_limit + 1);
    }
    // If the cloud is already sorted, then it is likely to still be sorted.
    // Therefore we can avoid re-sorting it for a period. If it is still
    // sorted after that period, then we can wait for a longer period before
    // checking again. In this way, only clouds that are changing regularly
    // are sorted.
    if (frameNumber - sortData.skip_limit >= sortData.frameSorted) {
        Matrix mvp = state.getModelViewMatrix() * state.getProjectionMatrix();
        for (SortData::SortItemList::iterator itr = sortData.spriteIdx->begin(),
                 end = sortData.spriteIdx->end();
             itr != end;
             ++itr) {
            Vec4f projPos
                = Vec4f(toOsg(_cloudsprites[itr->idx].position), 1.0f) * mvp;
            itr->depth = projPos.z() / projPos.w();
        }
        // Already sorted?
        if (std::adjacent_find(sortData.spriteIdx->rbegin(),
                               sortData.spriteIdx->rend(), SpriteComp())
            == sortData.spriteIdx->rend()) {
            // This cloud is sorted, so no need to re-sort.
            sortData.skip_limit = sortData.skip_limit * 2;
            if (sortData.skip_limit > 30) {
                // Jitter the skip frames to avoid synchronized sorts
                // which will cause periodic frame-rate drops
                sortData.skip_limit += sg_random() * 10;
            }
            if (sortData.skip_limit > 128) {
                // Maximum of every 128 frames (2 - 4 seconds)
                sortData.skip_limit = 128 + sg_random() * 10;
            }

        } else {
            std::sort(sortData.spriteIdx->begin(), sortData.spriteIdx->end(),
                      SpriteComp());
            sortData.skip_limit = 1;
        }
        sortData.frameSorted = frameNumber;
    }

    const Extensions* extensions = getExtensions(state.getContextID(),true);

    for(SortData::SortItemList::const_iterator itr = sortData.spriteIdx->begin(),
            end = sortData.spriteIdx->end();
        itr != end;
        ++itr) {
        const CloudSprite& t = _cloudsprites[itr->idx];
        GLfloat ua1[3] = { (GLfloat) t.texture_index_x/varieties_x,
                           (GLfloat) t.texture_index_y/varieties_y,
			   (GLfloat) t.width };
        GLfloat ua2[3] = { (GLfloat) t.height,
		           (GLfloat) t.shade,
                           (GLfloat) t.cloud_height };
        extensions->glVertexAttrib3fv(USR_ATTR_1, ua1 );
        extensions->glVertexAttrib3fv(USR_ATTR_2, ua2 );
        glColor4f(t.position.x(), t.position.y(), t.position.z(), zscale);
        _geometry->draw(renderInfo);
    }
}

void CloudShaderGeometry::addSprite(const SGVec3f& p, int tx, int ty,
                                    float w, float h,
                                    float s, float cull, float cloud_height)
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

    _cloudsprites.push_back(CloudSprite(p, tx, ty, w, h, s, cloud_height));
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
                    geom._cloudsprites.push_back(CloudShaderGeometry::CloudSprite(v, tx, ty, w, h,s,ch));
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
             fw.indent() << itr->position.x() << " " << itr->position.y() << " " 
                     << itr->position.z() << " " << itr->texture_index_x << " "
                     << itr->texture_index_y << " "  << itr->width << " " 
                     << itr->height << " " << itr->shade 
                     << itr->cloud_height << " "<< std::endl;
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
