/*
 * Copyright (c) 2007 Ivan Leben
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

#define VG_API_EXPORT
#include "vg/openvg.h"
#include "shDefs.h"
#include "shContext.h"
#include "shExtensions.h"
#include "shGeometry.h"
#include "shImage.h"
#include "shPaint.h"
#include "shPath.h"

// In FlightGear we use our own transformation matrices given by OSG, so disable
// the default model view matrix used by ShaderVG.
#define USE_MODELVIEW_MATRIX 0

void shPremultiplyFramebuffer()
{
    /* Multiply target color with its own alpha */
    glBlendFunc(GL_ZERO, GL_DST_ALPHA);
}

void shUnpremultiplyFramebuffer()
{
    /* TODO: hmmmm..... any idea? */
}

void updateBlendingStateGL(VGContext* c, int alphaIsOne)
{
    /* Most common drawing mode (SRC_OVER with alpha=1)
     as well as SRC is optimized by turning OpenGL
     blending off. In other cases its turned on. */

    switch (c->blendMode) {
    case VG_BLEND_SRC:
        glBlendFunc(GL_ONE, GL_ZERO);
        glDisable(GL_BLEND);
        break;

    case VG_BLEND_SRC_IN:
        glBlendFunc(GL_DST_ALPHA, GL_ZERO);
        glEnable(GL_BLEND);
        break;

    case VG_BLEND_DST_IN:
        glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
        glEnable(GL_BLEND);
        break;

    case VG_BLEND_SRC_OUT_SH:
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ZERO);
        glEnable(GL_BLEND);
        break;

    case VG_BLEND_DST_OUT_SH:
        glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        break;

    case VG_BLEND_SRC_ATOP_SH:
        glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        break;

    case VG_BLEND_DST_ATOP_SH:
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA);
        glEnable(GL_BLEND);
        break;

    case VG_BLEND_DST_OVER:
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
        glEnable(GL_BLEND);
        break;

    case VG_BLEND_SRC_OVER:
    default:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        if (alphaIsOne) {
            glDisable(GL_BLEND);
        } else {
            glEnable(GL_BLEND);
        }
        break;
    };
}

/*-----------------------------------------------------------
 * Draws the triangles representing the stroke of a path.
 *-----------------------------------------------------------*/

static void shDrawStroke(SHPath* p)
{
    VG_GETCONTEXT(VG_NO_RETVAL);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SHVector2) * p->stroke.size, p->stroke.items, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(context->locationDraw.pos, 2, GL_FLOAT, GL_FALSE, sizeof(SHVector2), NULL);
    glEnableVertexAttribArray(context->locationDraw.pos);
    glDrawArrays(GL_TRIANGLES, 0, p->stroke.size);
    glDisableVertexAttribArray(context->locationDraw.pos);
    GL_CHECK_ERROR;
}

/*-----------------------------------------------------------
 * Draws the subdivided vertices in the OpenGL mode given
 * (this could be VG_TRIANGLE_FAN or VG_LINE_STRIP).
 *-----------------------------------------------------------*/

