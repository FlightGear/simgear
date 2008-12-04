// 3D cloud class
//
// Written by Harald JOHNSEN, started April 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <osg/AlphaFunc>
#include <osg/Program>
#include <osg/Uniform>
#include <osg/ref_ptr>
#include <osg/Texture2D>
#include <osg/NodeVisitor>
#include <osg/PositionAttitudeTransform>
#include <osg/Material>
#include <osgUtil/UpdateVisitor>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>


#include <simgear/compiler.h>

#include <plib/sg.h>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/PathOptions.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>

#include <algorithm>
#include <osg/GLU>

#include "cloudfield.hxx"
#include "newcloud.hxx"
#include "CloudShaderGeometry.hxx"

using namespace simgear;
using namespace osg;

typedef std::map<std::string, osg::ref_ptr<osg::StateSet> > StateSetMap;
typedef std::vector< osg::ref_ptr<osg::Geode> > GeodeList;
typedef std::map<std::string, GeodeList*> CloudMap;

static StateSetMap cloudTextureMap;
static CloudMap cloudMap;
double SGNewCloud::sprite_density = 1.0;
int SGNewCloud::num_flavours = 10;

static char vertexShaderSource[] = 
    "#version 120\n"
    "\n"
    "varying float fogFactor;\n"
    "attribute float textureIndexX;\n"
    "attribute float textureIndexY;\n"
    "attribute float wScale;\n"
    "attribute float hScale;\n"
    "attribute float shade;\n"
    "attribute float cloud_height;\n"
    "void main(void)\n"
    "{\n"
    "  gl_TexCoord[0] = gl_MultiTexCoord0 + vec4(textureIndexX, textureIndexY, 0.0, 0.0);\n"
    "  vec4 ep = gl_ModelViewMatrixInverse * vec4(0.0,0.0,0.0,1.0);\n"
    "  vec4 l  = gl_ModelViewMatrixInverse * vec4(0.0,0.0,1.0,1.0);\n"
    "  vec3 u = normalize(ep.xyz - l.xyz);\n"
// Find a rotation matrix that rotates 1,0,0 into u. u, r and w are
// the columns of that matrix.
    "  vec3 absu = abs(u);\n"
    "  vec3 r = normalize(vec3(-u.y, u.x, 0));\n"
    "  vec3 w = cross(u, r);\n"
// Do the matrix multiplication by [ u r w pos]. Assume no
// scaling in the homogeneous component of pos.
    "  gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "  gl_Position.xyz = gl_Vertex.x * u * wScale;\n"
    "  gl_Position.xyz += gl_Vertex.y * r * hScale;\n"
    "  gl_Position.xyz += gl_Vertex.z * w;\n"
    "  gl_Position.xyz += gl_Color.xyz;\n"
// Determine a lighting normal based on the vertex position from the
// center of the cloud, so that sprite on the opposite side of the cloud to the sun are darker.
    "  float n = dot(normalize(gl_LightSource[0].position.xyz), normalize(mat3x3(gl_ModelViewMatrix) * gl_Position.xyz));\n"
// Determine the position - used for fog and shading calculations        
    "  vec3 ecPosition = vec3(gl_ModelViewMatrix * gl_Position);\n"
    "  float fogCoord = abs(ecPosition.z);\n"
    "  float fract = smoothstep(0.0, cloud_height, gl_Position.z + cloud_height);\n"
// Final position of the sprite
    "  gl_Position = gl_ModelViewProjectionMatrix * gl_Position;\n"
// Limit the normal range from [0,1.0], and apply the shading (vertical factor)
    "  n = min(smoothstep(-0.5, 0.5, n), shade * (1.0 - fract) + fract);\n"
// This lighting normal is then used to mix between almost pure ambient (0) and diffuse (1.0) light
    "  vec4 backlight = 0.9 * gl_LightSource[0].ambient + 0.1 * gl_LightSource[0].diffuse;\n"
    "  gl_FrontColor = mix(backlight, gl_LightSource[0].diffuse, n);\n"
    "  gl_FrontColor += gl_FrontLightModelProduct.sceneColor;\n"
