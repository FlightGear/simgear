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

#include "ShaderGeometry.hxx"
#include "TreeBin.hxx"

#define SG_TREE_QUAD_TREE_SIZE 32

namespace simgear
{

osg::Geometry* createOrthQuads(float w, float h, const osg::Matrix& rotate)
{

    //const osg::Vec3& pos = osg::Vec3(0.0f,0.0f,0.0f),
    // set up the coords
    osg::Vec3Array& v = *(new osg::Vec3Array(8));
    osg::Vec2Array& t = *(new osg::Vec2Array(8));
    
    /*
    float rotation = 0.0f;
    float sw = sinf(rotation)*w*0.5f;
    float cw = cosf(rotation)*w*0.5f;

    v[0].set(pos.x()-sw,pos.y()-cw,pos.z()+0.0f);
    v[1].set(pos.x()+sw,pos.y()+cw,pos.z()+0.0f);
    v[2].set(pos.x()+sw,pos.y()+cw,pos.z()+h);
    v[3].set(pos.x()-sw,pos.y()-cw,pos.z()+h);

    v[4].set(pos.x()-cw,pos.y()+sw,pos.z()+0.0f);
    v[5].set(pos.x()+cw,pos.y()-sw,pos.z()+0.0f);
    v[6].set(pos.x()+cw,pos.y()-sw,pos.z()+h);
    v[7].set(pos.x()-cw,pos.y()+sw,pos.z()+h);
    */
    float cw = w*0.5f;

    v[0].set(0.0f,-cw,0.0f);
    v[1].set(0.0f, cw,0.0f);
    v[2].set(0.0f, cw,h);
    v[3].set(0.0f,-cw,h);

    v[4].set(-cw,0.0f,0.0f);
    v[5].set( cw,0.0f,0.0f);
    v[6].set( cw,0.0f,h);
    v[7].set(-cw,0.0f,h);

    t[0].set(0.0f,0.0f);
    t[1].set(1.0f,0.0f);
    t[2].set(1.0f,1.0f);
    t[3].set(0.0f,1.0f);

    t[4].set(0.0f,0.0f);
    t[5].set(1.0f,0.0f);
    t[6].set(1.0f,1.0f);
    t[7].set(0.0f,1.0f);
    
    for (unsigned int i = 0; i < 8; i++)
    {
      v[i] = v[i] * rotate;      
    }

    osg::Geometry *geom = new osg::Geometry;

    geom->setVertexArray( &v );

    geom->setTexCoordArray( 0, &t );

    geom->addPrimitiveSet( new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,8) );

    return geom;
}

osg::Group* createForest(const TreeBin& forest, const osg::Matrix& transform)
{
    // Set up some shared structures. 
    // FIXME: Currently we only take the texture, height and width of the first tree in the forest. In the future
    // we should be able to handle multiple textures etc.    
    TreeBin::Tree firstTree = forest.getTree(0);
    
    osg::Geometry* shared_geometry = createOrthQuads(firstTree.width, 
                                                     firstTree.height, 
                                                     transform);
    osg::Group* group = new osg::Group;

    osg::Texture2D *tex = new osg::Texture2D;
    tex->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP );
    tex->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP );
    tex->setImage(osgDB::readImageFile(firstTree.texture));

    osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
    alphaFunc->setFunction(osg::AlphaFunc::GEQUAL,0.05f);

    osg::StateSet *dstate = new osg::StateSet;
    dstate->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON );
    dstate->setTextureAttribute(0, new osg::TexEnv );
    dstate->setAttributeAndModes( new osg::BlendFunc, osg::StateAttribute::ON );
    dstate->setAttributeAndModes( alphaFunc, osg::StateAttribute::ON );
    dstate->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    dstate->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
    
    osg::StateSet* stateset = new osg::StateSet;
    stateset->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON );
    stateset->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );

    osg::Program* program = new osg::Program;
    stateset->setAttribute(program);
    osg::Uniform* baseTextureSampler = new osg::Uniform("baseTexture",0);
    stateset->addUniform(baseTextureSampler);

    /*
     * FIXME: Currently, calculating the diffuse term results in a bad 
     * "flickering" and a tendency of the diffuse term be either 
     * 0.0 of 1.0. Hence, it has been commented out in the shader below.
     * I (Stuart) suspect it may be because the light is so distant that
     * we're seeing floating point representation issues.
     */
    char vertexShaderSource[] = 
//        "varying vec3 N;\n"
//        "varying vec3 v;\n"
        "varying vec2 texcoord;\n"
        "varying float fogFactor;\n"
        "\n"
        "void main(void)\n"
        "{\n"
