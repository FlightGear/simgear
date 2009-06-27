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

#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/util/QuadTreeBuilder.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>

#include "ShaderGeometry.hxx"
#include "TreeBin.hxx"

#define SG_TREE_QUAD_TREE_DEPTH 3

// Comments from Tim Moore:
// Some work remains for this code. Stuart's enhancement for multiple
// textures per forest should be integrated. We should try to use one
// ShaderGeometry for *all* the trees in the scene graph and do the
// rotation and scale with a MatrixTransform above the trees quad
// tree. The positions would of course have to be transformed by the
// inverse of that transform. Also, we should investigate whether it
// would be better to instantiate trees as polygons in a osg::Geometry
// object instead of using the ShaderGeometry instancing technique.

using namespace osg;

namespace simgear
{

// memoize geometry
typedef boost::tuple<float, float, int> ForestTuple;
typedef std::map<ForestTuple, ref_ptr<Geometry> > OrthQuadMap;

osg::Geometry* createOrthQuads(float w, float h, int varieties, const osg::Matrix& rotate)
{
    static OrthQuadMap orthQuadMap;
    OrthQuadMap::iterator giter
        = orthQuadMap.find(ForestTuple(w, h, varieties));
    if (giter != orthQuadMap.end())
        return giter->second.get();

    //const osg::Vec3& pos = osg::Vec3(0.0f,0.0f,0.0f),
    // set up the coords
    // Create front and back polygons so we don't need to screw around
    // with two-sided lighting in the shader.
    osg::Vec3Array& v = *(new osg::Vec3Array(8));
    osg::Vec3Array& n = *(new osg::Vec3Array(8));
    osg::Vec2Array& t = *(new osg::Vec2Array(8));
    
    float cw = w*0.5f;

    v[0].set(0.0f,-cw,0.0f);
    v[1].set(0.0f, cw,0.0f);
    v[2].set(0.0f, cw,h);
    v[3].set(0.0f,-cw,h);

    v[4].set(-cw,0.0f,0.0f);
    v[5].set( cw,0.0f,0.0f);
    v[6].set( cw,0.0f,h);
    v[7].set(-cw,0.0f,h);

    // The texture coordinate range is not the
    // entire coordinate space - as the texture
    // has a number of different trees on it.
    float tx = 1.0f/varieties;

    t[0].set(0.0f,0.0f);
    t[1].set(  tx,0.0f);
    t[2].set(  tx,1.0f);
    t[3].set(0.0f,1.0f);

    t[4].set(0.0f,0.0f);
    t[5].set(  tx,0.0f);
    t[6].set(  tx,1.0f);
    t[7].set(0.0f,1.0f);

    // For now the normal is normal to the quad. If we want to get
    // fancier and approximate a cylindrical tree or something, then
    // we would really want more geometry.
    std::fill(n.begin(), n.begin() + 4, Vec3f(1.0f, 0.0f, 0.0f));
    std::fill(n.begin() + 4, n.end(), Vec3f(0.0f, -1.0f, 0.0f));
    for (unsigned int i = 0; i < 8; i++) {
        v[i] = v[i] * rotate;
        // Should be the inverse transpose, but assume that rotate is
        // orthonormal.
        n[i] = n[i] * rotate;     
    }

    osg::Geometry *geom = new osg::Geometry;

    geom->setVertexArray(&v);
    geom->setTexCoordArray(0, &t);
    geom->setNormalArray(&n);
    geom->setNormalBinding(Geometry::BIND_PER_VERTEX);
    // No color for now; that's used to pass the position.
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,8));

    orthQuadMap.insert(std::make_pair(ForestTuple(w, h, varieties), geom));
    return geom;
}

 static char vertexShaderSource[] = 
    "varying float fogFactor;\n"
    "attribute float textureIndex;\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "  gl_TexCoord[0] = gl_MultiTexCoord0 + vec4(textureIndex, 0.0, 0.0, 0.0);\n"
    "  vec3 position = gl_Vertex.xyz * gl_Color.w + gl_Color.xyz;\n"
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
    MakeTreesLeaf(float range, Geometry* geometry, int varieties) :
        _range(range), _geometry(geometry), _varieties(varieties)
    {}
    MakeTreesLeaf(const MakeTreesLeaf& rhs) :
        _range(rhs._range), _geometry(rhs._geometry), _varieties(rhs._varieties) {}
    LOD* operator() () const
    {
        LOD* result = new LOD;
        Geode* geode = new Geode;
        ShaderGeometry* sg = new ShaderGeometry(_varieties);
        sg->setGeometry(_geometry);
        geode->addDrawable(sg);
        result->addChild(geode, 0, _range);
        return result;
    }
    float _range;
    int _varieties;
    Geometry* _geometry;
};

struct AddTreesLeafObject
{
    void operator() (LOD* lod, const TreeBin::Tree& tree) const
    {
        Geode* geode = static_cast<Geode*>(lod->getChild(0));
        ShaderGeometry* sg
            = static_cast<ShaderGeometry*>(geode->getDrawable(0));
        sg->addTree(tree);
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
        return TreeBin::Tree(SGVec3f(pos * mat), tree.texture_index,
                             tree.scale);
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
    osg::Geometry* shared_geometry = createOrthQuads(forest.width, 
                                                     forest.height, 
                                                     forest.texture_varieties,
                                                     ident);

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
            Shader* vertex_shader = new Shader(Shader::VERTEX, vertexShaderSource);
            program->addShader(vertex_shader);
            program->addBindAttribLocation("textureIndex", 1);

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
        ShaderGeometryQuadtree quadtree(GetTreeCoord(),
                                        AddTreesLeafObject(),
                                        SG_TREE_QUAD_TREE_DEPTH,
                                        MakeTreesLeaf(forest.range,
                                                      shared_geometry,
                                                      forest.texture_varieties));
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