// As we get within 100m of the sprite, it is faded out
    "  gl_FrontColor.a = smoothstep(10.0, 100.0, fogCoord);\n"
    "  gl_BackColor = gl_FrontColor;\n"
// Fog doesn't affect clouds as much as other objects.        
    "  fogFactor = exp( -gl_Fog.density * fogCoord * 0.5);\n"
    "  fogFactor = clamp(fogFactor, 0.0, 1.0);\n"
    "}\n";

static char fragmentShaderSource[] = 
    "uniform sampler2D baseTexture; \n"
    "varying float fogFactor;\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "  vec4 base = texture2D( baseTexture, gl_TexCoord[0].st);\n"
    "  vec4 finalColor = base * gl_Color;\n"
    "  gl_FragColor = mix(gl_Fog.color, finalColor, fogFactor );\n"
    "}\n";

class SGCloudFogUpdateCallback : public osg::StateAttribute::Callback {
    public:
        virtual void operator () (osg::StateAttribute* sa, osg::NodeVisitor* nv)
        {
            SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);
            osg::Fog* fog = static_cast<osg::Fog*>(sa);
            fog->setMode(osg::Fog::EXP);
            fog->setColor(updateVisitor->getFogColor().osg());
            fog->setDensity(updateVisitor->getFogExpDensity());
        }
};

SGNewCloud::SGNewCloud(string type,
                       const SGPath &tex_path, 
                       string tex,
                       double min_w,
                       double max_w,
                       double min_h,
                       double max_h,
                       double min_sprite_w,
                       double max_sprite_w,
                       double min_sprite_h,
                       double max_sprite_h,
                       double b,
                       int n,
                       int nt_x,
                       int nt_y) :
        min_width(min_w),
        max_width(max_w),
        min_height(min_h),
        max_height(max_h),
        min_sprite_width(min_sprite_w),
        max_sprite_width(max_sprite_w),
        min_sprite_height(min_sprite_h),
        max_sprite_height(max_sprite_h),
        bottom_shade(b),
        num_sprites(n),
        num_textures_x(nt_x),
        num_textures_y(nt_y),
        texture(tex),
        name(type)
{
    // Create a new StateSet for the texture, if required.
    StateSetMap::iterator iter = cloudTextureMap.find(texture);

    if (iter == cloudTextureMap.end()) {
        stateSet = new osg::StateSet;
                
        osg::ref_ptr<osgDB::ReaderWriter::Options> options = makeOptionsFromPath(tex_path);
                
        osg::Texture2D *tex = new osg::Texture2D;
        tex->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP );
        tex->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP );
        tex->setImage(osgDB::readImageFile(texture, options.get()));
                
        StateAttributeFactory* attribFactory = StateAttributeFactory::instance();
        
        stateSet->setMode(GL_LIGHTING,  osg::StateAttribute::ON);
        stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        
        // Fog handling
        osg::Fog* fog = new osg::Fog;
        fog->setUpdateCallback(new SGCloudFogUpdateCallback);
        stateSet->setAttributeAndModes(fog);
        stateSet->setDataVariance(osg::Object::DYNAMIC);
        
        stateSet->setAttributeAndModes(attribFactory->getSmoothShadeModel());
        stateSet->setAttributeAndModes(attribFactory->getStandardBlendFunc());

        stateSet->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON );
        stateSet->setRenderBinDetails(osg::StateSet::TRANSPARENT_BIN, "DepthSortedBin");
                
        static ref_ptr<AlphaFunc> alphaFunc;
        static ref_ptr<Program> program;
        static ref_ptr<Uniform> baseTextureSampler;
        static ref_ptr<Material> material;
                  
        // Generate the shader etc, if we don't already have one.
        if (!program.valid()) {
            alphaFunc = new AlphaFunc;
            alphaFunc->setFunction(AlphaFunc::GREATER,0.001f);
            program  = new Program;
            baseTextureSampler = new osg::Uniform("baseTexture", 0);
            Shader* vertex_shader = new Shader(Shader::VERTEX, vertexShaderSource);
            program->addShader(vertex_shader);
            program->addBindAttribLocation("textureIndexX", CloudShaderGeometry::TEXTURE_INDEX_X);
            program->addBindAttribLocation("textureIndexY", CloudShaderGeometry::TEXTURE_INDEX_Y);
            program->addBindAttribLocation("wScale", CloudShaderGeometry::WIDTH);
            program->addBindAttribLocation("hScale", CloudShaderGeometry::HEIGHT);
            program->addBindAttribLocation("shade", CloudShaderGeometry::SHADE);
            program->addBindAttribLocation("cloud_height", CloudShaderGeometry::CLOUD_HEIGHT);
                  
            Shader* fragment_shader = new Shader(Shader::FRAGMENT, fragmentShaderSource);
            program->addShader(fragment_shader);
            material = new Material;
            // Don´t track vertex color
            material->setColorMode(Material::OFF);
            
            // We don't actually use the material information either - see shader.
            material->setAmbient(Material::FRONT_AND_BACK,
                                 Vec4(0.5f, 0.5f, 0.5f, 1.0f));
            material->setDiffuse(Material::FRONT_AND_BACK,
                                 Vec4(0.5f, 0.5f, 0.5f, 1.0f));
        }
                  
        stateSet->setAttributeAndModes(alphaFunc.get());
        stateSet->setAttribute(program.get());
        stateSet->addUniform(baseTextureSampler.get());
        stateSet->setMode(GL_VERTEX_PROGRAM_TWO_SIDE, StateAttribute::ON);
        stateSet->setAttribute(material.get());
                
        // Add the newly created texture to the map for use later.
        cloudTextureMap.insert(StateSetMap::value_type(texture,  stateSet));
    } else {
        stateSet = iter->second.get();
    }
    
    quad = createOrthQuad(min_sprite_width, min_sprite_height, num_textures_x, num_textures_y);
}