//        "  v = vec3(gl_ModelViewMatrix * gl_Vertex);\n"
//        "  N = normalize(gl_NormalMatrix * gl_Normal);\n"
        "  texcoord = gl_MultiTexCoord0.st;\n"        
        "  vec3 position = gl_Vertex.xyz * gl_Color.w + gl_Color.xyz;\n"
        "  gl_Position   = gl_ModelViewProjectionMatrix * vec4(position,1.0);\n"
	    "  const float LOG2 = 1.442695;\n"
	    "  gl_FogFragCoord = gl_Position.z;\n"
	    "  fogFactor = exp2( -gl_Fog.density * gl_Fog.density * gl_FogFragCoord * gl_FogFragCoord * LOG2 );\n"
	    "  fogFactor = clamp(fogFactor, 0.0, 1.0);\n"
        "}\n";

    char fragmentShaderSource[] = 
        "uniform sampler2D baseTexture; \n"
//        "varying vec3 N;\n"
//        "varying vec3 v;\n"
        "varying vec2 texcoord;\n"
        "varying float fogFactor;\n"
        "\n"
        "void main(void) \n"
        "{ \n"
        "  vec4 base = texture2D( baseTexture, texcoord);\n"
//        "  vec3 L = normalize(gl_LightSource[0].position.xyz);\n"
//        "  vec4 vDiffuse = gl_FrontLightProduct[0].diffuse * max(dot(N,L), 0.0);\n"
//        "  vDiffuse = sqrt(clamp(vDiffuse, 0.0, 1.0));\n"
//        "  vec4 vAmbient = gl_FrontLightProduct[0].ambient;\n"
//        "  vec4 finalColor = base * (vAmbient + vDiffuse);\n"
          "  vec4 finalColor = base * gl_FrontLightProduct[0].diffuse;\n"
          "  gl_FragColor = mix(gl_Fog.color, finalColor, fogFactor );\n"
        "}\n";

    osg::Shader* vertex_shader = new osg::Shader(osg::Shader::VERTEX, vertexShaderSource);
    program->addShader(vertex_shader);
    
    osg::Shader* fragment_shader = new osg::Shader(osg::Shader::FRAGMENT, fragmentShaderSource);
    program->addShader(fragment_shader);
   
    // Now, create a quadtree for the forest.
    osg::ref_ptr<osg::Group> _root;
    ShaderGeometry* leaves[SG_TREE_QUAD_TREE_SIZE][SG_TREE_QUAD_TREE_SIZE];

    // Determine the extents of the tree, and a list of the required textures for later.    
    osg::BoundingBox extents;
    for (unsigned int i = 0; i < forest.getNumTrees(); i++)
    {    
        const osg::Vec3f center = forest.getTree(i).position.osg() * transform;
        extents.expandBy(center);
    }
    
    const osg::Vec2 quadMin(extents.xMin(), extents.yMin());
    const osg::Vec2 quadMax(extents.xMax(), extents.yMax());

    for (int i = 0; i < SG_TREE_QUAD_TREE_SIZE; ++i) {
      osg::LOD* interior = new osg::LOD;      
      //osg::Group* interior = new osg::Group;
      group->addChild(interior);
      for (int j = 0; j < SG_TREE_QUAD_TREE_SIZE; ++j) {
        osg::Geode* geode = new osg::Geode;
        leaves[i][j] = new ShaderGeometry();
        leaves[i][j]->setGeometry(shared_geometry);
        geode->setStateSet(stateset);
        geode->addDrawable(leaves[i][j]);
        interior->addChild(geode, 0, firstTree.range);        
      }
    }
    
    // Now we've got our quadtree, add the trees based on location.
    
    for (unsigned int i = 0; i < forest.getNumTrees(); i++)
    {    
      TreeBin::Tree t = forest.getTree(i);
      osg::Vec3 center = t.position.osg() * transform;

      int x = (int)(SG_TREE_QUAD_TREE_SIZE * (center.x() - quadMin.x()) / (quadMax.x() - quadMin.x()));
      x = osg::clampTo(x, 0, (SG_TREE_QUAD_TREE_SIZE - 1));
      int y = (int)(SG_TREE_QUAD_TREE_SIZE * (center.y() - quadMin.y()) / (quadMax.y() - quadMin.y()));
      y = osg::clampTo(y, 0, (SG_TREE_QUAD_TREE_SIZE -1));
    
      leaves[y][x]->addTree(t.position.osg(), t.height);
    }
    
    return group;
}

}
