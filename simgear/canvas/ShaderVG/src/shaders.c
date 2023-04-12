/*
 * Copyright (c) 2021 Takuma Hayashi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library in the file COPYING;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "vg/openvg.h"
#include "shContext.h"
#include "shDefs.h"
#include "shaders.h"
#include <string.h>
#include <stdio.h>

// The shaders are moved to fgdata/gui/shaders
#if 1
// Defined in CanvasMgr.cxx
void *simgearShaderOpen(const char*, const char**, int*);
void simgearShaderClose(void*);

static const char* vgShaderVertexPipeline = "canvas_pipeline.vert";
static const char* vgShaderFragmentPipeline = "canvas_pipeline.frag";
// static const char* vgShaderVertexUserDefault = "canvas_user_default.vert";
// static const char* vgShaderFragmentUserDefault = "canvas_user_default.frag";
static const char* vgShaderVertexColorRamp = "canvas_color_ramp.vert";
static const char* vgShaderFragmentColorRamp = "canvas_color_ramp.frag";

#else
static const char* vgShaderVertexPipeline = R"glsl(
    #version 330
    
/*** Input *******************/
    in vec2 pos;
    in vec2 textureUV;
    uniform mat4 sh_Model;
    uniform mat4 sh_Ortho;
    uniform mat3 paintInverted;

/*** Output ******************/
    out vec2 texImageCoord;
    out vec2 paintCoord;

/*** Grobal variables ********************/
    vec4 sh_Vertex;

/*** Functions ****************************************/

    // User defined shader
    void shMain(void);

/*** Main thread  **************************************************/
    void main() {

        /* Stage 3: Transformation */
        sh_Vertex = vec4(pos, 0, 1);

        /* Extended Stage: User defined shader that affects gl_Position */
        shMain();

        /* 2D pos in texture space */
        texImageCoord = textureUV;

        /* 2D pos in paint space (Back to paint space) */
        paintCoord = (paintInverted * vec3(pos, 1)).xy;

    }
)glsl";

static const char* vgShaderVertexUserDefault = R"glsl(
    void shMain() {
        gl_Position = sh_Ortho * sh_Model * sh_Vertex;
     }
)glsl";

static const char* vgShaderFragmentPipeline = R"glsl(

    #version 330

/*** Enum constans ************************************/

    #define PAINT_TYPE_COLOR			0x1B00
    #define PAINT_TYPE_LINEAR_GRADIENT	0x1B01
    #define PAINT_TYPE_RADIAL_GRADIENT	0x1B02
    #define PAINT_TYPE_PATTERN			0x1B03

    #define DRAW_IMAGE_NORMAL 			0x1F00
    #define DRAW_IMAGE_MULTIPLY 		0x1F01

    #define DRAW_MODE_PATH				0
    #define DRAW_MODE_IMAGE				1

/*** Interpolated *************************************/

    in vec2 texImageCoord;
    in vec2 paintCoord;

/*** Input ********************************************/

    // Basic rendering Mode
    uniform int drawMode;
    // Image
    uniform sampler2D imageSampler;
    uniform int imageMode;
    // Paint
    uniform int paintType;
    uniform vec4 paintColor;
    uniform vec2 paintParams[3];
    // Gradient
    uniform sampler2D rampSampler;
    // Pattern
    uniform sampler2D patternSampler;
    // Color transform
    uniform vec4 scaleFactorBias[2];

/*** Output *******************************************/

    //out vec4 fragColor;

/*** Built-in variables for shMain *******************************************/

    vec4 sh_Color;

/*** Functions ****************************************/

    // 9.3.1 Linear Gradients
    float linearGradient(vec2 fragCoord, vec2 p0, vec2 p1){

        float x  = fragCoord.x;
        float y  = fragCoord.y;
        float x0 = p0.x;
        float y0 = p0.y;
        float x1 = p1.x;
        float y1 = p1.y;
        float dx = x1 - x0;
        float dy = y1 - y0;
    
        return
            ( dx * (x - x0) + dy * (y - y0) )
         /  ( dx*dx + dy*dy );
    }

    // 9.3.2 Radial Gradients
    float radialGradient(vec2 fragCoord, vec2 centerCoord, vec2 focalCoord, float r){

        float x   = fragCoord.x;
        float y   = fragCoord.y;
        float cx  = centerCoord.x;
        float cy  = centerCoord.y;
        float fx  = focalCoord.x;
        float fy  = focalCoord.y;
        float dx  = x - fx;
        float dy  = y - fy;
        float dfx = fx - cx;
        float dfy = fy - cy;
    
        return
            ( (dx * dfx + dy * dfy) + sqrt(r*r*(dx*dx + dy*dy) - pow(dx*dfy - dy*dfx, 2.0)) )
         /  ( r*r - (dfx*dfx + dfy*dfy) );
    }

    // User defined shader
    void shMain(void);

