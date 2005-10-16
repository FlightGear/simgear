// non fixed Opengl pipeline rendering
//
// Written by Harald JOHNSEN, started Jully 2005.
//
// Copyright (C) 2005  Harald JOHNSEN
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
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//


#include <plib/sg.h>
#include <plib/ssg.h>
#include <plib/ul.h>

#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/screen/extensions.hxx>

#include <simgear/debug/logstream.hxx>

#include <simgear/screen/shader.h>

#include "animation.hxx"
/*
    <animation>
        <type>shader</type>
        <shader>fresnel</shader>
        <object-name>...</object-name>
    </animation>

    <animation>
        <type>shader</type>
        <shader>heat-haze</shader>
        <object-name>...</object-name>
        <speed>...</speed>
        <speed-prop>...</speed-prop>
        <factor>...</factor>
        <factor-prop>...</factor-prop>
    </animation>

    <animation>
        <type>shader</type>
        <shader>chrome</shader>
        <texture>...</texture>
        <object-name>...</object-name>
    </animation>

    <animation>
        <type>shader</type>
        <shader></shader>
        <object-name>...</object-name>
        <depth-test>false</depth-test>
    </animation>

*/
static Shader *shFresnel=NULL;
static GLuint texFresnel = 0;

static GLuint texBackground = 0;
static int texBackgroundWidth = 1024, texBackgroundHeight = 1024;
static GLenum texBackgroundTarget = GL_TEXTURE_2D;
static bool isRectangleTextureSupported = false;
static bool istexBackgroundRectangle = false;
static bool initDone = false;
static bool haveBackground = false;

static glActiveTextureProc glActiveTexturePtr = 0;
static double totalTime = 0.0;
static sgMat4 shadIdentMatrix;


static int null_shader_callback( ssgEntity *e ) {
 	GLuint dlist = 0;
    ssgLeaf *leaf = (ssgLeaf *) e;
#ifdef _SSG_USE_DLIST
    dlist = leaf->getDListIndex();
    if( ! dlist ) {
        leaf->makeDList();
        dlist = leaf->getDListIndex();
    }
#endif
    if( ! dlist )
        return true;
    ssgSimpleState *sst = ((ssgSimpleState *)leaf->getState());
    if ( sst )
        sst->apply();

    SGShaderAnimation *my_shader = (SGShaderAnimation *) ( e->getUserData() );
    if( ! my_shader->_depth_test )
        glDisable( GL_DEPTH_TEST );
    glCallList ( dlist ) ;
    // restore states
    if( ! my_shader->_depth_test )
        glEnable( GL_DEPTH_TEST );

    // don't draw !
    return false;
}

