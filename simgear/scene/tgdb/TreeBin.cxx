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
#include <simgear/scene/util/QuadTreeBuilder.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/OsgUtils.hxx>

#include "TreeBin.hxx"

#define SG_TREE_QUAD_TREE_DEPTH 3
#define SG_TREE_FADE_OUT_LEVELS 10

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

struct TreesBoundingBoxCallback : public Drawable::ComputeBoundingBoxCallback
{
    TreesBoundingBoxCallback() {}
    TreesBoundingBoxCallback(const TreesBoundingBoxCallback&, const CopyOp&) {}
    META_Object(simgear, TreesBoundingBoxCallback);
    virtual BoundingBox computeBound(const Drawable&) const;
};

BoundingBox
TreesBoundingBoxCallback::computeBound(const Drawable& drawable) const
{
    BoundingBox bb;
    const Geometry* geom = static_cast<const Geometry*>(&drawable);
    const Vec3Array* v = static_cast<const Vec3Array*>(geom->getVertexArray());
    const Vec3Array* pos = static_cast<const Vec3Array*>(geom->getColorArray());
    const Vec3Array* params
        = static_cast<const Vec3Array*>(geom->getNormalArray());
    const FloatArray* rot
        = static_cast<const FloatArray*>(geom->getFogCoordArray());
    float w = (*params)[0].x();
    float h = (*params)[0].y();
    Geometry::PrimitiveSetList primSets = geom->getPrimitiveSetList();
    FloatArray::const_iterator rotitr = rot->begin();
    for (Geometry::PrimitiveSetList::const_iterator psitr = primSets.begin(),
             psend = primSets.end();
         psitr != psend;
         ++psitr, ++rotitr) {
        Matrixd trnsfrm = (Matrixd::scale(w, w, h)
                           * Matrixd::rotate(*rotitr, Vec3(0.0f, 0.0f, 1.0f)));
        DrawArrays* da = static_cast<DrawArrays*>(psitr->get());
        GLint psFirst = da->getFirst();
        GLint psEndVert = psFirst + da->getCount();
        for (GLint i = psFirst;i < psEndVert; ++i) {
            Vec3 pt = (*v)[i];
            pt = pt * trnsfrm;
            pt += (*pos)[i];
            bb.expandBy(pt);
        }
    }
    return bb;
}

static std::mutex static_sharedGeometryMutex;
static std::map<std::thread::id, ref_ptr<Vec3Array>  > sharedVertexMap;
static std::map<std::thread::id, ref_ptr<Vec2Array>  > sharedTextureCoordMap;

void clearSharedTreeGeometry()
{
    std::lock_guard<std::mutex> g(static_sharedGeometryMutex);
    sharedVertexMap.clear();
    sharedTextureCoordMap.clear();
}

