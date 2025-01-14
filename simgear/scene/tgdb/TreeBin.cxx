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
#include <osg/VertexAttribDivisor>

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/math/sg_random.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/util/QuadTreeBuilder.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/OsgUtils.hxx>

#include "ShaderGeometry.hxx"
#include "TreeBin.hxx"

#define SG_TREE_QUAD_TREE_DEPTH 3
#define SG_TREE_FADE_OUT_LEVELS 10

const int TREE_INSTANCE_POSITIONS = 6;  // (x,y,z) See also tree.eff
const int TREE_INSTANCE_TERRAIN_NORMALS = 7; // (x,y,z) See also tree.eff

using namespace osg;

namespace simgear
{

bool use_tree_shadows;
bool use_tree_normals;

// Tree instance scheme:
// vertex - local position of quad vertex.
// normal - x y scaling, z number of varieties
// fog coord - rotation

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

    Geometry* geometry = new Geometry;
    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);
    geometry->setComputeBoundingBoxCallback(new TreeInstanceBoundingBoxCallback);

    Vec3Array* vertexArray = new Vec3Array;
    Vec2Array* texCoords   = new Vec2Array;

    vertexArray->reserve(12);
    texCoords->reserve(12);

    // Create the vertices
    osg::Vec3 v0(0.0f, -0.5f, 0.0f);
    osg::Vec3 v1(0.0f,  0.5f, 0.0f);
    osg::Vec3 v2(0.0f,  0.5f, 1.0f);
    osg::Vec3 v3(0.0f, -0.5f, 1.0f);
    vertexArray->push_back(v0); vertexArray->push_back(v1); vertexArray->push_back(v2); // 1st triangle
    vertexArray->push_back(v0); vertexArray->push_back(v2); vertexArray->push_back(v3); // 2nd triangle

    osg::Vec3 v4(-0.5f, 0.0f, 0.0f);
    osg::Vec3 v5( 0.5f, 0.0f, 0.0f);
    osg::Vec3 v6( 0.5f, 0.0f, 1.0f);
    osg::Vec3 v7(-0.5f, 0.0f, 1.0f);
    vertexArray->push_back(v4); vertexArray->push_back(v5); vertexArray->push_back(v6); // 3rd triangle
    vertexArray->push_back(v4); vertexArray->push_back(v6); vertexArray->push_back(v7); // 4th triangle

    // The texture coordinate range is not the entire coordinate
    // space, as the texture has a number of different trees on
    // it. We let the shader choose the variety.
    osg::Vec2 t0(0.0f, 0.0f);
    osg::Vec2 t1(1.0f, 0.0f);
    osg::Vec2 t2(1.0f, 0.234f);
    osg::Vec2 t3(0.0f, 0.234f);
    texCoords->push_back(t0); texCoords->push_back(t1); texCoords->push_back(t2); // 1st triangle
    texCoords->push_back(t0); texCoords->push_back(t2); texCoords->push_back(t3); // 2nd triangle
    texCoords->push_back(t0); texCoords->push_back(t1); texCoords->push_back(t2); // 3rd triangle
    texCoords->push_back(t0); texCoords->push_back(t2); texCoords->push_back(t3); // 4th triangle

    if (use_tree_shadows || use_tree_normals) {
        // Tree shadows are simply another set of triangles that will be rotated into position
        // by the vertex shader based on the terrain normal.
        vertexArray->push_back(v0); vertexArray->push_back(v1); vertexArray->push_back(v2); // 5th triangle
        vertexArray->push_back(v0); vertexArray->push_back(v2); vertexArray->push_back(v3); // 6th triangle

        // Generate texture coordinates for the additional pair of triangles
        texCoords->push_back(t0); texCoords->push_back(t1); texCoords->push_back(t2); // 5th triangle
        texCoords->push_back(t0); texCoords->push_back(t2); texCoords->push_back(t3); // 6th triangle

        // If we have enabled tree shadows then we also need to add some color information
        // to identify the shadow triangles
        Vec4Array* colors = new Vec4Array;
        for (unsigned int c = 0; c < 12; ++c) colors->push_back(Vec4(0.0,0.0,0.0,0.0));  // Triangles 1-4
        for (unsigned int c = 0; c <  6; ++c) colors->push_back(Vec4(1.0,0.0,0.0,0.0));  // Triangles 5-6
        geometry->setColorArray(colors, Array::BIND_PER_VERTEX);

        osg::Vec3Array* tnormals = new osg::Vec3Array;
        geometry->setVertexAttribArray(TREE_INSTANCE_TERRAIN_NORMALS, tnormals, Array::BIND_PER_VERTEX);
    }

    geometry->setVertexArray(vertexArray);
    geometry->setTexCoordArray(0, texCoords, Array::BIND_PER_VERTEX);

    Vec3Array* params = new Vec3Array;
    params->push_back(Vec3(forest->width, forest->height, (float)forest->texture_varieties));
    geometry->setNormalArray(params, Array::BIND_OVERALL);

    osg::Vec3Array* positions = new osg::Vec3Array;
    geometry->setVertexAttribArray(TREE_INSTANCE_POSITIONS, positions, Array::BIND_PER_VERTEX);


    DrawArrays* primset = new DrawArrays(GL_TRIANGLES, 0, vertexArray->size());

    geometry->addPrimitiveSet(primset);

    EffectGeode* result = new EffectGeode;
    result->addDrawable(geometry);
    StateSet* ss = result->getOrCreateStateSet();
    ss->setAttributeAndModes(new osg::VertexAttribDivisor(TREE_INSTANCE_POSITIONS, 1));
    if (use_tree_shadows || use_tree_normals) {
        ss->setAttributeAndModes(new osg::VertexAttribDivisor(TREE_INSTANCE_TERRAIN_NORMALS, 1));
    }
    
    return result;
}