static int heat_haze_shader_callback( ssgEntity *e ) {
 	GLuint dlist = 0;
    ssgLeaf *leaf = (ssgLeaf *) e;
#ifdef _SSG_USE_DLIST
    dlist = leaf->getDListIndex();
    if( ! dlist ) {
        leaf->makeDList();
        dlist = leaf->getDListIndex();
    }
#endif
    if( ! dlist )
        return true;

    GLint viewport[4];
    glGetIntegerv( GL_VIEWPORT, viewport );
    const int screen_width = viewport[2];
    const int screen_height = viewport[3];
    if( ! haveBackground ) {
        // store the backbuffer in a texture
        if( ! texBackground ) {
            // allocate our texture here so we don't waste memory if no model use that effect
            // check if we need a rectangle texture and if the card support it
            if( (screen_width > 1024 || screen_height > 1024) && isRectangleTextureSupported ) {
                // Note that the 3 (same) extensions use the same enumerants
                texBackgroundTarget = GL_TEXTURE_RECTANGLE_NV;
                istexBackgroundRectangle = true;
                texBackgroundWidth = screen_width;
                texBackgroundHeight = screen_height;
            }
            glGenTextures(1, &texBackground);
            glEnable(texBackgroundTarget);
            glBindTexture(texBackgroundTarget, texBackground);
            // trying to match the backbuffer pixel format
            GLint internalFormat = GL_RGB8;
            GLint colorBits = 0, alphaBits = 0;
            glGetIntegerv( GL_BLUE_BITS, &colorBits );
            glGetIntegerv( GL_ALPHA_BITS, &alphaBits );
            if(colorBits == 5) {
                if( alphaBits == 0 )
                    internalFormat = GL_RGB5;
                else
                    internalFormat = GL_RGB5_A1;
            } else {
                if( alphaBits != 0 )
                    internalFormat = GL_RGBA8;
            }
            glTexImage2D(texBackgroundTarget, 0, internalFormat, 
                            texBackgroundWidth, texBackgroundHeight, 0, GL_RGB, GL_FLOAT, NULL);

            glTexParameteri(texBackgroundTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(texBackgroundTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(texBackgroundTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(texBackgroundTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        glEnable(texBackgroundTarget);
        glBindTexture(texBackgroundTarget, texBackground);
        // center of texture = center of screen
        // obviously we don't have the whole screen if screen_width > texBackgroundWidth
        // if rectangle textures are not supported, this give some artifacts on the borders
        if( istexBackgroundRectangle ) {
            glCopyTexSubImage2D( texBackgroundTarget, 0, 0, 0, 
                0, 0, texBackgroundWidth, texBackgroundHeight );
        } else {
            glCopyTexSubImage2D( texBackgroundTarget, 0, 0, 0, 
                (screen_width - texBackgroundWidth) / 2, 
                (screen_height - texBackgroundHeight) / 2, 
                texBackgroundWidth, texBackgroundHeight );
        }
        haveBackground = true;
        glBindTexture(texBackgroundTarget, 0);
        glDisable(texBackgroundTarget);
    }
    ssgSimpleState *sst = ((ssgSimpleState *)leaf->getState());
    if ( sst )
        sst->apply();

    SGShaderAnimation *my_shader = (SGShaderAnimation *) ( e->getUserData() );
    if( ! my_shader->_depth_test )
        glDisable( GL_DEPTH_TEST );
    glDepthMask( GL_FALSE );
    glDisable( GL_LIGHTING );
    if(1) {
        // noise texture, tex coord from the model translated by a time factor
        glActiveTexturePtr( GL_TEXTURE0_ARB );
        glEnable(GL_TEXTURE_2D);
        const float noiseDist = fmod(- totalTime * my_shader->_factor * my_shader->_speed, 4.0);
        glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glTranslatef( noiseDist, 0.0f, 0.0f );
        glMatrixMode(GL_MODELVIEW);

        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

        // background texture
        glActiveTexturePtr( GL_TEXTURE1_ARB );
        glEnable(texBackgroundTarget);
        glBindTexture(texBackgroundTarget, texBackground);

        // automatic generation of texture coordinates
        // map to screen space
        sgMat4 CameraProjM, CameraViewM;
        glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *) CameraProjM);
        glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) CameraViewM);
        const float dummy_scale = 1.0f; //0.95f;
        const float deltaPos = 0.05f;
        glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            if( istexBackgroundRectangle ) {
                // coords go from 0.0 to n, not from 0.0 to 1.0
                glTranslatef( texBackgroundWidth * 0.5f, texBackgroundHeight * 0.5f, 0.0f );
                glScalef( texBackgroundWidth * 0.5f,
                    texBackgroundHeight * 0.5f, 1.0f );
            } else {
                glTranslatef( 0.5f, 0.5f, 0.0f );
                glScalef( float( screen_width ) / float( texBackgroundWidth ) * 0.5f,
                    float( screen_height ) / float( texBackgroundHeight ) * 0.5f, 1.0f );
            }
            glMultMatrixf( (GLfloat *) CameraProjM );
            glMultMatrixf( (GLfloat *) CameraViewM );
            glTranslatef( deltaPos, deltaPos, deltaPos );
        glMatrixMode(GL_MODELVIEW);

        glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR );
        glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR );
        glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR );
        glTexGeni( GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR );
        glTexGenfv( GL_S, GL_EYE_PLANE, shadIdentMatrix[0] );
        glTexGenfv( GL_T, GL_EYE_PLANE, shadIdentMatrix[1] );
        glTexGenfv( GL_R, GL_EYE_PLANE, shadIdentMatrix[2] );
        glTexGenfv( GL_Q, GL_EYE_PLANE, shadIdentMatrix[3] );
        glEnable( GL_TEXTURE_GEN_S );
        glEnable( GL_TEXTURE_GEN_T );
        glEnable( GL_TEXTURE_GEN_R );
        glEnable( GL_TEXTURE_GEN_Q );

        sgVec4 enviro = {1.00f, 1.00f, 1.00f, 0.85f};

        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE ); 
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR ); 
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR ); 
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, enviro);

        glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE0_ARB);
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PRIMARY_COLOR_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA ); 

        glCallList ( dlist ) ;
        glMatrixMode(GL_TEXTURE);
        glTranslatef( - deltaPos*2.0f, -deltaPos*2.5f, -deltaPos*2.0f );
        glMatrixMode(GL_MODELVIEW);
        glCallList ( dlist ) ;

        // alter colors only on last rendering
        sgVec4 fLight = {0.93f, 0.93f, 1.00f, 0.85f};
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR ); 

        glMatrixMode(GL_TEXTURE);
        glTranslatef( deltaPos*0.7f, deltaPos*1.7f, deltaPos*0.7f );
        glMatrixMode(GL_MODELVIEW);
        glCallList ( dlist ) ;


        glActiveTexturePtr( GL_TEXTURE1_ARB );
        glDisable( GL_TEXTURE_GEN_S );
        glDisable( GL_TEXTURE_GEN_T );
        glDisable( GL_TEXTURE_GEN_R );
        glDisable( GL_TEXTURE_GEN_Q );
        glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
        glDisable(texBackgroundTarget);
        glActiveTexturePtr( GL_TEXTURE0_ARB );
        glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // restore states
    if( ! my_shader->_depth_test )
        glEnable( GL_DEPTH_TEST );

    glEnable( GL_LIGHTING );
    glDepthMask( GL_TRUE );
    if( sst )
        sst->force();

   // don't draw !
    return false;
}

