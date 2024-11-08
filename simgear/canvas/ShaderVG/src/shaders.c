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

#include <stdio.h>
#include <string.h>

#include "vg/openvg.h"
#include "shDefs.h"
#include "shContext.h"
#include "shaders.h"

static const char* vgShaderVertexPipeline = "pipeline.vert";
static const char* vgShaderFragmentPipeline = "pipeline.frag";

static const char* vgShaderVertexColorRamp = "color_ramp.vert";
static const char* vgShaderFragmentColorRamp = "color_ramp.frag";

// Defined in simgear/scene/util/LoadShader.hxx
void *sgShaderVGShaderOpen(const char *filename, const char **buf, int *size);
void sgShaderVGShaderClose(void *ptr);

void shInitPiplelineShaders(void)
{
    VG_GETCONTEXT(VG_NO_RETVAL);

    const char* buf;
    int size;
    void *shader;

    context->vs = glCreateShader(GL_VERTEX_SHADER);
    shader = sgShaderVGShaderOpen(vgShaderVertexPipeline, &buf, &size);
    if (shader) {
        glShaderSource(context->vs, 1, &buf, &size);
        glCompileShader(context->vs);
        GL_CHECK_SHADER(context->vs, vgShaderVertexPipeline);
        sgShaderVGShaderClose(shader);
    }

    context->fs = glCreateShader(GL_FRAGMENT_SHADER);
    shader = sgShaderVGShaderOpen(vgShaderFragmentPipeline, &buf, &size);
    if (shader) {
        glShaderSource(context->fs, 1, &buf, &size);
        glCompileShader(context->fs);
        GL_CHECK_SHADER(context->fs, vgShaderFragmentPipeline);
        sgShaderVGShaderClose(shader);
    }

    context->progDraw = glCreateProgram();
    glAttachShader(context->progDraw, context->vs);
    glAttachShader(context->progDraw, context->fs);
    glLinkProgram(context->progDraw);
    GL_CHECK_ERROR;

    context->locationDraw.pos = glGetAttribLocation(context->progDraw, "pos");
    context->locationDraw.textureUV = glGetAttribLocation(context->progDraw, "textureUV");
    // In FlightGear we use our own transformation matrices given by OSG
    context->locationDraw.mvp = glGetUniformLocation(context->progDraw, "sh_Mvp");
    // context->locationDraw.model = glGetUniformLocation(context->progDraw, "sh_Model");
    // context->locationDraw.projection = glGetUniformLocation(context->progDraw, "sh_Ortho");
    context->locationDraw.paintInverted = glGetUniformLocation(context->progDraw, "paintInverted");
    context->locationDraw.drawMode = glGetUniformLocation(context->progDraw, "drawMode");
    context->locationDraw.imageSampler = glGetUniformLocation(context->progDraw, "imageSampler");
    context->locationDraw.imageMode = glGetUniformLocation(context->progDraw, "imageMode");
    context->locationDraw.paintType = glGetUniformLocation(context->progDraw, "paintType");
    context->locationDraw.rampSampler = glGetUniformLocation(context->progDraw, "rampSampler");
    context->locationDraw.patternSampler = glGetUniformLocation(context->progDraw, "patternSampler");
    context->locationDraw.paintParams = glGetUniformLocation(context->progDraw, "paintParams");
    context->locationDraw.paintColor = glGetUniformLocation(context->progDraw, "paintColor");
    context->locationDraw.scaleFactorBias = glGetUniformLocation(context->progDraw, "scaleFactorBias");
    GL_CHECK_ERROR;

    // TODO: Support color transform to remove this from here
    glUseProgram(context->progDraw);
    GLfloat factor_bias[8] = {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0};
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

void shDeinitPiplelineShaders(void)
{
    VG_GETCONTEXT(VG_NO_RETVAL);
    glDeleteShader(context->vs);
    glDeleteShader(context->fs);
    glDeleteProgram(context->progDraw);
    GL_CHECK_ERROR;
}

void shInitRampShaders(void)
{
    VG_GETCONTEXT(VG_NO_RETVAL);

    const char* buf;
    int size;
    void *shader;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    shader = sgShaderVGShaderOpen(vgShaderVertexColorRamp, &buf, &size);
    if (shader) {
        glShaderSource(vs, 1, &buf, &size);
        glCompileShader(vs);
        GL_CHECK_SHADER(vs, vgShaderVertexColorRamp);
        sgShaderVGShaderClose(shader);
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    shader = sgShaderVGShaderOpen(vgShaderFragmentColorRamp, &buf, &size);
    if (shader) {
        glShaderSource(fs, 1, &buf, &size);
        glCompileShader(fs);
        GL_CHECK_SHADER(fs, vgShaderFragmentColorRamp);
        sgShaderVGShaderClose(shader);
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

void shDeinitRampShaders(void)
{
    VG_GETCONTEXT(VG_NO_RETVAL);
    glDeleteProgram(context->progColorRamp);
}

VG_API_CALL void vgShaderSourceSH(VGuint shadertype, const VGbyte* string)
{
    VG_GETCONTEXT(VG_NO_RETVAL);

    switch (shadertype) {
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

VG_API_CALL void vgCompileShaderSH(void)
{
    shDeinitPiplelineShaders();
    shInitPiplelineShaders();
}

VG_API_CALL void vgUniform1fSH(VGint location, VGfloat v0)
{
    glUniform1f(location, v0);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform2fSH(VGint location, VGfloat v0, VGfloat v1)
{
    glUniform2f(location, v0, v1);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform3fSH(VGint location, VGfloat v0, VGfloat v1, VGfloat v2)
{
    glUniform3f(location, v0, v1, v2);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform4fSH(VGint location, VGfloat v0, VGfloat v1, VGfloat v2, VGfloat v3)
{
    glUniform4f(location, v0, v1, v2, v3);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform1fvSH(VGint location, VGint count, const VGfloat* value)
{
    glUniform1fv(location, count, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform2fvSH(VGint location, VGint count, const VGfloat* value)
{
    glUniform2fv(location, count, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform3fvSH(VGint location, VGint count, const VGfloat* value)
{
    glUniform3fv(location, count, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform4fvSH(VGint location, VGint count, const VGfloat* value)
{
    glUniform4fv(location, count, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniformMatrix2fvSH(VGint location, VGint count, VGboolean transpose, const VGfloat* value)
{
    glUniformMatrix2fv(location, count, transpose, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniformMatrix3fvSH(VGint location, VGint count, VGboolean transpose, const VGfloat* value)
{
    glUniformMatrix3fv(location, count, transpose, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniformMatrix4fvSH(VGint location, VGint count, VGboolean transpose, const VGfloat* value)
{
    glUniformMatrix4fv(location, count, transpose, value);
    GL_CHECK_ERROR;
}

VG_API_CALL VGint vgGetUniformLocationSH(const VGbyte* name)
{
    VG_GETCONTEXT(-1);
    VGint retval = glGetUniformLocation(context->progDraw, name);
    GL_CHECK_ERROR;
    return retval;
}

VG_API_CALL void vgGetUniformfvSH(VGint location, VGfloat* params)
{
    VG_GETCONTEXT(VG_NO_RETVAL);
    glGetUniformfv(context->progDraw, location, params);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform1iSH(VGint location, VGint v0)
{
    glUniform1i(location, v0);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform2iSH(VGint location, VGint v0, VGint v1)
{
    glUniform2i(location, v0, v1);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform3iSH(VGint location, VGint v0, VGint v1, VGint v2)
{
    glUniform3i(location, v0, v1, v2);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform4iSH(VGint location, VGint v0, VGint v1, VGint v2, VGint v3)
{
    glUniform4i(location, v0, v1, v2, v3);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform1ivSH(VGint location, VGint count, const VGint* value)
{
    glUniform1iv(location, count, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform2ivSH(VGint location, VGint count, const VGint* value)
{
    glUniform2iv(location, count, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform3ivSH(VGint location, VGint count, const VGint* value)
{
    glUniform3iv(location, count, value);
    GL_CHECK_ERROR;
}

VG_API_CALL void vgUniform4ivSH(VGint location, VGint count, const VGint* value)
{
    glUniform4iv(location, count, value);
    GL_CHECK_ERROR;
}
