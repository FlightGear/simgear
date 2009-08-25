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
#include <vector>
#include <string>
#include <map>

#include <boost/tuple/tuple_comparison.hpp>

#include <osg/AlphaFunc>
#include <osg/Billboard>
#include <osg/BlendFunc>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/Math>
#include <osg/MatrixTransform>
#include <osg/Matrix>
#include <osg/StateSet>
#include <osg/Texture2D>
#include <osg/TexEnv>

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/util/QuadTreeBuilder.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/structure/OSGUtils.hxx>

#include "ShaderGeometry.hxx"
#include "TreeBin.hxx"

#define SG_TREE_QUAD_TREE_DEPTH 3

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

Geometry* makeSharedTreeGeometry(int numQuads)
{
    // generate a repeatable random seed
    mt seed;
    mt_init(&seed, unsigned(123));
    // set up the coords
    osg::Vec3Array* v = new osg::Vec3Array;
    osg::Vec2Array* t = new osg::Vec2Array;
    v->reserve(numQuads * 4);
    t->reserve(numQuads * 4);
    for (int i = 0; i < numQuads; ++i) {
        // Apply a random scaling factor and texture index.
        float h = (mt_rand(&seed) + mt_rand(&seed)) / 2.0f + 0.5f;
        float cw = h * .5;
        v->push_back(Vec3(0.0f, -cw, 0.0f));
        v->push_back(Vec3(0.0f, cw, 0.0f));
        v->push_back(Vec3(0.0f, cw, h));
        v->push_back(Vec3(0.0f,-cw, h));
        // The texture coordinate range is not the entire coordinate
        // space, as the texture has a number of different trees on
        // it. Here we assign random coordinates and let the shader
        // choose the variety.
        float variety = mt_rand(&seed);
        t->push_back(Vec2(variety, 0.0f));
        t->push_back(Vec2(variety + 1.0f, 0.0f));
        t->push_back(Vec2(variety + 1.0f, 1.0f));
        t->push_back(Vec2(variety, 1.0f));
    }
    Geometry* result = new Geometry;
    result->setVertexArray(v);
    result->setTexCoordArray(0, t);
    result->setComputeBoundingBoxCallback(new TreesBoundingBoxCallback);
    result->setUseDisplayList(false);
    return result;
}

ref_ptr<Geometry> sharedTreeGeometry;

Geometry* createTreeGeometry(float width, float height, int varieties)
{
    if (!sharedTreeGeometry)
        sharedTreeGeometry = makeSharedTreeGeometry(1600);
    Geometry* quadGeom = simgear::clone(sharedTreeGeometry.get(),
                                        CopyOp::SHALLOW_COPY);
    Vec3Array* params = new Vec3Array;
    params->push_back(Vec3(width, height, (float)varieties));
    quadGeom->setNormalArray(params);
    quadGeom->setNormalBinding(Geometry::BIND_OVERALL);
    // Positions
    quadGeom->setColorArray(new Vec3Array);
    quadGeom->setColorBinding(Geometry::BIND_PER_VERTEX);
    FloatArray* rotation = new FloatArray(2);
    (*rotation)[0] = 0.0;
    (*rotation)[1] = PI_2;
    quadGeom->setFogCoordArray(rotation);
    quadGeom->setFogCoordBinding(Geometry::BIND_PER_PRIMITIVE_SET);
    for (int i = 0; i < 2; ++i)
        quadGeom->addPrimitiveSet(new DrawArrays(PrimitiveSet::QUADS));
    return quadGeom;
}

Geode* createTreeGeode(float width, float height, int varieties)
{
    Geode* result = new Geode;
    result->addDrawable(createTreeGeometry(width, height, varieties));
    return result;
}