static void shDrawVertices(SHPath* p, GLenum mode)
{
    int start = 0;
    int size = 0;

    /* We separate vertex arrays by contours to properly
     handle the fill modes */
    VG_GETCONTEXT(VG_NO_RETVAL);

    glBufferData(GL_ARRAY_BUFFER, sizeof(SHVertex) * p->vertices.size, p->vertices.items, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(context->locationDraw.pos, 2, GL_FLOAT, GL_FALSE, sizeof(SHVertex), NULL);
    glEnableVertexAttribArray(context->locationDraw.pos);

    while (start < p->vertices.size) {
        size = p->vertices.items[start].flags;
        glDrawArrays(mode, start, size);
        start += size;
    }

    glDisableVertexAttribArray(context->locationDraw.pos);
    GL_CHECK_ERROR;
}

/*--------------------------------------------------------------
 * Constructs & draws colored OpenGL primitives that cover the
 * given bounding box to represent the currently selected
 * stroke or fill paint
 *--------------------------------------------------------------*/

static void shDrawPaintMesh(VGContext* c, SHVector2* min, SHVector2* max,
                            VGPaintMode mode, GLenum texUnit)
{
    SHPaint* p = 0;
    SHVector2 pmin, pmax;
    SHfloat K = 1.0f;

    /* Pick the right paint */
    if (mode == VG_FILL_PATH) {
        p = (c->fillPaint ? c->fillPaint : &c->defaultPaint);
    } else if (mode == VG_STROKE_PATH) {
        p = (c->strokePaint ? c->strokePaint : &c->defaultPaint);
        K = SH_CEIL(c->strokeMiterLimit * c->strokeLineWidth) + 1.0f;
    }

    /* We want to be sure to cover every pixel of this path so better
     take a pixel more than leave some out (multi-sampling is tricky). */
    SET2V(pmin, (*min));
    SUB2(pmin, K, K);
    SET2V(pmax, (*max));
    ADD2(pmax, K, K);

    /* Construct appropriate OpenGL primitives so as
     to fill the stencil mask with select paint */

    switch (p->type) {
    case VG_PAINT_TYPE_LINEAR_GRADIENT:
        shLoadLinearGradientMesh(p, mode, VG_MATRIX_PATH_USER_TO_SURFACE);
        break;

    case VG_PAINT_TYPE_RADIAL_GRADIENT:
        shLoadRadialGradientMesh(p, mode, VG_MATRIX_PATH_USER_TO_SURFACE);
        break;

    case VG_PAINT_TYPE_PATTERN:
        if (p->pattern != VG_INVALID_HANDLE) {
            shLoadPatternMesh(p, mode, VG_MATRIX_PATH_USER_TO_SURFACE);
            break;
        } /* else behave as a color paint */

    case VG_PAINT_TYPE_COLOR:
        shLoadOneColorMesh(p);
        break;
    }

    GLfloat v[] = {pmin.x, pmin.y,
                   pmax.x, pmin.y,
                   pmin.x, pmax.y,
                   pmax.x, pmax.y};

    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(c->locationDraw.pos, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);
    glEnableVertexAttribArray(c->locationDraw.pos);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(c->locationDraw.pos);
    GL_CHECK_ERROR;
}

VGboolean shIsTessCacheValid(VGContext* c, SHPath* p)
{
    SHfloat nX, nY;
    SHVector2 X, Y;
    SHMatrix3x3 mi; //, mchange;
    VGboolean valid = VG_TRUE;

    if (p->cacheDataValid == VG_FALSE) {
        valid = VG_FALSE;
    } else if (p->cacheTransformInit == VG_FALSE) {
        valid = VG_FALSE;
    } else if (shInvertMatrix(&p->cacheTransform, &mi) == VG_FALSE) {
        valid = VG_FALSE;
    } else {
        /* TODO: Compare change matrix for any scale or shear  */
        //  MULMATMAT( c->pathTransform, mi, mchange );
        SET2(X, mi.m[0][0], mi.m[1][0]);
        SET2(Y, mi.m[0][1], mi.m[1][1]);
        nX = NORM2(X);
        nY = NORM2(Y);
        if (nX > 1.01f || nX < 0.99 ||
            nY > 1.01f || nY < 0.99)
            valid = VG_FALSE;
    }

    if (valid == VG_FALSE) {
        /* Update cache */
        p->cacheDataValid = VG_TRUE;
        p->cacheTransformInit = VG_TRUE;
        p->cacheTransform = c->pathTransform;
        p->cacheStrokeTessValid = VG_FALSE;
    }

    return valid;
}

VGboolean shIsStrokeCacheValid(VGContext* c, SHPath* p)
{
    VGboolean valid = VG_TRUE;

    if (p->cacheStrokeInit == VG_FALSE) {
        valid = VG_FALSE;
    } else if (p->cacheStrokeTessValid == VG_FALSE) {
        valid = VG_FALSE;
    } else if (c->strokeDashPattern.size > 0) {
        valid = VG_FALSE;
    } else if (p->cacheStrokeLineWidth != c->strokeLineWidth ||
               p->cacheStrokeCapStyle != c->strokeCapStyle ||
               p->cacheStrokeJoinStyle != c->strokeJoinStyle ||
               p->cacheStrokeMiterLimit != c->strokeMiterLimit) {
        valid = VG_FALSE;
    }

    if (valid == VG_FALSE) {
        /* Update cache */
        p->cacheStrokeInit = VG_TRUE;
        p->cacheStrokeTessValid = VG_TRUE;
        p->cacheStrokeLineWidth = c->strokeLineWidth;
        p->cacheStrokeCapStyle = c->strokeCapStyle;
        p->cacheStrokeJoinStyle = c->strokeJoinStyle;
        p->cacheStrokeMiterLimit = c->strokeMiterLimit;
    }

    return valid;
}

/*-----------------------------------------------------------
 * Tessellates / strokes the path and draws it according to
 * VGContext state.
 *-----------------------------------------------------------*/

VG_API_CALL void vgDrawPath(VGPath path, VGbitfield paintModes)
{
    SHPath* p;
    SHMatrix3x3 mi;
#if USE_MODELVIEW_MATRIX
    SHfloat mgl[16];
#endif
    SHPaint *fill, *stroke;
    SHRectangle* rect;

    VG_GETCONTEXT(VG_NO_RETVAL);

    VG_RETURN_ERR_IF(!shIsValidPath(context, path),
                     VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

    VG_RETURN_ERR_IF(paintModes & (~(VG_STROKE_PATH | VG_FILL_PATH)),
                     VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

    /* FlightGear: we need to bind a VAO before we are allowed to draw when
       using the core profile. */
    glBindVertexArray(context->vao);
    /* Also bind the VBO because we are reusing it */
    glBindBuffer(GL_ARRAY_BUFFER, context->vbo);

    /* Check whether scissoring is enabled and scissor
       rectangle is valid */
    if (context->scissoring == VG_TRUE) {
        rect = &context->scissor.items[0];
        if (context->scissor.size == 0) VG_RETURN(VG_NO_RETVAL);
        if (rect->w <= 0.0f || rect->h <= 0.0f) VG_RETURN(VG_NO_RETVAL);
        glScissor((GLint)rect->x, (GLint)rect->y, (GLint)rect->w, (GLint)rect->h);
        glEnable(GL_SCISSOR_TEST);
    }

    p = (SHPath*)path;

    /* If user-to-surface matrix invertible tessellate in
       surface space for better path resolution */
    if (shIsTessCacheValid(context, p) == VG_FALSE) {
        if (shInvertMatrix(&context->pathTransform, &mi)) {
            shFlattenPath(p, 1);
            shTransformVertices(&mi, p);
        } else
            shFlattenPath(p, 0);
        shFindBoundbox(p);
    }

    /* Pick paint if available or default*/
    fill = (context->fillPaint ? context->fillPaint : &context->defaultPaint);
    stroke = (context->strokePaint ? context->strokePaint : &context->defaultPaint);

    /* Apply transformation */
    glUseProgram(context->progDraw);
#if USE_MODELVIEW_MATRIX
    shMatrixToGL(&context->pathTransform, mgl);
    glUniformMatrix4fv(context->locationDraw.model, 1, GL_FALSE, mgl);
#endif
    /* FlightGear: Get the model view projection matrix written by OSG */
    glUniformMatrix4fv(context->locationDraw.mvp, 1, GL_FALSE, context->mvpMatrix);
    glUniform1i(context->locationDraw.drawMode, 0); /* drawMode: path */
    GL_CHECK_ERROR;

    if (paintModes & VG_FILL_PATH) {
        /* Tesselate into stencil */
        glEnable(GL_STENCIL_TEST);
        if (context->fillRule == VG_EVEN_ODD) {
            glStencilFunc(GL_ALWAYS, 0, 0);
            glStencilOp(GL_INVERT, GL_INVERT, GL_INVERT);
        } else {
            // pseudo non-zero fill rule. Fill everything at least covered once, don't
            // care for possible decrements.
            // TODO implement real non-zero fill-rule
            glStencilFunc(GL_ALWAYS, 1, 1);
            glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        }
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        shDrawVertices(p, GL_TRIANGLE_FAN);

        /* Setup blending */
        updateBlendingStateGL(context,
                              fill->type == VG_PAINT_TYPE_COLOR &&
                              fill->color.a == 1.0f);

        /* Draw paint where stencil odd */
        glStencilFunc(GL_EQUAL, 1, 1);
        glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        shDrawPaintMesh(context, &p->min, &p->max, VG_FILL_PATH, GL_TEXTURE0);

        /* Reset state */
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_STENCIL_TEST);
        // glDisable(GL_BLEND);
    }

    if ((paintModes & VG_STROKE_PATH) &&
        context->strokeLineWidth > 0.0f) {

#if 0
        if (1) {/*context->strokeLineWidth > 1.0f) {*/
#endif
            if (shIsStrokeCacheValid(context, p) == VG_FALSE) {
                /* Generate stroke triangles in user space */
                shVector2ArrayClear(&p->stroke);
                shStrokePath(context, p);
            }

            /* Stroke into stencil */
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_NOTEQUAL, 1, 1);
            glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            shDrawStroke(p);

            /* Setup blending */
            updateBlendingStateGL(context,
                                  stroke->type == VG_PAINT_TYPE_COLOR &&
                                  stroke->color.a == 1.0f);

            /* Draw paint where stencil odd */
            glStencilFunc(GL_EQUAL, 1, 1);
            glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            shDrawPaintMesh(context, &p->min, &p->max, VG_STROKE_PATH, GL_TEXTURE0);

            /* Reset state */
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDisable(GL_STENCIL_TEST);
            // glDisable(GL_BLEND);
#if 0
        } else {
            /* Simulate thin stroke by alpha */
            SHColor c = stroke->color;
            if (context->strokeLineWidth < 1.0f)
                c.a *= context->strokeLineWidth;
      
            /* Draw contour as a line */
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            shDrawVertices(p, GL_LINE_STRIP);
            glDisable(GL_BLEND);
        }
#endif
    }

    if (context->scissoring == VG_TRUE)
        glDisable(GL_SCISSOR_TEST);

    /* Unbind the VBO and VAO. The VAO cannot actually be unbound, but we just
       set it to 0 and let OSG do its thing later. */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void vgDrawImage(VGImage image)
{
// FlightGear: We don't use OpenVG's image drawing stuff
// NOTE: If we ever end up using it, this function has not been adapted to use
// VAOs and VBOs, so some extra work is required.
#if 0

    SHImage* i;
#if USE_MODELVIEW_MATRIX
    SHfloat mgl[16];
#endif
    SHPaint* fill;
    //SHVector2 min, max;
    SHRectangle* rect;

    VG_GETCONTEXT(VG_NO_RETVAL);

    VG_RETURN_ERR_IF(!shIsValidImage(context, image),
                     VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

    /* TODO: check if image is current render target */

    /* Check whether scissoring is enabled and scissor
     rectangle is valid */
    if (context->scissoring == VG_TRUE) {
        rect = &context->scissor.items[0];
        if (context->scissor.size == 0) VG_RETURN(VG_NO_RETVAL);
        if (rect->w <= 0.0f || rect->h <= 0.0f) VG_RETURN(VG_NO_RETVAL);
        glScissor((GLint)rect->x, (GLint)rect->y, (GLint)rect->w, (GLint)rect->h);
        glEnable(GL_SCISSOR_TEST);
    }

    /* Apply image-user-to-surface transformation */
    i = (SHImage*)image;

    glUseProgram(context->progDraw);
#if USE_MODELVIEW_MATRIX
    shMatrixToGL(&context->imageTransform, mgl);
    glUniformMatrix4fv(context->locationDraw.model, 1, GL_FALSE, mgl);
    GL_CHECK_ERROR;
#endif
    glUniform1i(context->locationDraw.drawMode, 1); /* drawMode: image */
    GL_CHECK_ERROR;

    /* Clamp to edge for proper filtering, modulate for multiply mode */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, i->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* Adjust antialiasing to settings */
    if (context->imageQuality == VG_IMAGE_QUALITY_NONANTIALIASED) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glEnableVertexAttribArray(context->locationDraw.textureUV);
    GLfloat uv[] = {0.0f, 0.0f,
                    1.0f, 0.0f,
                    0.0f, 1.0f,
                    1.0f, 1.0f};
    glVertexAttribPointer(context->locationDraw.textureUV, 2, GL_FLOAT, GL_FALSE, 0, uv);
    glUniform1i(context->locationDraw.imageSampler, 0);
    GL_CHECK_ERROR;

    /* Pick fill paint */
    fill = (context->fillPaint ? context->fillPaint : &context->defaultPaint);

    /* Setup blending */
    updateBlendingStateGL(context, 0);

    /* Draw textured quad */
    glEnable(GL_TEXTURE_2D);

    if (context->imageMode == VG_DRAW_IMAGE_MULTIPLY) {
        /* Multiply each colors */
        glUniform1i(context->locationDraw.imageMode, VG_DRAW_IMAGE_MULTIPLY);
        switch (fill->type) {
        case VG_PAINT_TYPE_RADIAL_GRADIENT:
            shLoadRadialGradientMesh(fill, VG_FILL_PATH, VG_MATRIX_IMAGE_USER_TO_SURFACE);
            break;
        case VG_PAINT_TYPE_LINEAR_GRADIENT:
            shLoadLinearGradientMesh(fill, VG_FILL_PATH, VG_MATRIX_IMAGE_USER_TO_SURFACE);
            break;
        case VG_PAINT_TYPE_PATTERN:
            shLoadPatternMesh(fill, VG_FILL_PATH, VG_MATRIX_IMAGE_USER_TO_SURFACE);
            break;
        default:
        case VG_PAINT_TYPE_COLOR:
            shLoadOneColorMesh(fill);
            break;
        }
    } else {
        glUniform1i(context->locationDraw.imageMode, VG_DRAW_IMAGE_NORMAL);
    }

    GLfloat v[] = {0.0f, 0.0f,
                   i->width, 0.0f,
                   0.0f, i->height,
                   i->width, i->height};
    glVertexAttribPointer(context->locationDraw.pos, 2, GL_FLOAT, GL_FALSE, 0, v);
    glEnableVertexAttribArray(context->locationDraw.pos);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(context->locationDraw.pos);

    glDisable(GL_TEXTURE_2D);
    GL_CHECK_ERROR;

    glDisableVertexAttribArray(context->locationDraw.textureUV);

    if (context->scissoring == VG_TRUE)
        glDisable(GL_SCISSOR_TEST);

    VG_RETURN(VG_NO_RETVAL);

#endif
}