SGNewCloud::~SGNewCloud() {
}

osg::Geometry* SGNewCloud::createOrthQuad(float w, float h, int varieties_x, int varieties_y)
{
    // Create front and back polygons so we don't need to screw around
    // with two-sided lighting in the shader.
    osg::Vec3Array& v = *(new osg::Vec3Array(4));
    osg::Vec3Array& n = *(new osg::Vec3Array(4));
    osg::Vec2Array& t = *(new osg::Vec2Array(4));
    
    float cw = w*0.5f;

    v[0].set(0.0f, -cw, 0.0f);
    v[1].set(0.0f,  cw, 0.0f);
    v[2].set(0.0f,  cw, h);
    v[3].set(0.0f, -cw, h);
    
    // The texture coordinate range is not the
    // entire coordinate space - as the texture
    // has a number of different clouds on it.
    float tx = 1.0f/varieties_x;
    float ty = 1.0f/varieties_y;

    t[0].set(0.0f, 0.0f);
    t[1].set(  tx, 0.0f);
    t[2].set(  tx, ty);
    t[3].set(0.0f, ty);

    // The normal isn't actually use in lighting.
    n[0].set(1.0f, -1.0f, -1.0f);
    n[1].set(1.0f,  1.0f, -1.0f);
    n[2].set(1.0f,  1.0f,  1.0f);
    n[3].set(1.0f, -1.0f,  1.0f);

    osg::Geometry *geom = new osg::Geometry;

    geom->setVertexArray(&v);
    geom->setTexCoordArray(0, &t);
    geom->setNormalArray(&n);
    geom->setNormalBinding(Geometry::BIND_PER_VERTEX);
    // No color for now; that's used to pass the position.
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4));

    return geom;
}

// return a random number between -n/2 and n/2, tending to 0
static float Rnd(float n) {
    return n * (-0.5f + (sg_random() + sg_random()) / 2.0f);
}

