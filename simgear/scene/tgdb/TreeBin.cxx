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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <thread>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Math>
#include <osg/MatrixTransform>
#include <osg/Matrix>
#include <osg/NodeVisitor>

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/math/sg_random.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/OsgUtils.hxx>

#include "TreeBin.hxx"

const int TREE_INSTANCE_POSITIONS = 6;  // (x,y,z) See also tree.eff

using namespace osg;

namespace simgear
{

// Tree instance scheme:
// vertex - local position of quad vertex.
// normal - x y scaling, z number of varieties
// fog coord - rotation
// color - xyz of tree quad origin, replicated 4 times.
//
// The tree quad is rendered twice, with different rotations, to
// create the crossed tree geometry.


struct TreeInstanceBoundingBoxCallback : public Drawable::ComputeBoundingBoxCallback {
    TreeInstanceBoundingBoxCallback() {}
    TreeInstanceBoundingBoxCallback(const TreeInstanceBoundingBoxCallback&, const CopyOp&) {}
    META_Object(simgear, TreeInstanceBoundingBoxCallback);
    virtual BoundingBox computeBound(const Drawable& drawable) const
    {
        BoundingBox bb;
        const Geometry* geometry = static_cast<const Geometry*>(&drawable);
        const Vec3Array* instancePositions = static_cast<const Vec3Array*>(geometry->getVertexAttribArray(TREE_INSTANCE_POSITIONS));

        const Vec3Array* normals = static_cast<const Vec3Array*>(geometry->getNormalArray());
        const osg::Vec3f normal = static_cast<const Vec3f>((*normals)[0]);

        float maxScaleX = (float) normal[0];
        float maxScaleY = (float) normal[1];

        for (unsigned int v = 0; v < instancePositions->size(); ++v) {
            Vec3 pt = (*instancePositions)[v];
            bb.expandBy(pt);
        }

        bb = BoundingBox(bb._min - osg::Vec3(maxScaleX, maxScaleX, maxScaleY),
                         bb._max + osg::Vec3(maxScaleX, maxScaleX, maxScaleY));

        return bb;
    }
};


EffectGeode* createTreeGeode(TreeBin* forest)
{

    Vec3Array* vertexArray = new Vec3Array;
    Vec2Array* texCoords   = new Vec2Array;

    // Create a number of quads rotated evenly in the z-axis around the origin.
    const int NUM_QUADS = 3;

    vertexArray->reserve(NUM_QUADS * 6);
    texCoords->reserve(NUM_QUADS * 6);

    for (int i = 0; i < NUM_QUADS; ++i) {
        const double x1 = sin(((double) i) * PI / (double) NUM_QUADS) * 0.5f;
        const double y1 = cos(((double) i) * PI / (double) NUM_QUADS) * 0.5f;
        const double x2 = -x1;
        const double y2 = -y1;

        const osg::Vec3 v0(x1, y1, 0.0f);
        const osg::Vec3 v1(x2, y2, 0.0f);
        const osg::Vec3 v2(x2, y2, 1.0f);
        const osg::Vec3 v3(x1, y1, 1.0f);
        vertexArray->push_back(v0); vertexArray->push_back(v1); vertexArray->push_back(v2); // 1st triangle
        vertexArray->push_back(v0); vertexArray->push_back(v2); vertexArray->push_back(v3); // 2nd triangle

        // The texture coordinate range is not the entire coordinate
        // space, as the texture has a number of different trees on
        // it. We let the shader choose the variety.
        // Y-value chosen so that we definitely won't get artifacts from the tree trunk on the 
        // subtexture above in the tree atlas
        const osg::Vec2 t0(0.0f, 0.0f);
        const osg::Vec2 t1(1.0f, 0.0f);
        const osg::Vec2 t2(1.0f, 0.234f);
        const osg::Vec2 t3(0.0f, 0.234f);
        texCoords->push_back(t0); texCoords->push_back(t1); texCoords->push_back(t2); // 1st triangle
        texCoords->push_back(t0); texCoords->push_back(t2); texCoords->push_back(t3); // 2nd triangle
    }

    Geometry* geometry = new Geometry;
    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);
    geometry->setComputeBoundingBoxCallback(new TreeInstanceBoundingBoxCallback);

    geometry->setVertexArray(vertexArray);
    geometry->setTexCoordArray(0, texCoords, Array::BIND_PER_VERTEX);

    Vec3Array* params = new Vec3Array;
    params->push_back(Vec3(forest->width, forest->height, (float)forest->texture_varieties));
    geometry->setNormalArray(params, Array::BIND_OVERALL);

    osg::Vec3Array* positions = new osg::Vec3Array;
    positions->reserve(forest->getNumTrees());

    for (auto i = forest->_trees.begin(); i != forest->_trees.end(); ++i) {
        positions->push_back(*i);
    }