typedef std::map<std::string, osg::observer_ptr<Effect> > EffectMap;

static EffectMap treeEffectMap;
inline static std::mutex treeEffectMapMutex; // Protects the treeEffectMap for multi-threaded access

// Helper classes for creating the quad tree
namespace
{
struct MakeTreesLeaf
{
    MakeTreesLeaf(simgear::TreeBin* forest, ref_ptr<Effect> effect):
        _forest(forest), _effect(effect) {}

    MakeTreesLeaf(const MakeTreesLeaf& rhs) :
        _forest(rhs._forest), _effect(rhs._effect) {}

    LOD* operator() () const
    {
        if (LOD* result = new LOD; result) {
            // Create a series of LOD nodes so trees cover decreases slightly
            // gradually with distance from _range to 2*_range
            for (float i = 0.0f; i < SG_TREE_FADE_OUT_LEVELS; ++i)
            {
                if (EffectGeode* geode = createTreeGeode(_forest); geode) {
                    geode->setEffect(_effect.get());
                    result->addChild(geode, 0, _forest->range * (1.0f + i / (SG_TREE_FADE_OUT_LEVELS - 1.0f)));
                }
            }

            return result;
        }

        return nullptr;
    }

    simgear::TreeBin* _forest;
    ref_ptr<Effect> _effect;
};

struct AddTreesLeafObject
{
    void operator() (LOD* lod, const TreeBin::Tree& tree) const
    {
        Geode* geode = static_cast<Geode*>(lod->getChild(int(tree.position.x() * 10.0f) % lod->getNumChildren()));
        Vec3d pos = toOsg(tree.position);
        Vec3d ter = toOsg(tree.tnormal);
        Geometry* geom = static_cast<Geometry*>(geode->getDrawable(0));
        Vec3Array* posArray = static_cast<Vec3Array*>(geom->getVertexAttribArray(TREE_INSTANCE_POSITIONS));
        Vec3Array* tnormalArray = NULL;

        if (use_tree_shadows || use_tree_normals) {
            tnormalArray = static_cast<Vec3Array*>(geom->getVertexAttribArray(TREE_INSTANCE_TERRAIN_NORMALS));
        }

        posArray->push_back(pos);

        if (tnormalArray && (use_tree_shadows || use_tree_normals))
            tnormalArray->push_back(ter);

        DrawArrays* primSet = static_cast<DrawArrays*>(geom->getPrimitiveSet(0));
        primSet->setNumInstances(posArray->size());
    }
};

struct GetTreeCoord
{
    Vec3 operator() (const TreeBin::Tree& tree) const
    {
        return toOsg(tree.position);
    }
};

typedef QuadTreeBuilder<LOD*, TreeBin::Tree, MakeTreesLeaf, AddTreesLeafObject,
                        GetTreeCoord> ShaderGeometryQuadtree;
}