/*** Main thread  *************************************/

    void main()
    {
        vec4 col;

        /* Stage 6: Paint Generation */
        switch(paintType){
        case PAINT_TYPE_LINEAR_GRADIENT:
            {
                vec2  x0 = paintParams[0];
                vec2  x1 = paintParams[1];
                float factor = linearGradient(paintCoord, x0, x1);
                col = texture(rampSampler, vec2(factor, 0.5));
            }
            break;
        case PAINT_TYPE_RADIAL_GRADIENT:
            {
                vec2  center = paintParams[0];
                vec2  focal  = paintParams[1];
                float radius = paintParams[2].x;
                float factor = radialGradient(paintCoord, center, focal, radius);
                col = texture(rampSampler, vec2(factor, 0.5));
            }
            break;
        case PAINT_TYPE_PATTERN:
            {
                float width  = paintParams[0].x;
                float height = paintParams[0].y;
                vec2  texCoord = vec2(paintCoord.x / width, paintCoord.y / height);
                col = texture(patternSampler, texCoord);
            }
            break;
        default:
        case PAINT_TYPE_COLOR:
            col = paintColor;
            break;
        }

        /* Stage 7: Image Interpolation */
        if(drawMode == DRAW_MODE_IMAGE) {
            col = texture(imageSampler, texImageCoord)
                      * (imageMode == DRAW_IMAGE_MULTIPLY ? col : vec4(1.0, 1.0, 1.0, 1.0));
        } 

        /* Stage 8: Color Transformation, Blending, and Antialiasing */
        sh_Color = col * scaleFactorBias[0] + scaleFactorBias[1] ;

        /* Extended Stage: User defined shader that affects gl_FragColor */
        shMain();
    }
)glsl";

static const char* vgShaderFragmentUserDefault = R"glsl(
    void shMain(){ gl_FragColor = sh_Color; };
)glsl";

static const char* vgShaderVertexColorRamp = R"glsl(
    #version 330
    
    in  vec2 step;
    in  vec4 stepColor;
    out vec4 interpolateColor;

    void main()
    {
        gl_Position = vec4(step.xy, 0, 1);
        interpolateColor = stepColor;
    }
)glsl";

static const char* vgShaderFragmentColorRamp = R"glsl(
    #version 330

    in  vec4 interpolateColor;
    out vec4 fragColor;

    void main()
    {
        fragColor = interpolateColor;
    }
)glsl";
#endif