osg::ref_ptr<Geometry> createTreeGeometry(float width, float height, int varieties)
{
    // Get the shared Geometry.  This is only shared within the thread as
    // the cloned Geometries all end up with the same VertexBufferObject.
    // This is OK (just about) within a thread, as the VBO will simply be
    // updated with data for each Geometry sequentially and passed to
    // the underlying graphics driver.  However in a multithreaded case,
    // this can result in multiple threads updating the VBO in parallel
    // and segmentation faults.
    std::lock_guard<std::mutex> g(static_sharedGeometryMutex);
    std::thread::id this_id = std::this_thread::get_id();
    if (!sharedVertexMap[this_id]) {
        SG_LOG(SG_TERRAIN, SG_DEBUG, "Creating new shared geometry for thread " << this_id);
        // generate a repeatable random seed
        pc_init(123);
        int numQuads = 1600;

        // set up the coords
        sharedVertexMap[this_id] = new osg::Vec3Array;
        sharedVertexMap[this_id]->reserve(numQuads * 6);

        sharedTextureCoordMap[this_id] = new osg::Vec2Array;
        sharedTextureCoordMap[this_id]->reserve(numQuads * 6);

        for (int i = 0; i < numQuads; ++i) {
            // Apply a random scaling factor and texture index.
            float h = (pc_rand() + pc_rand()) / 2.0f + 0.5f;
            float cw = h * .5;
            // Create the vertices
            osg::Vec3 v0(0.0f, -cw, 0.0f);
            osg::Vec3 v1(0.0f,  cw, 0.0f);
            osg::Vec3 v2(0.0f,  cw,    h);
            osg::Vec3 v3(0.0f, -cw,    h);
            sharedVertexMap[this_id]->push_back(v0); sharedVertexMap[this_id]->push_back(v1); sharedVertexMap[this_id]->push_back(v2); // 1st triangle
            sharedVertexMap[this_id]->push_back(v0); sharedVertexMap[this_id]->push_back(v2); sharedVertexMap[this_id]->push_back(v3); // 2nd triangle
            // The texture coordinate range is not the entire coordinate
            // space, as the texture has a number of different trees on
            // it. Here we assign random coordinates and let the shader
            // choose the variety.
            float variety = pc_rand();
            osg::Vec2 t0(variety, 0.0f);
            osg::Vec2 t1(variety + 1.0f, 0.0f);
            osg::Vec2 t2(variety + 1.0f, 0.234f);
            osg::Vec2 t3(variety, 0.234f);
            sharedTextureCoordMap[this_id]->push_back(t0); sharedTextureCoordMap[this_id]->push_back(t1); sharedTextureCoordMap[this_id]->push_back(t2); // 1st triangle
            sharedTextureCoordMap[this_id]->push_back(t0); sharedTextureCoordMap[this_id]->push_back(t2); sharedTextureCoordMap[this_id]->push_back(t3); // 2nd triangle
        }
    }

    osg::ref_ptr<Geometry> quadGeom = new Geometry;
    quadGeom->setUseVertexBufferObjects(true);
    quadGeom->setVertexArray(sharedVertexMap[this_id]);
    quadGeom->setTexCoordArray(0, sharedTextureCoordMap[this_id], Array::BIND_PER_VERTEX);
    quadGeom->setComputeBoundingBoxCallback(new TreesBoundingBoxCallback);

    osg::ref_ptr<Vec3Array> params = new Vec3Array;
    params->push_back(Vec3(width, height, (float)varieties));
    quadGeom->setNormalArray(params, Array::BIND_OVERALL);
    // Positions
    quadGeom->setColorArray(new Vec3Array, Array::BIND_PER_VERTEX);
    FloatArray* rotation = new FloatArray(3);
    (*rotation)[0] = 0.0;
    (*rotation)[1] = PI_2;
    quadGeom->setFogCoordArray(rotation, Array::BIND_PER_PRIMITIVE_SET);
    // The primitive sets render the same geometry, but the second
    // will rotated 90 degrees by the vertex shader, which uses the
    // fog coordinate as a rotation.
    for (int i = 0; i < 2; ++i)
        quadGeom->addPrimitiveSet(new DrawArrays(PrimitiveSet::TRIANGLES));
    return quadGeom;
}

osg::ref_ptr<EffectGeode> createTreeGeode(float width, float height, int varieties)
{
    osg::ref_ptr<EffectGeode> result = new EffectGeode;
    result->addDrawable(createTreeGeometry(width, height, varieties));
    return result;
}

void addTreeToLeafGeode(Geode* geode, const SGVec3f& p)
{
    Vec3 pos = toOsg(p);
    unsigned int numDrawables = geode->getNumDrawables();
    Geometry* geom = static_cast<Geometry*>(geode->getDrawable(numDrawables - 1));
    Vec3Array* posArray = static_cast<Vec3Array*>(geom->getColorArray());

    if (posArray->size() >= static_cast<Vec3Array*>(geom->getVertexArray())->size()) {
        Vec3Array* paramsArray = static_cast<Vec3Array*>(geom->getNormalArray());
        Vec3 params = (*paramsArray)[0];
        geom = createTreeGeometry(params.x(), params.y(), params.z());
        posArray = static_cast<Vec3Array*>(geom->getColorArray());
        geode->addDrawable(geom);
    }

    if (posArray)
    {
        posArray->insert(posArray->end(), 6, pos);

        size_t numVerts = posArray->size();
        for (unsigned int i = 0; i < 2; ++i) {
            if (i < geom->getNumPrimitiveSets()) {
                DrawArrays* primSet = static_cast<DrawArrays*>(geom->getPrimitiveSet(i));
                if (primSet != nullptr)
                    primSet->setCount(numVerts);
            }
        }
    }
}