struct TreeTransformer
{
    TreeTransformer(Matrix& mat_) : mat(mat_) {}
    TreeBin::Tree operator()(const TreeBin::Tree& tree) const
    {
        Vec3 pos  = toOsg(tree.position);
	    Vec3 norm = toOsg(tree.tnormal);
        return TreeBin::Tree(toSG(pos * mat),toSG(norm * mat));
    }
    Matrix mat;
};

// We may end up with a quadtree with many empty leaves. One might say
// that we should avoid constructing the leaves in the first place,
// but this node visitor tries to clean up after the fact.

struct QuadTreeCleaner : public osg::NodeVisitor
{
    QuadTreeCleaner() : NodeVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {
    }
    void apply(LOD& lod)
    {
        for (int i  = lod.getNumChildren() - 1; i >= 0; --i) {
            EffectGeode* geode = dynamic_cast<EffectGeode*>(lod.getChild(i));
            // EffectGeode* geode = nullptr;
            // try {
            //     geode = dynamic_cast<EffectGeode*>(lod.getChild(i));
            // }
            // catch (std::bad_cast) {
            //     SG_LOG(SG_TERRAIN, SG_ALERT, "caught bad_cast exception " << typeof(lod.getChild(i)));
            // }
            if (!geode)
                continue;
            bool geodeEmpty = true;
            for (unsigned j = 0; j < geode->getNumDrawables(); ++j) {
                const Geometry* geom = dynamic_cast<Geometry*>(geode->getDrawable(j));
                if (!geom) {
                    geodeEmpty = false;
                    break;
                }
                for (unsigned k = 0; k < geom->getNumPrimitiveSets(); k++) {
                    const PrimitiveSet* ps = geom->getPrimitiveSet(k);
                    if (ps->getNumIndices() > 0) {
                        geodeEmpty = false;
                        break;
                    }
                }
            }
            if (geodeEmpty)
                lod.removeChildren(i, 1);
        }
    }
};

// This actually returns a MatrixTransform node. If we rotate the whole
// forest into the local Z-up coordinate system we can reuse the
// primitive tree geometry for all the forests of the same type.

osg::Group* createForest(SGTreeBinList& forestList, const osg::Matrix& transform,
                         const SGReaderWriterOptions* options, int depth)
{
    Matrix transInv = Matrix::inverse(transform);
    // Set up some shared structures.
    ref_ptr<Group> group;
    MatrixTransform* mt = new MatrixTransform(transform);

    SGTreeBinList::iterator i;

    use_tree_shadows = false;
    use_tree_normals = false;
    if (options) {
        SGPropertyNode* propertyNode = options->getPropertyNode().get();
        if (propertyNode) {
            use_tree_shadows
                = propertyNode->getBoolValue("/sim/rendering/random-vegetation-shadows",
                                             use_tree_shadows);
           use_tree_normals
                = propertyNode->getBoolValue("/sim/rendering/random-vegetation-normals",
                                             use_tree_normals);
		}
	}

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

        // Now, create a quadtree for the forest.
        ShaderGeometryQuadtree
            quadtree(GetTreeCoord(), AddTreesLeafObject(),
                     depth,
                     MakeTreesLeaf(forest, effect));
        // Transform tree positions from the "geocentric" positions we
        // get from the scenery polys into the local Z-up coordinate
        // system.
        std::vector<TreeBin::Tree> rotatedTrees;
        rotatedTrees.reserve(forest->_trees.size());
        std::transform(forest->_trees.begin(), forest->_trees.end(),
                       std::back_inserter(rotatedTrees),
                       TreeTransformer(transInv));
        quadtree.buildQuadTree(rotatedTrees.begin(), rotatedTrees.end());
        group = quadtree.getRoot();

        for (size_t i = 0; i < group->getNumChildren(); ++i)
            mt->addChild(group->getChild(i));

        delete forest;
    }

    forestList.clear();
    QuadTreeCleaner cleaner;
    mt->accept(cleaner);
    return mt;
}

TreeBin::TreeBin(const SGMaterial *mat)
{
    texture_varieties = mat->get_tree_varieties();
    range = mat->get_tree_range();
    height = mat->get_tree_height();
    width = mat->get_tree_width();
    texture = mat->get_tree_texture();
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

        SGVec3f loc = SGVec3f(x,y,z);
        SGVec3f norm = SGVec3f(a,b,c);

        insert(loc, norm);
    }

    stream.close();
};


}