void addTreeToLeafGeode(Geode* geode, const SGVec3f& p)
{
    const Vec3& pos = p.osg();
    unsigned int numDrawables = geode->getNumDrawables();
    Geometry* geom
        = static_cast<Geometry*>(geode->getDrawable(numDrawables - 1));
    Vec3Array* posArray = static_cast<Vec3Array*>(geom->getColorArray());
    if (posArray->size()
        >= static_cast<Vec3Array*>(geom->getVertexArray())->size()) {
        Vec3Array* paramsArray
            = static_cast<Vec3Array*>(geom->getNormalArray());
        Vec3 params = (*paramsArray)[0];
        geom = createTreeGeometry(params.x(), params.y(), params.z());
        posArray = static_cast<Vec3Array*>(geom->getColorArray());
        geode->addDrawable(geom);
    }
    posArray->insert(posArray->end(), 4, pos);
    size_t numVerts = posArray->size();
    for (int i = 0; i < 2; ++i) {
        DrawArrays* primSet
            = static_cast<DrawArrays*>(geom->getPrimitiveSet(i));
        primSet->setCount(numVerts);
    }
}

 static char vertexShaderSource[] = 
    "varying float fogFactor;\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "  float numVarieties = gl_Normal.z;\n"
    "  float texFract = floor(fract(gl_MultiTexCoord0.x) * numVarieties) / numVarieties;\n"
    "  texFract += floor(gl_MultiTexCoord0.x) / numVarieties;\n"
    "  float sr = sin(gl_FogCoord);\n"
    "  float cr = cos(gl_FogCoord);\n"
    "  gl_TexCoord[0] = vec4(texFract, gl_MultiTexCoord0.y, 0.0, 0.0);\n"
    // scaling
    "  vec3 position = gl_Vertex.xyz * gl_Normal.xxy;\n"
    // Rotation of the generic quad to specific one for the tree.
    "  position.xy = vec2(dot(position.xy, vec2(cr, sr)), dot(position.xy, vec2(-sr, cr)));\n"
    "  position = position + gl_Color.xyz;\n"
    "  gl_Position   = gl_ModelViewProjectionMatrix * vec4(position,1.0);\n"
    "  vec3 ecPosition = vec3(gl_ModelViewMatrix * vec4(position, 1.0));\n"
    "  float n = dot(normalize(gl_LightSource[0].position.xyz), normalize(-ecPosition));\n"
    "  vec3 diffuse = gl_FrontMaterial.diffuse.rgb * max(0.1, n);\n"
    "  vec4 ambientColor = gl_FrontLightModelProduct.sceneColor + gl_LightSource[0].ambient * gl_FrontMaterial.ambient;\n"
    "  gl_FrontColor = ambientColor + gl_LightSource[0].diffuse * vec4(diffuse, 1.0);\n"
    "  gl_BackColor = gl_FrontColor;\n"
    "  float fogCoord = abs(ecPosition.z);\n"
    "  fogFactor = exp( -gl_Fog.density * gl_Fog.density * fogCoord * fogCoord);\n"
    "  fogFactor = clamp(fogFactor, 0.0, 1.0);\n"
    "}\n";

static char fragmentShaderSource[] = 
    "uniform sampler2D baseTexture; \n"
    "varying float fogFactor;\n"
    "\n"
    "void main(void) \n"
    "{ \n"
    "  vec4 base = texture2D( baseTexture, gl_TexCoord[0].st);\n"
    "  vec4 finalColor = base * gl_Color;\n"
    "  gl_FragColor = mix(gl_Fog.color, finalColor, fogFactor );\n"
    "}\n";

typedef std::map<std::string, osg::ref_ptr<StateSet> > StateSetMap;

static StateSetMap treeTextureMap;

// Helper classes for creating the quad tree
namespace
{
struct MakeTreesLeaf
{
    MakeTreesLeaf(float range, int varieties, float width, float height) :
        _range(range),  _varieties(varieties),
        _width(width), _height(height) {}

    MakeTreesLeaf(const MakeTreesLeaf& rhs) :
        _range(rhs._range),
        _varieties(rhs._varieties), _width(rhs._width), _height(rhs._height)
    {}