osg::ref_ptr<Geode> SGNewCloud::genCloud() {
    
    CloudMap::iterator iter = cloudMap.find(name);
    osg::ref_ptr<osg::Geode> geode;
    
    // We generate up to num_flavours of different versions
    // of the same cloud before we start re-using them. This
    // allows us to strike a balance between performance and
    // visual complexity.
    
    GeodeList* g = (*iter).second;

    if (iter == cloudMap.end() || g->size() < num_flavours) 
    {
        geode = new Geode;
        
        CloudShaderGeometry* sg = new CloudShaderGeometry(num_textures_x, num_textures_y, max_width, max_height);
        
        // Determine how big this specific cloud instance is. Note that we subtract
        // the sprite size because the width/height is used to define the limits of
        // the center of the sprites, not their edges.
        float width = min_width + sg_random() * (max_width - min_width) - min_sprite_width;
        float height = min_height + sg_random() * (max_height - min_height) - min_sprite_height;
        
        // Determine the cull distance. This is used to remove sprites that are too close together.
        // The value is squared as we use vector calculations.
        float cull_distance_squared = min_sprite_height * min_sprite_height * 0.1f;
        
        // The number of sprites we actually used is a function of the (user-controlled) density
        int n_sprites = num_sprites * sprite_density;
        
        for (int i = 0; i < n_sprites; i++)
        {
            // Determine the position of the sprite. Rather than being completely random,
            // we place them on the surface of a distorted sphere. However, we place
            // the first and second sprites on the top and bottom, and the third in the
            // center of the sphere (and at maximum size) to ensure good coverage and
            // reduce the chance of there being "holes" in our cloud.
            float x, y, z;
            
            if (i == 0) {
                x = 0;
                y = 0;
                z = height * 0.5f;
            } else if (i == 1) {
                x = 0;
                y = 0;
                z = - height * 0.5f;
            } else if (i == 2) {
                x = 0;
                y = 0;
                z = 0;
            } else {
                double theta = sg_random() * SGD_2PI;
                double elev  = sg_random() * SGD_PI;
                x = width * cos(theta) * 0.5f * sin(elev);
                y = width * sin(theta) * 0.5f * sin(elev);
                z = height * cos(elev) * 0.5f; 
            }
            
            SGVec3f *pos = new SGVec3f(x, y, z); 
    
            // Determine the height and width as scaling factors on the minimum size (used to create the quad)
            float sprite_width = 1.0f + sg_random() * (max_sprite_width - min_sprite_width) / min_sprite_width;
            float sprite_height = 1.0f + sg_random() * (max_sprite_height - min_sprite_height) / min_sprite_height;
            
            if (i == 2) {
                // The center sprite is always maximum size to fill up any holes.
                sprite_width = 1.0f + (max_sprite_width - min_sprite_width) / min_sprite_width;
                sprite_height = 1.0f + (max_sprite_height - min_sprite_height) / min_sprite_height;
            }
            
            // Determine the sprite texture indexes;
            int index_x = (int) floor(sg_random() * num_textures_x);
            if (index_x == num_textures_x) { index_x--; }
            
            int index_y = (int) floor(sg_random() * num_textures_y);
            if (index_y == num_textures_y) { index_y--; }
            
            sg->addSprite(*pos, 
                        index_x, 
                        index_y, 
                        sprite_width, 
                        sprite_height, 
                        bottom_shade, 
                        cull_distance_squared, 
                        height * 0.5f);
        }
        
        
        sg->setGeometry(quad);
        geode->addDrawable(sg);
        geode->setName("3D cloud");
        geode->setStateSet(stateSet.get());
        
        if (iter == cloudMap.end())
        {
            // This is the first of this cloud to be generated.
            GeodeList* geodelist = new GeodeList;
            geodelist->push_back(geode);
            cloudMap.insert(CloudMap::value_type(name,  geodelist));
        }
        else
        {
            // Add the new cloud to the list of geodes
            (*iter).second->push_back(geode.get());
        }
    
    } else {
        
        int index = sg_random() * num_flavours;
        if (index == num_flavours) index--;
        
        geode = iter->second->at(index);
    }
    
    return geode;
}