static int fresnel_shader_callback( ssgEntity *e ) {
 	GLuint dlist = 0;
    ssgLeaf *leaf = (ssgLeaf *) e;
#ifdef _SSG_USE_DLIST
    dlist = leaf->getDListIndex();
    if( ! dlist ) {
        leaf->makeDList();
        dlist = leaf->getDListIndex();
    }
#endif
    if( ! dlist )
        return true;
    ssgSimpleState *sst = ((ssgSimpleState *)leaf->getState());
    if ( sst )
        sst->apply();

    sgVec4 sunColor, ambientColor;
    ssgGetLight( 0 )->getColour(GL_DIFFUSE, sunColor );
    ssgGetLight( 0 )->getColour(GL_AMBIENT, ambientColor );

    SGShaderAnimation *my_shader = (SGShaderAnimation *) ( e->getUserData() );
    glEnable(GL_BLEND);
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ;
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);

	if( true ) {
//        sgVec4 R = {0.5,0.0,0.0,0.0};
        sgVec4 enviro = {1.0,0.0,0.0,1.0};
//        sgCopyVec4( enviro, sunColor );
        glActiveTexturePtr( GL_TEXTURE0_ARB );
        glEnable(GL_TEXTURE_2D);
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
        glActiveTexturePtr( GL_TEXTURE1_ARB );
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_1D);
        glBindTexture(GL_TEXTURE_1D, texFresnel);
        // c = a0 * a2 + a1 * (1-a2)
//        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
//        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB ); 
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_CONSTANT_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR ); 
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR ); 
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_COLOR ); 
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, enviro);
        shFresnel->enable();
            shFresnel->bind();
            glCallList ( dlist ) ;
        shFresnel->disable();
        glActiveTexturePtr( GL_TEXTURE1_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
        glDisable(GL_TEXTURE_1D);
        glActiveTexturePtr( GL_TEXTURE0_ARB );
        glDisable(GL_TEXTURE_1D);
        glEnable(GL_TEXTURE_2D);
    }
    // restore states
//    glBindTexture(GL_TEXTURE_2D, 0);
//    glDepthFunc(GL_LESS);
//    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ;
   if( sst )
	    sst->force();

    // don't draw !
    return false;
}