    LOD* operator() () const
    {
        LOD* result = new LOD;
        Geode* geode = createTreeGeode(_width, _height, _varieties);
        result->addChild(geode, 0, _range);
        return result;
    }
    float _range;
    int _varieties;
    float _width;
    float _height;
};

struct AddTreesLeafObject
{
    void operator() (LOD* lod, const TreeBin::Tree& tree) const
    {
        Geode* geode = static_cast<Geode*>(lod->getChild(0));
        addTreeToLeafGeode(geode, tree.position);
    }
};

struct GetTreeCoord
{
    Vec3 operator() (const TreeBin::Tree& tree) const
    {
        return tree.position.osg();
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
        const Vec3& pos = tree.position.osg();
        return TreeBin::Tree(SGVec3f(pos * mat));
    }
    Matrix mat;
};

// This actually returns a MatrixTransform node. If we rotate the whole
// forest into the local Z-up coordinate system we can reuse the
// primitive tree geometry for all the forests of the same type.

osg::Group* createForest(TreeBin& forest, const osg::Matrix& transform)
{
    Matrix transInv = Matrix::inverse(transform);
    static Matrix ident;
    // Set up some shared structures.
    ref_ptr<Group> group;

    osg::StateSet* stateset = 0;
    StateSetMap::iterator iter = treeTextureMap.find(forest.texture);
    if (iter == treeTextureMap.end()) {
        osg::Texture2D *tex = new osg::Texture2D;
        tex->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP );
        tex->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP );
        tex->setImage(osgDB::readImageFile(forest.texture));

        static ref_ptr<AlphaFunc> alphaFunc;
        static ref_ptr<Program> program;
        static ref_ptr<Uniform> baseTextureSampler;
        static ref_ptr<Material> material;
    
        stateset = new osg::StateSet;
        stateset->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON );
        stateset->setRenderBinDetails(RANDOM_OBJECTS_BIN, "DepthSortedBin");
        if (!program.valid()) {
            alphaFunc = new AlphaFunc;
            alphaFunc->setFunction(AlphaFunc::GEQUAL,0.33f);
            program  = new Program;
            baseTextureSampler = new osg::Uniform("baseTexture", 0);
            Shader* vertex_shader = new Shader(Shader::VERTEX,
                                               vertexShaderSource);
            program->addShader(vertex_shader);
            Shader* fragment_shader = new Shader(Shader::FRAGMENT,
                                                 fragmentShaderSource);
            program->addShader(fragment_shader);
            material = new Material;
            // Don´t track vertex color
            material->setColorMode(Material::OFF);
            material->setAmbient(Material::FRONT_AND_BACK,
                                 Vec4(1.0f, 1.0f, 1.0f, 1.0f));
            material->setDiffuse(Material::FRONT_AND_BACK,
                                 Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        stateset->setAttributeAndModes(alphaFunc.get());
        stateset->setAttribute(program.get());
        stateset->addUniform(baseTextureSampler.get());
        stateset->setMode(GL_VERTEX_PROGRAM_TWO_SIDE, StateAttribute::ON);
        stateset->setAttribute(material.get());

        treeTextureMap.insert(StateSetMap::value_type(forest.texture,
                                                      stateset));
    } else {
        stateset = iter->second.get();
    }
    // Now, create a quadtree for the forest.
    {
        ShaderGeometryQuadtree
            quadtree(GetTreeCoord(), AddTreesLeafObject(),
                     SG_TREE_QUAD_TREE_DEPTH,
                     MakeTreesLeaf(forest.range, forest.texture_varieties,
                                   forest.width, forest.height));
        // Transform tree positions from the "geocentric" positions we
        // get from the scenery polys into the local Z-up coordinate
        // system.
        std::vector<TreeBin::Tree> rotatedTrees;
        rotatedTrees.reserve(forest._trees.size());
        std::transform(forest._trees.begin(), forest._trees.end(),
                       std::back_inserter(rotatedTrees),
                       TreeTransformer(transInv));
        quadtree.buildQuadTree(rotatedTrees.begin(), rotatedTrees.end());
        group = quadtree.getRoot();
    }
    MatrixTransform* mt = new MatrixTransform(transform);
    for (size_t i = 0; i < group->getNumChildren(); ++i)
        mt->addChild(group->getChild(i));
    mt->setStateSet(stateset);
    return mt;
}

}