    geometry->setVertexAttribArray(TREE_INSTANCE_POSITIONS, positions, Array::BIND_PER_VERTEX, 1);
    //if (_customAttribs != NULL) {
    //    geometry->setVertexAttribArray(INSTANCE_CUSTOM_ATTRIBS, _customAttribs, Array::BIND_PER_VERTEX, 1);
    //}

    DrawArrays* primset = new DrawArrays(GL_TRIANGLES, 0, vertexArray->size(), positions->size());
    geometry->addPrimitiveSet(primset);

    // Force generation of the bounding box in this pager thread so that we don't need to do it
    // in the main update thread later.
    geometry->getBound();

    EffectGeode* result = new EffectGeode;
    result->addDrawable(geometry);
    return result;
}

typedef std::map<std::string, osg::observer_ptr<Effect> > EffectMap;

static EffectMap treeEffectMap;
inline static std::mutex treeEffectMapMutex; // Protects the treeEffectMap for multi-threaded access

// This actually returns a MatrixTransform node. If we rotate the whole
// forest into the local Z-up coordinate system we can reuse the
// primitive tree geometry for all the forests of the same type.

Group* createForest(SGTreeBinList& forestList, osg::ref_ptr<simgear::SGReaderWriterOptions> options)
{
    // Set up some shared structures.
    Group* group = new Group;

    SGTreeBinList::iterator i;

    for (i = forestList.begin(); i != forestList.end(); ++i) {
        TreeBin* forest = *i;

        // No point generating anything if there aren't any trees.
        if (forest->getNumTrees() == 0) continue;

        osg::ref_ptr<Effect> effect;

        {
            const std::lock_guard<std::mutex> lock(treeEffectMapMutex); // Lock the treeEffectMap for this scope
            EffectMap::iterator iter = treeEffectMap.find(forest->texture);
            if (iter == treeEffectMap.end() || (!iter->second.lock(effect)))
            {
                SGPropertyNode_ptr effectProp = new SGPropertyNode;
                makeChild(effectProp, "inherits-from")->setStringValue(forest->teffect);
                SGPropertyNode* params = makeChild(effectProp, "parameters");
                // emphasize n = 0
                params->getChild("texture", 0, true)->getChild("image", 0, true)
                    ->setStringValue(forest->texture);
                params->getChild("texture", 1, true)->getChild("image", 0, true)
                    ->setStringValue(forest->normal_map);
                effect = makeEffect(effectProp, true, options);

                if (iter == treeEffectMap.end()) {
                    treeEffectMap.insert(EffectMap::value_type(forest->texture, effect));
                    SG_LOG(SG_TERRAIN, SG_DEBUG, "Created new tree effectMap for " << forest->texture);
                } else {
                    iter->second = effect; // update existing, but empty observer
                }
            }

            if (effect == 0) {
                SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to find effect for " << forest->texture);
            }
        }

        EffectGeode* geode = createTreeGeode(forest);
        geode->setEffect(effect.get());
        group->addChild(geode);
    }

    return group;
}

TreeBin::TreeBin(const SGMaterial *mat)
{
    texture_varieties = mat->get_tree_varieties();
    range = mat->get_tree_range();
    height = mat->get_tree_height();
    width = mat->get_tree_width();
    texture = mat->get_tree_texture();
    normal_map = mat->get_tree_normal_map();
    teffect = mat->get_tree_effect();
};


TreeBin::TreeBin(const SGPath& absoluteFileName, const SGMaterial *mat) : 
TreeBin(mat)
{
    sg_gzifstream stream(absoluteFileName);
    if (!stream.is_open()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to open " << absoluteFileName);
        return;
    }

    while (!stream.eof()) {
        // read a line.  Each line defines a single tree position, and may have
        // a comment, starting with #
        std::string line;
        std::getline(stream, line);

        // strip comments
        std::string::size_type hash_pos = line.find('#');
        if (hash_pos != std::string::npos)
            line.resize(hash_pos);

        // and process further
        std::stringstream in(line);

        // Line format is X Y Z A B C
        // where:
        // X,Y,Z are the cartesian coordinates of the center tree
        // A,B,C is the normal of the underlying terrain, defaulting to 0,0,1
        float x = 0.0f, y = 0.0f, z = 0.0f, a = 0.0f, b = 0.0f, c = 1.0f;
        in >> x >> y >> z;

        if (in.bad() || in.fail()) {
            SG_LOG(SG_TERRAIN, SG_WARN, "Error parsing tree entry in: " << absoluteFileName << " line: \"" << line << "\"");
            continue;
        }

        // these might fail, so check them after we look at failbit
        in >> a >> b >> c;

        osg::Vec3d loc = osg::Vec3d(x,y,z);
        insert(loc);
    }

    stream.close();
};


}