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
#include <simgear/structure/OSGUtils.hxx>

#include "ShaderGeometry.hxx"
#include "TreeBin.hxx"

#define SG_TREE_QUAD_TREE_DEPTH 3
#define SG_TREE_FADE_OUT_LEVELS 10

using namespace osg;

namespace simgear
{

bool use_tree_shadows;
bool use_tree_normals;

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

Geometry* makeSharedTreeGeometry(int numQuads)
{
    // generate a repeatable random seed
    pc_init(123);
    // set up the coords
    osg::Vec3Array* v = new osg::Vec3Array;
    osg::Vec2Array* t = new osg::Vec2Array;
    v->reserve(numQuads * 4);
    t->reserve(numQuads * 4);
    for (int i = 0; i < numQuads; ++i) {
        // Apply a random scaling factor and texture index.
        float h = (pc_rand() + pc_rand()) / 2.0f + 0.5f;
        float cw = h * .5;
        v->push_back(Vec3(0.0f, -cw, 0.0f));
        v->push_back(Vec3(0.0f, cw, 0.0f));
        v->push_back(Vec3(0.0f, cw, h));
        v->push_back(Vec3(0.0f,-cw, h));
        // The texture coordinate range is not the entire coordinate
        // space, as the texture has a number of different trees on
        // it. Here we assign random coordinates and let the shader
        // choose the variety.
        float variety = pc_rand();
        t->push_back(Vec2(variety, 0.0f));
        t->push_back(Vec2(variety + 1.0f, 0.0f));
        t->push_back(Vec2(variety + 1.0f, 0.234f));
        t->push_back(Vec2(variety, 0.234f));
    }
    Geometry* result = new Geometry;
    result->setVertexArray(v);
    result->setTexCoordArray(0, t, Array::BIND_PER_VERTEX);
    result->setComputeBoundingBoxCallback(new TreesBoundingBoxCallback);
    //result->setUseDisplayList(false);
    return result;
}

static std::mutex static_sharedGeometryMutex;
static ref_ptr<Geometry> sharedTreeGeometry;

void clearSharedTreeGeometry()
{
    std::lock_guard<std::mutex> g(static_sharedGeometryMutex);
    sharedTreeGeometry = {};
}

Geometry* createTreeGeometry(float width, float height, int varieties)
{
    Geometry* quadGeom = nullptr;
    {
        std::lock_guard<std::mutex> g(static_sharedGeometryMutex);
        if (!sharedTreeGeometry)
            sharedTreeGeometry = makeSharedTreeGeometry(1600);
        quadGeom = simgear::clone(sharedTreeGeometry.get(),
                                  CopyOp::SHALLOW_COPY);
    }

    Vec3Array* params = new Vec3Array;
    params->push_back(Vec3(width, height, (float)varieties));
    quadGeom->setNormalArray(params);
    quadGeom->setNormalBinding(Geometry::BIND_OVERALL);
    // Positions
    quadGeom->setColorArray(new Vec3Array);
    quadGeom->setColorBinding(Geometry::BIND_PER_VERTEX);
    // Normals
    if (use_tree_shadows || use_tree_normals)
	{
    	quadGeom->setSecondaryColorArray(new Vec3Array);
    	quadGeom->setSecondaryColorBinding(Geometry::BIND_PER_VERTEX);
	}
    FloatArray* rotation = new FloatArray(3);
    (*rotation)[0] = 0.0;
    (*rotation)[1] = PI_2;
    if (use_tree_shadows) {(*rotation)[2] = -1.0;}
    quadGeom->setFogCoordArray(rotation);
    quadGeom->setFogCoordBinding(Geometry::BIND_PER_PRIMITIVE_SET);
    // The primitive sets render the same geometry, but the second
    // will rotated 90 degrees by the vertex shader, which uses the
    // fog coordinate as a rotation.
    int imax = 2;
    if (use_tree_shadows) {imax = 3;}
    for (int i = 0; i < imax; ++i)
        quadGeom->addPrimitiveSet(new DrawArrays(PrimitiveSet::QUADS));
    return quadGeom;
}

EffectGeode* createTreeGeode(float width, float height, int varieties)
{
    EffectGeode* result = new EffectGeode;
    result->addDrawable(createTreeGeometry(width, height, varieties));
    return result;
}

void addTreeToLeafGeode(Geode* geode, const SGVec3f& p, const SGVec3f& t)
{
    Vec3 pos = toOsg(p);
    Vec3 ter = toOsg(t);
    unsigned int numDrawables = geode->getNumDrawables();
    Geometry* geom = static_cast<Geometry*>(geode->getDrawable(numDrawables - 1));
    Vec3Array* posArray = static_cast<Vec3Array*>(geom->getColorArray());
    Vec3Array* tnormalArray = NULL;

    if (use_tree_shadows || use_tree_normals) {
        tnormalArray = static_cast<Vec3Array*>(geom->getSecondaryColorArray());
    }

    if (posArray->size() >= static_cast<Vec3Array*>(geom->getVertexArray())->size()) {
        Vec3Array* paramsArray = static_cast<Vec3Array*>(geom->getNormalArray());
        Vec3 params = (*paramsArray)[0];
        geom = createTreeGeometry(params.x(), params.y(), params.z());
        posArray = static_cast<Vec3Array*>(geom->getColorArray());

        if (use_tree_shadows || use_tree_normals) {
            tnormalArray = static_cast<Vec3Array*>(geom->getSecondaryColorArray());
        }
        geode->addDrawable(geom);
    }

    if (tnormalArray && (use_tree_shadows || use_tree_normals))
        tnormalArray->insert(tnormalArray->end(), 4, ter);

    if (posArray)
    {
        posArray->insert(posArray->end(), 4, pos);

        size_t numVerts = posArray->size();
        unsigned int imax = 2;
        if (use_tree_shadows) { imax = 3; }
        for (unsigned int i = 0; i < imax; ++i) {
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
        LOD* result = new LOD;

        // Create a series of LOD nodes so trees cover decreases slightly
        // gradually with distance from _range to 2*_range
        for (float i = 0.0; i < SG_TREE_FADE_OUT_LEVELS; i++)
        {
            EffectGeode* geode = createTreeGeode(_width, _height, _varieties);
            geode->setEffect(_effect.get());
            result->addChild(geode, 0, _range * (1.0 + i / (SG_TREE_FADE_OUT_LEVELS - 1.0)));
        }
        return result;
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
        addTreeToLeafGeode(geode, tree.position, tree.tnormal);
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
    static Matrix ident;
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

        ref_ptr<Effect> effect;
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
    if (!absoluteFileName.exists()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Tree list file " << absoluteFileName << " does not exist.");
        return;
    }

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

        insert(Tree(loc, norm));
    }

    stream.close();
};


}