typedef std::map<std::string, osg::observer_ptr<Effect> > EffectMap;

static EffectMap treeEffectMap;
inline static std::mutex treeEffectMapMutex; // Protects the treeEffectMap for multi-threaded access

// Helper classes for creating the quad tree
namespace
{
struct MakeTreesLeaf
{
    MakeTreesLeaf(float range, int varieties, float width, float height,
        Effect* effect) :
        _range(range),  _varieties(varieties),
        _width(width), _height(height), _effect(effect) {}

    MakeTreesLeaf(const MakeTreesLeaf& rhs) :
        _range(rhs._range),
        _varieties(rhs._varieties), _width(rhs._width), _height(rhs._height),
        _effect(rhs._effect)
    {}

    LOD* operator() () const
    {
        if (LOD* result = new LOD; result) {
            // Create a series of LOD nodes so trees cover decreases slightly
            // gradually with distance from _range to 2*_range
            for (float i = 0.0f; i < SG_TREE_FADE_OUT_LEVELS; ++i)
            {
                if (osg::ref_ptr<EffectGeode> geode = createTreeGeode(_width, _height, _varieties); geode) {
                    geode->setEffect(_effect.get());
                    result->addChild(geode, 0, _range * (1.0f + i / (SG_TREE_FADE_OUT_LEVELS - 1.0f)));
                }
            }

            return result;
        }

        return nullptr;
    }

    float _range;
    int _varieties;
    float _width;
    float _height;
    ref_ptr<Effect> _effect;
};

struct AddTreesLeafObject
{
    void operator() (LOD* lod, const TreeBin::Tree& tree) const
    {
        Geode* geode = static_cast<Geode*>(lod->getChild(int(tree.position.x() * 10.0f) % lod->getNumChildren()));
        addTreeToLeafGeode(geode, tree.position);
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
        Vec3 pos = toOsg(tree.position);
        return TreeBin::Tree(toSG(pos * mat));
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

osg::Group* createForest(SGTreeBinList& forestList, const SGReaderWriterOptions* options, int depth)
{
    Matrix transInv = Matrix::identity();
    // Set up some shared structures.
    ref_ptr<Group> group;
    MatrixTransform* mt = new MatrixTransform();

    SGTreeBinList::iterator i;

    for (i = forestList.begin(); i != forestList.end(); ++i) {
        TreeBin* forest = *i;

        ref_ptr<Effect> effect;

        {
            const std::lock_guard<std::mutex> lock(treeEffectMapMutex); // Lock the treeEffectMap for this scope
            EffectMap::iterator iter = treeEffectMap.find(forest->texture);
            if ((iter == treeEffectMap.end())||
                (!iter->second.lock(effect)))
            {
                SGPropertyNode_ptr effectProp = new SGPropertyNode;
                makeChild(effectProp, "inherits-from")->setStringValue(forest->teffect);
                SGPropertyNode* params = makeChild(effectProp, "parameters");
                // emphasize n = 0
                params->getChild("texture", 0, true)->getChild("image", 0, true)
                    ->setStringValue(forest->texture);
                effect = makeEffect(effectProp, true, options);
                if (iter == treeEffectMap.end())
                    treeEffectMap.insert(EffectMap::value_type(forest->texture, effect));
                else
                    iter->second = effect; // update existing, but empty observer
            }
        }

        // Now, create a quadtree for the forest.
        ShaderGeometryQuadtree
            quadtree(GetTreeCoord(), AddTreesLeafObject(),
                     depth,
                     MakeTreesLeaf(forest->range, forest->texture_varieties,
                                   forest->width, forest->height, effect));
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

        insert(Tree(loc));
    }

    stream.close();
};


}