void shInitPiplelineShaders(void) {

  VG_GETCONTEXT(VG_NO_RETVAL);
//const char* extendedStage;
  const char* buf[2];
  GLint size[2];

  context->vs = glCreateShader(GL_VERTEX_SHADER);
//if(context->userShaderVertex){
//  extendedStage = (const char*)context->userShaderVertex;
//} else {
//  extendedStage = vgShaderVertexUserDefault;
//}

  {
    int len;
    const char *shader;
    void *sh1 = simgearShaderOpen(vgShaderVertexPipeline, &shader, &len);
    buf[0] = shader;
    size[0] = len;

//  void *sh2 = simgearShaderOpen(extendedStage, &shader, &len);
//  buf[1] = shader;
//  size[1] = len;

    glShaderSource(context->vs, 1, buf, size);
    glCompileShader(context->vs);
    GL_CHECK_SHADER(context->vs, vgShaderVertexPipeline);

//  simgearShaderClose(sh2);
    simgearShaderClose(sh1);
  }

  context->fs = glCreateShader(GL_FRAGMENT_SHADER);
//if(context->userShaderFragment){
//  extendedStage = (const char*)context->userShaderFragment;
//} else {
//  extendedStage = vgShaderFragmentUserDefault;
//}

  {
    int len;
    const char *shader;
    void *sh1 = simgearShaderOpen(vgShaderFragmentPipeline, &shader, &len);
    buf[0] = shader;
    size[0] = len;

//  void *sh2 = simgearShaderOpen(extendedStage, &shader, &len);
//  buf[1] = shader;
//  size[1] = len;

    glShaderSource(context->fs, 1, buf, size);
    glCompileShader(context->fs);
    GL_CHECK_SHADER(context->fs, vgShaderFragmentPipeline);

//  simgearShaderClose(sh2);
    simgearShaderClose(sh1);
  }

  context->progDraw = glCreateProgram();
  glAttachShader(context->progDraw, context->vs);
  glAttachShader(context->progDraw, context->fs);
  glLinkProgram(context->progDraw);
  GL_CHECK_ERROR;

  context->locationDraw.pos            = glGetAttribLocation(context->progDraw,  "pos");
  context->locationDraw.textureUV      = glGetAttribLocation(context->progDraw,  "textureUV");
  context->locationDraw.model          = glGetUniformLocation(context->progDraw, "sh_Model");
  context->locationDraw.projection     = glGetUniformLocation(context->progDraw, "sh_Ortho");
  context->locationDraw.paintInverted  = glGetUniformLocation(context->progDraw, "paintInverted");
  context->locationDraw.drawMode       = glGetUniformLocation(context->progDraw, "drawMode");
  context->locationDraw.imageSampler   = glGetUniformLocation(context->progDraw, "imageSampler");
  context->locationDraw.imageMode      = glGetUniformLocation(context->progDraw, "imageMode");
  context->locationDraw.paintType      = glGetUniformLocation(context->progDraw, "paintType");
  context->locationDraw.rampSampler    = glGetUniformLocation(context->progDraw, "rampSampler");
  context->locationDraw.patternSampler = glGetUniformLocation(context->progDraw, "patternSampler");
  context->locationDraw.paintParams    = glGetUniformLocation(context->progDraw, "paintParams");
  context->locationDraw.paintColor     = glGetUniformLocation(context->progDraw, "paintColor");
  context->locationDraw.scaleFactorBias= glGetUniformLocation(context->progDraw, "scaleFactorBias");
  GL_CHECK_ERROR;

  // TODO: Support color transform to remove this from here
  glUseProgram(context->progDraw);
  GLfloat factor_bias[8] = {1.0,1.0,1.0,1.0,0.0,0.0,0.0,0.0};
  glUniform4fv(context->locationDraw.scaleFactorBias, 2, factor_bias);
  GL_CHECK_ERROR;

#if 0
  /* Initialize uniform variables */
  float mat[16];
  float volume = fmax(context->surfaceWidth, context->surfaceHeight) / 2;
  shCalcOrtho2D(mat, 0, context->surfaceWidth , context->surfaceHeight, 0, -volume, volume);
  glUniformMatrix4fv(context->locationDraw.projection, 1, GL_FALSE, mat);
  GL_CHECK_ERROR;
#endif
}

void shDeinitPiplelineShaders(void){

  VG_GETCONTEXT(VG_NO_RETVAL);
  glDeleteShader(context->vs);
  glDeleteShader(context->fs);
  glDeleteProgram(context->progDraw);
  GL_CHECK_ERROR;
}

void shInitRampShaders(void) {

  VG_GETCONTEXT(VG_NO_RETVAL);
  GLint  compileStatus;

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  {
    int len;
    const char *shader;
    void *sh = simgearShaderOpen(vgShaderVertexColorRamp, &shader, &len);

    glShaderSource(vs, 1, &shader, &len);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compileStatus);
//  printf("Shader compile status :%d line:%d\n", compileStatus, __LINE__);
    GL_CHECK_ERROR;

    simgearShaderClose(sh);
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  {
    int len;
    const char *shader;
    void *sh = simgearShaderOpen(vgShaderFragmentColorRamp, &shader, &len);

    glShaderSource(fs, 1, &shader, &len);
    glCompileShader(fs);
    GL_CHECK_SHADER(fs, vgShaderFragmentColorRamp);

    simgearShaderClose(sh);
  }

  context->progColorRamp = glCreateProgram();
  glAttachShader(context->progColorRamp, vs);
  glAttachShader(context->progColorRamp, fs);
  glLinkProgram(context->progColorRamp);
  glDeleteShader(vs);
  glDeleteShader(fs);
  GL_CHECK_ERROR;

  context->locationColorRamp.step = glGetAttribLocation(context->progColorRamp, "step");
  context->locationColorRamp.stepColor = glGetAttribLocation(context->progColorRamp, "stepColor");
  GL_CHECK_ERROR;
}