static int chrome_shader_callback( ssgEntity *e ) {
 	GLuint dlist = 0;
    ssgLeaf *leaf = (ssgLeaf *) e;
#ifdef _SSG_USE_DLIST
    dlist = leaf->getDListIndex();
    if( ! dlist ) {
        leaf->makeDList();
        dlist = leaf->getDListIndex();
    }
#endif
    if( ! dlist )
        return true;
    ssgSimpleState *sst = ((ssgSimpleState *)leaf->getState());
    if ( sst )
        sst->apply();

    SGShaderAnimation *my_shader = (SGShaderAnimation *) ( e->getUserData() );
    if( ! my_shader->_depth_test )
        glDisable( GL_DEPTH_TEST );

    GLint maskTexComponent = 3;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS, &maskTexComponent);

    // The fake env chrome texture
    glActiveTexturePtr( GL_TEXTURE1_ARB );
    glEnable(GL_TEXTURE_2D);
    {
        // No lighting is computed in spherical mapping mode because the environment
        // is supposed to be allready lighted. We must reshade our environment texture.
        sgVec4 sunColor, ambientColor, envColor;
        ssgGetLight( 0 )->getColour(GL_DIFFUSE, sunColor );
        ssgGetLight( 0 )->getColour(GL_AMBIENT, ambientColor );
        sgAddScaledVec3( envColor, ambientColor, sunColor, 0.4f);
        glBindTexture(GL_TEXTURE_2D, my_shader->_effectTexture->getHandle());

        sgVec3 delta_light;
        sgSubVec3(delta_light, envColor, my_shader->_envColor);
        if( (fabs(delta_light[0]) + fabs(delta_light[1]) + fabs(delta_light[2])) > 0.05f ) {
		    sgCopyVec3( my_shader->_envColor, envColor );
            // reload the texture data and let the driver reshade it for us
            glPixelTransferf( GL_RED_SCALE, envColor[0] );
            glPixelTransferf( GL_GREEN_SCALE, envColor[1] );
            glPixelTransferf( GL_BLUE_SCALE, envColor[2] );
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, my_shader->_texWidth, my_shader->_texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, my_shader->_textureData);
            glPixelTransferf( GL_RED_SCALE, 1.0f );
            glPixelTransferf( GL_GREEN_SCALE, 1.0f );
            glPixelTransferf( GL_BLUE_SCALE, 1.0f );
        }
    }
    if( maskTexComponent == 4 ) {
        // c = lerp(model tex, chrome tex, model tex alpha)
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB ); 
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR ); 
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR ); 
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_PREVIOUS_ARB );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA ); 

        glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
    } else {
        // c = chrome tex
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE );
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR ); 

        glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
        glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
        glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
    }
    // automatic generation of texture coordinates
    // from normals

    glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
    glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
    glEnable( GL_TEXTURE_GEN_S );
    glEnable( GL_TEXTURE_GEN_T );

    glCallList ( dlist ) ;

    glActiveTexturePtr( GL_TEXTURE1_ARB );
    glDisable( GL_TEXTURE_GEN_S );
    glDisable( GL_TEXTURE_GEN_T );

    glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexturePtr( GL_TEXTURE0_ARB );

    // restore states
    if( ! my_shader->_depth_test )
        glEnable( GL_DEPTH_TEST );

    if( sst )
        sst->force();

   // don't draw !
    return false;
}

static void init_shaders(void) {
	Shader::Init();
    if( false && Shader::is_VP_supported() ) {
	    shFresnel = new Shader("/FlightGear/data/Textures/fresnel_vp.txt", "fresnel_vp");
//        shFresnel->bindNames("somedata", 0);
    }
	glActiveTexturePtr = (glActiveTextureProc) SGLookupFunction("glActiveTextureARB");
    const int fresnelSize = 512;
    unsigned char imageFresnel[ fresnelSize * 3 ];
    for(int i = 0; i < fresnelSize; i++) {
        const float R0 = 0.2f;
        float NdotV = float( i ) / float( fresnelSize );
        float f = R0 + (1.0f-R0)*pow(1.0f - NdotV, 5);
        unsigned char ff = (unsigned char) (f * 255.0);
        imageFresnel[i*3+0] = imageFresnel[i*3+1] = imageFresnel[i*3+2] = ff;
    }
    glGenTextures( 1, &texFresnel );
	glBindTexture(GL_TEXTURE_1D, texFresnel );
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_GENERATE_MIPMAP_SGIS, true);
	glTexImage1D(GL_TEXTURE_1D, 0, 3, fresnelSize, 0, GL_RGB, GL_UNSIGNED_BYTE, imageFresnel);
	glBindTexture(GL_TEXTURE_1D, 0 );

    sgMakeIdentMat4( shadIdentMatrix );

	initDone = true;
}

////////////////////////////////////////////////////////////////////////
// Implementation of SGShaderAnimation
////////////////////////////////////////////////////////////////////////

SGShaderAnimation::SGShaderAnimation ( SGPropertyNode *prop_root,
                   SGPropertyNode_ptr props )
  : SGAnimation(props, new ssgBranch),
    _condition(0),
    _condition_value(true),
    _shader_type(0),
    _param_1(props->getFloatValue("param", 1.0f)),
    _depth_test(props->getBoolValue("depth-test", true)),
    _factor(props->getFloatValue("factor", 1.0f)),
    _factor_prop(0),
    _speed(props->getFloatValue("speed", 1.0f)),
    _speed_prop(0),
    _effectTexture(0),
    _textureData(0),
    _texWidth(0),
    _texHeight(0)

{
    SGPropertyNode_ptr node = props->getChild("condition");
    if (node != 0) {
        _condition = sgReadCondition(prop_root, node);
        _condition_value = false;
    }
    node = props->getChild("factor-prop");
    if( node )
        _factor_prop = prop_root->getNode(node->getStringValue(), true);
    node = props->getChild("speed-prop");
    if( node )
        _speed_prop = prop_root->getNode(node->getStringValue(), true);

    sgSetVec4(_envColor, 0.0f, 0.0f, 0.0f, 1.0f);
    node = props->getChild("texture");
    if( node ) {
        _effectTexture = ssgGetCurrentOptions()->createTexture( (char *) node->getStringValue(), 0, 0, 0);
        glBindTexture(GL_TEXTURE_2D, _effectTexture->getHandle() );
        glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &_texWidth);
        glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &_texHeight);

        _textureData = new unsigned char[_texWidth * _texHeight * 3];
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, _textureData);
        glBindTexture(GL_TEXTURE_2D, 0 );
    }
    string shader_name = props->getStringValue("shader");
    if( shader_name == "fresnel" || shader_name == "reflection" )
        _shader_type = 1;
    else if( shader_name == "heat-haze" )
        _shader_type = 2;
    else if( shader_name == "chrome" && _effectTexture)
        _shader_type = 3;
}

static void setCallBack(ssgBranch *branch, ssgBase *user_data, ssgCallback cb) {
    for (int i = 0; i < branch->getNumKids(); i++) {
        ssgEntity *e = branch->getKid(i);
        if( e->isAKindOf( ssgTypeBranch() ) )
            setCallBack( (ssgBranch *) e, user_data, cb);
        else if( e->isAKindOf( ssgTypeVtxTable() ) ) {
    		e->setCallback( SSG_CALLBACK_PREDRAW, cb );
            e->setUserData( user_data );
        }
    }
}

void SGShaderAnimation::init()
{
    if( ! initDone )
        init_shaders();
    if( _shader_type == 1 && Shader::is_VP_supported() && shFresnel)
        setCallBack( getBranch(), (ssgBase *) this, fresnel_shader_callback );
    else if( _shader_type == 2 ) {
        // this is the same extension with different names
        isRectangleTextureSupported = SGIsOpenGLExtensionSupported("GL_EXT_texture_rectangle") ||
            SGIsOpenGLExtensionSupported("GL_ARB_texture_rectangle") ||
            SGIsOpenGLExtensionSupported("GL_NV_texture_rectangle");
        setCallBack( getBranch(), (ssgBase *) this, heat_haze_shader_callback );
    }
    else if( _shader_type == 3 )
        setCallBack( getBranch(), (ssgBase *) this, chrome_shader_callback );
    else
        setCallBack( getBranch(), (ssgBase *) this, null_shader_callback );
}

SGShaderAnimation::~SGShaderAnimation()
{
    delete _condition;
    delete _effectTexture;
    delete _textureData;
}

int
SGShaderAnimation::update()
{
    if (_condition)
        _condition_value = _condition->test();
    if( _factor_prop)
        _factor = _factor_prop->getFloatValue();
    if( _speed_prop)
        _speed = _speed_prop->getFloatValue();
    return 2;
}

void sgShaderFrameInit(double delta_time_sec) {
    haveBackground = false;
    totalTime += delta_time_sec;
}