void shDeinitRampShaders(void){
  VG_GETCONTEXT(VG_NO_RETVAL);
  glDeleteProgram(context->progColorRamp);
}

VG_API_CALL void vgShaderSourceSH(VGuint shadertype, const VGbyte* string){
    VG_GETCONTEXT(VG_NO_RETVAL);

    switch(shadertype) {
		case VG_FRAGMENT_SHADER_SH:
			context->userShaderFragment = (const void*)string;
			break;
		case VG_VERTEX_SHADER_SH:
			context->userShaderVertex = (const void*)string;
			break;
		default:
			break;
    }
}

VG_API_CALL void vgCompileShaderSH(void){
    shDeinitPiplelineShaders();
    shInitPiplelineShaders();
}

VG_API_CALL void vgUniform1fSH(VGint location, VGfloat v0){
    glUniform1f(location, v0);                                                     
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform2fSH(VGint location, VGfloat v0, VGfloat v1){
    glUniform2f(location, v0, v1);                                         
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform3fSH(VGint location, VGfloat v0, VGfloat v1, VGfloat v2){
    glUniform3f(location, v0, v1, v2);                             
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform4fSH(VGint location, VGfloat v0, VGfloat v1, VGfloat v2, VGfloat v3){
    glUniform4f(location, v0, v1, v2, v3);                 
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform1fvSH(VGint location, VGint count, const VGfloat *value){
    glUniform1fv(location, count, value);                           
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform2fvSH(VGint location, VGint count, const VGfloat *value){
    glUniform2fv(location, count, value);                           
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform3fvSH(VGint location, VGint count, const VGfloat *value){
    glUniform3fv(location, count, value);                           
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform4fvSH(VGint location, VGint count, const VGfloat *value){
    glUniform4fv(location, count, value);                           
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniformMatrix2fvSH(VGint location, VGint count, VGboolean transpose, const VGfloat *value){
    glUniformMatrix2fv(location, count, transpose, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniformMatrix3fvSH(VGint location, VGint count, VGboolean transpose, const VGfloat *value){
    glUniformMatrix3fv(location, count, transpose, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniformMatrix4fvSH(VGint location, VGint count, VGboolean transpose, const VGfloat *value){
    glUniformMatrix4fv(location, count, transpose, value);
    GL_CHECK_ERROR;
}

VG_API_CALL VGint vgGetUniformLocationSH(const VGbyte *name){
    VG_GETCONTEXT(-1);
    VGint retval = glGetUniformLocation(context->progDraw, name);
    GL_CHECK_ERROR;
    return retval;
}

VG_API_CALL void vgGetUniformfvSH(VGint location, VGfloat *params){
    VG_GETCONTEXT(VG_NO_RETVAL);
    glGetUniformfv(context->progDraw, location, params);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform1iSH (VGint location, VGint v0){
    glUniform1i (location, v0);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform2iSH (VGint location, VGint v0, VGint v1){
    glUniform2i (location, v0, v1);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform3iSH (VGint location, VGint v0, VGint v1, VGint v2){
    glUniform3i (location,  v0,  v1, v2);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform4iSH (VGint location, VGint v0, VGint v1, VGint v2, VGint v3){
    glUniform4i (location, v0, v1, v2, v3);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform1ivSH (VGint location, VGint count, const VGint *value){
    glUniform1iv (location, count, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform2ivSH (VGint location, VGint count, const VGint *value){
    glUniform2iv (location, count, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform3ivSH (VGint location, VGint count, const VGint *value){
    glUniform3iv (location, count, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform4ivSH (VGint location, VGint count, const VGint *value){
    glUniform4iv (location, count, value);
    GL_CHECK_ERROR;
}


