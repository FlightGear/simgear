/*
 *
 * Copyright (c) 2001 César Blecua Udías    All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * CESAR BLECUA UDIAS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#ifndef __SG_EXTENSIONS_HXX
#define __SG_EXTENSIONS_HXX 1

#if defined(__CYGWIN__) && !defined(WIN32) /* && !defined(USING_X) */
#define WIN32
#endif

#if defined(WIN32)  /* MINGW and MSC predefine WIN32 */
# include <windows.h>
#endif

#if !defined(WIN32)
# include <dlfcn.h>
#endif

#include <simgear/compiler.h>

#include SG_GL_H


#if defined(__cplusplus)
extern "C" {
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

bool SGSearchExtensionsString(const char *extString, const char *extName);
bool SGIsOpenGLExtensionSupported(char *extName);

#ifdef __APPLE__
  // don't use an inline function for symbol lookup, since it is too big
  void* macosxGetGLProcAddress(const char *func);

#elif !defined( WIN32 )

  void *SGGetGLProcAddress(const char *func);
  
#endif

inline void (*SGLookupFunction(const char *func))()
{
#if defined( WIN32 )
    return (void (*)()) wglGetProcAddress(func);

#elif defined( __APPLE__ )
    return (void (*)()) macosxGetGLProcAddress(func);

#else // UNIX

    return (void (*)()) SGGetGLProcAddress(func);
#endif
}

/*
 * OpenGL 1.2 and 1.3 enumerants
 */

#ifndef GL_VERSION_1_2
#define GL_CLAMP_TO_EDGE					0x812F
#define GL_TEXTURE_WRAP_R					0x8072
#define GL_BLEND_EQUATION					0x8009
#define GL_MIN							0x8007
#define GL_MAX							0x8008
#define GL_FUNC_ADD						0x8006
#define GL_FUNC_SUBTRACT					0x800A
#define GL_FUNC_REVERSE_SUBTRACT				0x800B
#define GL_BLEND_COLOR						0x8005
#define GL_CONSTANT_COLOR					0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR				0x8002
#define GL_CONSTANT_ALPHA					0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA				0x8004
#endif

typedef void (APIENTRY * glBlendEquationProc) (GLenum mode );
typedef void (APIENTRY * glBlendColorProc) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );


/* OpenGL extension declarations */

/*
 * glPointParameterf and glPointParameterfv
 */
#ifndef GL_EXT_point_parameters
#define GL_EXT_point_parameters 1
#define GL_POINT_SIZE_MIN_EXT					0x8126
#define GL_DISTANCE_ATTENUATION_EXT				0x8129
#endif

#ifndef GL_ARB_point_parameters
#define GL_ARB_point_parameters 1
#define GL_POINT_SIZE_MIN_ARB					0x8126
#define GL_DISTANCE_ATTENUATION_ARB				0x8129
#endif

typedef void (APIENTRY * glPointParameterfProc)(GLenum pname, GLfloat param);
typedef void (APIENTRY * glPointParameterfvProc)(GLenum pname, const GLfloat *params);

/*
 * glActiveTextureARB
 */

#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture 1
#define GL_TEXTURE0_ARB						0x84C0
#define GL_TEXTURE1_ARB						0x84C1
#define GL_TEXTURE2_ARB						0x84C2
#define GL_TEXTURE3_ARB						0x84C3
#define GL_TEXTURE4_ARB						0x84C4
#define GL_TEXTURE5_ARB						0x84C5
#define GL_TEXTURE6_ARB						0x84C6
#define GL_TEXTURE7_ARB						0x84C7
#define GL_TEXTURE8_ARB						0x84C8
#define GL_TEXTURE9_ARB						0x84C9
#define GL_TEXTURE10_ARB					0x84CA
#define GL_TEXTURE11_ARB					0x84CB
#define GL_TEXTURE12_ARB					0x84CC
#define GL_TEXTURE13_ARB					0x84CD
#define GL_TEXTURE14_ARB					0x84CE
#define GL_TEXTURE15_ARB					0x84CF
#define GL_TEXTURE16_ARB					0x84D0
#define GL_TEXTURE17_ARB					0x84D1
#define GL_TEXTURE18_ARB					0x84D2
#define GL_TEXTURE19_ARB					0x84D3
#define GL_TEXTURE20_ARB					0x84D4
#define GL_TEXTURE21_ARB					0x84D5
#define GL_TEXTURE22_ARB					0x84D6
#define GL_TEXTURE23_ARB					0x84D7
#define GL_TEXTURE24_ARB					0x84D8
#define GL_TEXTURE25_ARB					0x84D9
#define GL_TEXTURE26_ARB					0x84DA
#define GL_TEXTURE27_ARB					0x84DB
#define GL_TEXTURE28_ARB					0x84DC
#define GL_TEXTURE29_ARB					0x84DD
#define GL_TEXTURE30_ARB					0x84DE
#define GL_TEXTURE31_ARB					0x84DF
#define GL_ACTIVE_TEXTURE_ARB					0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB				0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB				0x84E2
#endif

typedef void (APIENTRY * glActiveTextureProc)(GLenum texture);
typedef void (APIENTRY * glClientActiveTextureProc)(GLenum texture);

/*
 * GL_EXT_separate_specular_color
 */

#ifndef GL_LIGHT_MODEL_COLOR_CONTROL
#define GL_LIGHT_MODEL_COLOR_CONTROL				0x81F8
#define GL_SINGLE_COLOR						0x81F9
#define GL_SEPARATE_SPECULAR_COLOR				0x81FA
#endif

/*
 * GL_ARB_texture_cube_map
 */

#ifndef GL_ARB_texture_cube_map
#define GL_ARB_texture_cube_map 1
#define GL_NORMAL_MAP_ARB					0x8511
#define GL_REFLECTION_MAP_ARB					0x8512
#define GL_TEXTURE_CUBE_MAP_ARB					0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB				0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB			0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB			0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB			0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB			0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB			0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB			0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB				0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB			0x851C
#endif

/*
 * GL_ARB_texture_env_combine
 */

#ifndef GL_ARB_texture_env_combine
#define GL_ARB_texture_env_combine 1
#define GL_COMBINE_ARB						0x8570
#define GL_COMBINE_RGB_ARB					0x8571
#define GL_COMBINE_ALPHA_ARB					0x8572
#define GL_RGB_SCALE_ARB					0x8573
#define GL_ADD_SIGNED_ARB					0x8574
#define GL_INTERPOLATE_ARB					0x8575
#define GL_SUBTRACT_ARB                       0x84E7
#define GL_CONSTANT_ARB						0x8576
#define GL_PRIMARY_COLOR_ARB					0x8577
#define GL_PREVIOUS_ARB						0x8578
#define GL_SOURCE0_RGB_ARB					0x8580
#define GL_SOURCE1_RGB_ARB					0x8581
#define GL_SOURCE2_RGB_ARB					0x8582
#define GL_SOURCE0_ALPHA_ARB					0x8588
#define GL_SOURCE1_ALPHA_ARB					0x8589
#define GL_SOURCE2_ALPHA_ARB					0x858A
#define GL_OPERAND0_RGB_ARB					0x8590
#define GL_OPERAND1_RGB_ARB					0x8591
#define GL_OPERAND2_RGB_ARB					0x8592
#define GL_OPERAND0_ALPHA_ARB					0x8598
#define GL_OPERAND1_ALPHA_ARB					0x8599
#define GL_OPERAND2_ALPHA_ARB					0x859A
#endif

/*
 * GL_ARB_texture_env_dot3
 */

#ifndef GL_ARB_texture_env_dot3
#define GL_ARB_texture_env_dot3 1
#define GL_DOT3_RGB_ARB						0x86AE
#define GL_DOT3_RGBA_ARB					0x86AF
#endif

/*
 * ARB_depth_texture
 */
#ifndef GL_ARB_depth_texture
#define GL_ARB_depth_texture 1
#define GL_DEPTH_COMPONENT16_ARB				0x81A5
#define GL_DEPTH_COMPONENT24_ARB				0x81A6
#define GL_DEPTH_COMPONENT32_ARB				0x81A7
#define GL_TEXTURE_DEPTH_SIZE_ARB				0x884A
#define GL_DEPTH_TEXTURE_MODE_ARB				0x884B
#endif

/*
 * ARB_multisample
 */
#ifndef GL_ARB_multisample
#define GL_ARB_multisample 1
#define GL_MULTISAMPLE_ARB					0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB				0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_ARB				0x809F
#define GL_SAMPLE_COVERAGE_ARB					0x80A0
#define GL_SAMPLE_BUFFERS_ARB					0x80A8
#define GL_SAMPLES_ARB						0x80A9
#define GL_SAMPLE_COVERAGE_VALUE_ARB				0x80AA
#define GL_SAMPLE_COVERAGE_INVERT_ARB				0x80AB
#define GL_MULTISAMPLE_BIT_ARB					0x20000000
#define GL_DOUBLEBUFFER						0x0C32
#define GL_AUX_BUFFERS						0x0C00
#define WGL_SAMPLE_BUFFERS_ARB					0x2041
#define WGL_SAMPLES_ARB						0x2042
#endif

#ifndef GL_SGIS_generate_mipmap
#define GL_SGIS_generate_mipmap 1
#define GL_GENERATE_MIPMAP_SGIS					0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS				0x8192
#endif

/* WGL spcific OpenGL extenstions */
#ifdef WIN32

/*
 * WGL_ARB_extensions_string
 */
#ifndef WGL_ARB_extensions_string
#define WGL_ARB_extensions_string 1
typedef const char * (APIENTRY * wglGetExtensionsStringARBProc) (HDC hDC);
#endif

/*
 * WGL_ARB_pbuffer
 */
#ifndef WGL_ARB_pbuffer
#define WGL_ARB_pbuffer 1
#define WGL_DRAW_TO_PBUFFER_ARB					0x202D
#define WGL_MAX_PBUFFER_PIXELS_ARB				0x202E
#define WGL_MAX_PBUFFER_WIDTH_ARB				0x202F
#define WGL_MAX_PBUFFER_HEIGHT_ARB				0x2030
#define WGL_PBUFFER_LARGEST_ARB					0x2033
#define WGL_PBUFFER_WIDTH_ARB					0x2034
#define WGL_PBUFFER_HEIGHT_ARB					0x2035
#define WGL_PBUFFER_LOST_ARB					0x2036
DECLARE_HANDLE(HPBUFFERARB);
typedef HPBUFFERARB (APIENTRY * wglCreatePbufferARBProc) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
typedef HDC (APIENTRY * wglGetPbufferDCARBProc) (HPBUFFERARB hPbuffer);
typedef int (APIENTRY * wglReleasePbufferDCARBProc) (HPBUFFERARB hPbuffer, HDC hDC);
typedef BOOL (APIENTRY * wglDestroyPbufferARBProc) (HPBUFFERARB hPbuffer);
typedef BOOL (APIENTRY * wglQueryPbufferARBProc) (HPBUFFERARB hPbuffer, int iAttribute, int *piValue);
#endif


/*
 * ARB_pixel_format
 */
#ifndef WGL_ARB_pixel_format
#define WGL_ARB_pixel_format 1
#define WGL_NUMBER_PIXEL_FORMATS_ARB				0x2000
#define WGL_DRAW_TO_WINDOW_ARB					0x2001
#define WGL_DRAW_TO_BITMAP_ARB					0x2002
#define WGL_ACCELERATION_ARB					0x2003
#define WGL_NEED_PALETTE_ARB					0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB				0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB				0x2006
#define WGL_SWAP_METHOD_ARB					0x2007
#define WGL_NUMBER_OVERLAYS_ARB					0x2008
#define WGL_NUMBER_UNDERLAYS_ARB				0x2009
#define WGL_TRANSPARENT_ARB					0x200A
#define WGL_SHARE_DEPTH_ARB					0x200C
#define WGL_SHARE_STENCIL_ARB					0x200D
#define WGL_SHARE_ACCUM_ARB					0x200E
#define WGL_SUPPORT_GDI_ARB					0x200F
#define WGL_SUPPORT_OPENGL_ARB					0x2010
#define WGL_DOUBLE_BUFFER_ARB					0x2011
#define WGL_STEREO_ARB						0x2012
#define WGL_PIXEL_TYPE_ARB					0x2013
#define WGL_COLOR_BITS_ARB					0x2014
#define WGL_RED_BITS_ARB					0x2015
#define WGL_RED_SHIFT_ARB					0x2016
#define WGL_GREEN_BITS_ARB					0x2017
#define WGL_GREEN_SHIFT_ARB					0x2018
#define WGL_BLUE_BITS_ARB					0x2019
#define WGL_BLUE_SHIFT_ARB					0x201A
#define WGL_ALPHA_BITS_ARB					0x201B
#define WGL_ALPHA_SHIFT_ARB					0x201C
#define WGL_ACCUM_BITS_ARB					0x201D
#define WGL_ACCUM_RED_BITS_ARB					0x201E
#define WGL_ACCUM_GREEN_BITS_ARB				0x201F
#define WGL_ACCUM_BLUE_BITS_ARB					0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB				0x2021
#define WGL_DEPTH_BITS_ARB					0x2022
#define WGL_STENCIL_BITS_ARB					0x2023
#define WGL_AUX_BUFFERS_ARB					0x2024
#define WGL_NO_ACCELERATION_ARB					0x2025
#define WGL_GENERIC_ACCELERATION_ARB				0x2026
#define WGL_FULL_ACCELERATION_ARB				0x2027
#define WGL_SWAP_EXCHANGE_ARB					0x2028
#define WGL_SWAP_COPY_ARB					0x2029
#define WGL_SWAP_UNDEFINED_ARB					0x202A
#define WGL_TYPE_RGBA_ARB					0x202B
#define WGL_TYPE_COLORINDEX_ARB					0x202C
#define WGL_TRANSPARENT_RED_VALUE_ARB				0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB				0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB				0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB				0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB				0x203B
typedef BOOL (APIENTRY * wglGetPixelFormatAttribivARBProc) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
typedef BOOL (APIENTRY * wglGetPixelFormatAttribfvARBProc) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
typedef BOOL (APIENTRY * wglChoosePixelFormatARBProc) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
#endif

/*
 * ARB_render_texture
 */
#ifndef WGL_ARB_render_texture
#define WGL_ARB_render_texture 1

#define WGL_BIND_TO_TEXTURE_RGB_ARB				0x2070
#define WGL_BIND_TO_TEXTURE_RGBA_ARB				0x2071
#define WGL_TEXTURE_FORMAT_ARB					0x2072
#define WGL_TEXTURE_TARGET_ARB					0x2073
#define WGL_MIPMAP_TEXTURE_ARB					0x2074
#define WGL_TEXTURE_RGB_ARB					0x2075
#define WGL_TEXTURE_RGBA_ARB					0x2076
#define WGL_NO_TEXTURE_ARB					0x2077
#define WGL_TEXTURE_CUBE_MAP_ARB				0x2078
#define WGL_TEXTURE_1D_ARB					0x2079
#define WGL_TEXTURE_2D_ARB					0x207A
#define WGL_NO_TEXTURE_ARB					0x2077
#define WGL_MIPMAP_LEVEL_ARB					0x207B
#define WGL_CUBE_MAP_FACE_ARB					0x207C
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB			0x207D
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB			0x207E
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB			0x207F
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB			0x2080
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB			0x2081
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB			0x2082
#define WGL_FRONT_LEFT_ARB					0x2083
#define WGL_FRONT_RIGHT_ARB					0x2084
#define WGL_BACK_LEFT_ARB					0x2085
#define WGL_BACK_RIGHT_ARB					0x2086
#define WGL_AUX0_ARB						0x2087
#define WGL_AUX1_ARB						0x2088
#define WGL_AUX2_ARB						0x2089
#define WGL_AUX3_ARB						0x208A
#define WGL_AUX4_ARB						0x208B
#define WGL_AUX5_ARB						0x208C
#define WGL_AUX6_ARB						0x208D
#define WGL_AUX7_ARB						0x208E
#define WGL_AUX8_ARB						0x208F
#define WGL_AUX9_ARB						0x2090
typedef BOOL (APIENTRY * wglBindTexImageARBProc) (HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL (APIENTRY * wglReleaseTexImageARBProc) (HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL (APIENTRY * wglSetPbufferAttribARBProc) (HPBUFFERARB hPbuffer, const int *piAttribList);
#endif

#elif !defined(__APPLE__) /* !WIN32 */

/* GLX pcific OpenGL extenstions */
#include <GL/glx.h>

#ifndef GLX_ARB_multisample
#define GLX_ARB_multisample1
#define GLX_SAMPLE_BUFFERS_ARB					100001
#define GLX_SAMPLES_ARB						100000
#endif

#ifndef GLX_SGIX_pbuffer
#define GLX_SGIX_pbuffer 1
#define GLX_DOUBLEBUFFER					5
#define GLX_AUX_BUFFERS						0x00000010
#endif

#ifndef GLXPbuffer
#define GLXPbuffer GLXPbufferSGIX
#endif
#ifndef GLXFBConfig
#define GLXFBConfig GLXFBConfigSGIX
#endif

typedef GLXFBConfig *(*glXChooseFBConfigProc) (Display *dpy, int screen, int *attribList, int *nitems);
typedef GLXPbuffer (*glXCreateGLXPbufferProc) (Display *dpy, GLXFBConfig config, unsigned int width, unsigned int height, int *attrib_list);
typedef GLXPbuffer (*glXCreatePbufferProc) (Display *dpy, GLXFBConfig config, int *attrib_list);
typedef XVisualInfo *(*glXGetVisualFromFBConfigProc) (Display *dpy, GLXFBConfig config);
typedef GLXContext (*glXCreateContextWithConfigProc) (Display *dpy,  GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);
typedef GLXContext (*glXCreateContextProc) (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
typedef void (*glXDestroyPbufferProc) (Display *dpy, GLXPbuffer pbuf);
typedef int (*glXQueryGLXPbufferSGIXProc) (Display *, GLXPbufferSGIX, int, unsigned int *);
typedef void (*glXQueryDrawableProc) (Display *, GLXDrawable, int, unsigned int *);
#endif /* WIN32 */


/* NVIDIA specific extension */

/*
 * NV_texture_rectangle
 */

#ifndef GL_NV_texture_rectangle
#define GL_NV_texture_rectangle 1
#define GL_TEXTURE_RECTANGLE_NV					0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_NV				0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_NV				0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_NV			0x84F8
#endif

/*
 * WGL_NV_texture_rectangle
 */

#ifndef WGL_NV_texture_rectangle
#define WGL_NV_texture_rectangle 1
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV			0x20A0
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV			0x20A1
#define WGL_TEXTURE_RECTANGLE_NV				0x20A2
#endif

/*
 * NV_render_depth_texture
 */

#ifndef WGL_NV_render_depth_texture
#define WGL_NV_render_depth_texture 1
#define WGL_NO_TEXTURE_ARB					0x2077
#define WGL_BIND_TO_TEXTURE_DEPTH_NV				0x20A3
#define WGL_BIND_TO_TEXTURE_RECTANGLE_DEPTH_NV			0x20A4
#define WGL_DEPTH_TEXTURE_FORMAT_NV				0x20A5
#define WGL_TEXTURE_DEPTH_COMPONENT_NV				0x20A6
#define WGL_DEPTH_COMPONENT_NV					0x20A7
#endif

/*
 * NV_float_buffer
 */
#ifndef GL_NV_float_buffer
#define GL_NV_float_buffer 1
#define GL_FLOAT_R_NV						0x8880
#define GL_FLOAT_RG_NV						0x8881
#define GL_FLOAT_RGB_NV						0x8882
#define GL_FLOAT_RGBA_NV					0x8883
#define GL_FLOAT_R16_NV						0x8884
#define GL_FLOAT_R32_NV						0x8885
#define GL_FLOAT_RG16_NV					0x8886
#define GL_FLOAT_RG32_NV					0x8887
#define GL_FLOAT_RGB16_NV					0x8888
#define GL_FLOAT_RGB32_NV					0x8889
#define GL_FLOAT_RGBA16_NV					0x888A
#define GL_FLOAT_RGBA32_NV					0x888B
#define GL_TEXTURE_FLOAT_COMPONENTS_NV				0x888C
#define GL_FLOAT_CLEAR_COLOR_VALUE_NV				0x888D
#define GL_FLOAT_RGBA_MODE_NV					0x888E
#endif
#ifndef GLX_NV_float_buffer
#define GLX_NV_float_buffer 1
#define GLX_FLOAT_COMPONENTS_NV					0x20B0
#define GLX_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV		0x20B1
#define GLX_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV		0x20B2
#define GLX_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV		0x20B3
#define GLX_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV		0x20B4
#define GLX_TEXTURE_FLOAT_R_NV					0x20B5
#define GLX_TEXTURE_FLOAT_RG_NV					0x20B6
#define GLX_TEXTURE_FLOAT_RGB_NV				0x20B7
#define GLX_TEXTURE_FLOAT_RGBA_NV				0x20B8
#endif
#ifndef WGL_NV_float_buffer
#define WGL_NV_float_buffer 1
#define WGL_FLOAT_COMPONENTS_NV					0x20B0
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV		0x20B1
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV		0x20B2
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV		0x20B3
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV		0x20B4
#define WGL_TEXTURE_FLOAT_R_NV					0x20B5
#define WGL_TEXTURE_FLOAT_RG_NV					0x20B6
#define WGL_TEXTURE_FLOAT_RGB_NV				0x20B7
#define WGL_TEXTURE_FLOAT_RGBA_NV				0x20B8
#endif


/* ATI specific extension */

/*
 * ATI_pixel_format_float
 */
#ifndef WGL_ATI_pixel_format_float
#define WGL_ATI_pixel_format_float 1
#define WGL_TYPE_RGBA_FLOAT_ATI					0x21A0
#define GL_RGBA_FLOAT_MODE_ATI					0x8820
#define GL_COLOR_CLEAR_UNCLAMPED_VALUE_ATI			0x8835
#endif

/*
 * ATI_texture_float
 */
#ifndef GL_ATI_texture_float
#define GL_ATI_texture_float 1
#define GL_RGBA_FLOAT32_ATI					0x8814
#define GL_RGB_FLOAT32_ATI					0x8815
#define GL_ALPHA_FLOAT32_ATI					0x8816
#define GL_INTENSITY_FLOAT32_ATI				0x8817
#define GL_LUMINANCE_FLOAT32_ATI				0x8818
#define GL_LUMINANCE_ALPHA_FLOAT32_ATI				0x8819
#define GL_RGBA_FLOAT16_ATI					0x881A
#define GL_RGB_FLOAT16_ATI					0x881B
#define GL_ALPHA_FLOAT16_ATI					0x881C
#define GL_INTENSITY_FLOAT16_ATI				0x881D
#define GL_LUMINANCE_FLOAT16_ATI				0x881E
#define GL_LUMINANCE_ALPHA_FLOAT16_ATI				0x881F
#endif

/*
 * ARB_vertex_program
 */
#ifndef GL_ARB_vertex_program
#define GL_ARB_vertex_program 1
#define GL_COLOR_SUM_ARB                  0x8458
#define GL_VERTEX_PROGRAM_ARB             0x8620
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB   0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB   0x8625
#define GL_CURRENT_VERTEX_ATTRIB_ARB      0x8626
#define GL_PROGRAM_LENGTH_ARB             0x8627
#define GL_PROGRAM_STRING_ARB             0x8628
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB 0x862E
#define GL_MAX_PROGRAM_MATRICES_ARB       0x862F
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB 0x8640
#define GL_CURRENT_MATRIX_ARB             0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB  0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB    0x8643
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB 0x8645
#define GL_PROGRAM_ERROR_POSITION_ARB     0x864B
#define GL_PROGRAM_BINDING_ARB            0x8677
#define GL_MAX_VERTEX_ATTRIBS_ARB         0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB 0x886A
#define GL_PROGRAM_ERROR_STRING_ARB       0x8874
#define GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
#define GL_PROGRAM_FORMAT_ARB             0x8876
#define GL_PROGRAM_INSTRUCTIONS_ARB       0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB   0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB        0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB    0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A7
#define GL_PROGRAM_PARAMETERS_ARB         0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB     0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB  0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AB
#define GL_PROGRAM_ATTRIBS_ARB            0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB        0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB     0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB  0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB 0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB 0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB   0x88B7
#define GL_MATRIX0_ARB                    0x88C0
#define GL_MATRIX1_ARB                    0x88C1
#define GL_MATRIX2_ARB                    0x88C2
#define GL_MATRIX3_ARB                    0x88C3
#define GL_MATRIX4_ARB                    0x88C4
#define GL_MATRIX5_ARB                    0x88C5
#define GL_MATRIX6_ARB                    0x88C6
#define GL_MATRIX7_ARB                    0x88C7
#define GL_MATRIX8_ARB                    0x88C8
#define GL_MATRIX9_ARB                    0x88C9
#define GL_MATRIX10_ARB                   0x88CA
#define GL_MATRIX11_ARB                   0x88CB
#define GL_MATRIX12_ARB                   0x88CC
#define GL_MATRIX13_ARB                   0x88CD
#define GL_MATRIX14_ARB                   0x88CE
#define GL_MATRIX15_ARB                   0x88CF
#define GL_MATRIX16_ARB                   0x88D0
#define GL_MATRIX17_ARB                   0x88D1
#define GL_MATRIX18_ARB                   0x88D2
#define GL_MATRIX19_ARB                   0x88D3
#define GL_MATRIX20_ARB                   0x88D4
#define GL_MATRIX21_ARB                   0x88D5
#define GL_MATRIX22_ARB                   0x88D6
#define GL_MATRIX23_ARB                   0x88D7
#define GL_MATRIX24_ARB                   0x88D8
#define GL_MATRIX25_ARB                   0x88D9
#define GL_MATRIX26_ARB                   0x88DA
#define GL_MATRIX27_ARB                   0x88DB
#define GL_MATRIX28_ARB                   0x88DC
#define GL_MATRIX29_ARB                   0x88DD
#define GL_MATRIX30_ARB                   0x88DE
#define GL_MATRIX31_ARB                   0x88DF
#endif

/*
 * ARB_fragment_program
 */
#ifndef GL_ARB_fragment_program
#define GL_ARB_fragment_program 1
#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB   0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB   0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB   0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB 0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB 0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB 0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x8810
#define GL_MAX_TEXTURE_COORDS_ARB         0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB    0x8872
#endif

typedef void (APIENTRY * glVertexAttrib1dProc) (GLuint index, GLdouble x);
typedef void (APIENTRY * glVertexAttrib1dvProc) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * glVertexAttrib1fProc) (GLuint index, GLfloat x);
typedef void (APIENTRY * glVertexAttrib1fvProc) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * glVertexAttrib1sProc) (GLuint index, GLshort x);
typedef void (APIENTRY * glVertexAttrib1svProc) (GLuint index, const GLshort *v);
typedef void (APIENTRY * glVertexAttrib2dProc) (GLuint index, GLdouble x, GLdouble y);
typedef void (APIENTRY * glVertexAttrib2dvProc) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * glVertexAttrib2fProc) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRY * glVertexAttrib2fvProc) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * glVertexAttrib2sProc) (GLuint index, GLshort x, GLshort y);
typedef void (APIENTRY * glVertexAttrib2svProc) (GLuint index, const GLshort *v);
typedef void (APIENTRY * glVertexAttrib3dProc) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * glVertexAttrib3dvProc) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * glVertexAttrib3fProc) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * glVertexAttrib3fvProc) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * glVertexAttrib3sProc) (GLuint index, GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * glVertexAttrib3svProc) (GLuint index, const GLshort *v);
typedef void (APIENTRY * glVertexAttrib4NbvProc) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * glVertexAttrib4NivProc) (GLuint index, const GLint *v);
typedef void (APIENTRY * glVertexAttrib4NsvProc) (GLuint index, const GLshort *v);
typedef void (APIENTRY * glVertexAttrib4NubProc) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
typedef void (APIENTRY * glVertexAttrib4NubvProc) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * glVertexAttrib4NuivProc) (GLuint index, const GLuint *v);
typedef void (APIENTRY * glVertexAttrib4NusvProc) (GLuint index, const GLushort *v);
typedef void (APIENTRY * glVertexAttrib4bvProc) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * glVertexAttrib4dProc) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * glVertexAttrib4dvProc) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * glVertexAttrib4fProc) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * glVertexAttrib4fvProc) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * glVertexAttrib4ivProc) (GLuint index, const GLint *v);
typedef void (APIENTRY * glVertexAttrib4sProc) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY * glVertexAttrib4svProc) (GLuint index, const GLshort *v);
typedef void (APIENTRY * glVertexAttrib4ubvProc) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * glVertexAttrib4uivProc) (GLuint index, const GLuint *v);
typedef void (APIENTRY * glVertexAttrib4usvProc) (GLuint index, const GLushort *v);
typedef void (APIENTRY * glVertexAttribPointerProc) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * glEnableVertexAttribArrayProc) (GLuint index);
typedef void (APIENTRY * glDisableVertexAttribArrayProc) (GLuint index);
typedef void (APIENTRY * glProgramStringProc) (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
typedef void (APIENTRY * glBindProgramProc) (GLenum target, GLuint program);
typedef void (APIENTRY * glDeleteProgramsProc) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * glGenProgramsProc) (GLsizei n, GLuint *programs);
typedef void (APIENTRY * glProgramEnvParameter4dProc) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * glProgramEnvParameter4dvProc) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * glProgramEnvParameter4fProc) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * glProgramEnvParameter4fvProc) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * glProgramLocalParameter4dProc) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * glProgramLocalParameter4dvProc) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * glProgramLocalParameter4fProc) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * glProgramLocalParameter4fvProc) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * glGetProgramEnvParameterdvProc) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * glGetProgramEnvParameterfvProc) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * glGetProgramLocalParameterdvProc) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * glGetProgramLocalParameterfvProc) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * glGetProgramivProc) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * glGetProgramStringProc) (GLenum target, GLenum pname, GLvoid *string);
typedef void (APIENTRY * glGetVertexAttribdvProc) (GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRY * glGetVertexAttribfvProc) (GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRY * glGetVertexAttribivProc) (GLuint index, GLenum pname, GLint *params);
typedef void (APIENTRY * glGetVertexAttribPointervProc) (GLuint index, GLenum pname, GLvoid* *pointer);
typedef GLboolean (APIENTRY * glIsProgramProc) (GLuint program);

/*
 * ARB_shader_objects
 */
#ifndef GL_ARB_shader_objects
#define GL_ARB_shader_objects 1
/* GL types for handling shader object handles and program/shader text */
typedef char GLcharARB;		/* native character */
typedef unsigned int GLhandleARB;	/* shader object handle */

#define GL_PROGRAM_OBJECT_ARB             0x8B40
#define GL_SHADER_OBJECT_ARB              0x8B48
#define GL_OBJECT_TYPE_ARB                0x8B4E
#define GL_OBJECT_SUBTYPE_ARB             0x8B4F
#define GL_FLOAT_VEC2_ARB                 0x8B50
#define GL_FLOAT_VEC3_ARB                 0x8B51
#define GL_FLOAT_VEC4_ARB                 0x8B52
#define GL_INT_VEC2_ARB                   0x8B53
#define GL_INT_VEC3_ARB                   0x8B54
#define GL_INT_VEC4_ARB                   0x8B55
#define GL_BOOL_ARB                       0x8B56
#define GL_BOOL_VEC2_ARB                  0x8B57
#define GL_BOOL_VEC3_ARB                  0x8B58
#define GL_BOOL_VEC4_ARB                  0x8B59
#define GL_FLOAT_MAT2_ARB                 0x8B5A
#define GL_FLOAT_MAT3_ARB                 0x8B5B
#define GL_FLOAT_MAT4_ARB                 0x8B5C
#define GL_SAMPLER_1D_ARB                 0x8B5D
#define GL_SAMPLER_2D_ARB                 0x8B5E
#define GL_SAMPLER_3D_ARB                 0x8B5F
#define GL_SAMPLER_CUBE_ARB               0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB          0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB          0x8B62
#define GL_SAMPLER_2D_RECT_ARB            0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB     0x8B64
#define GL_OBJECT_DELETE_STATUS_ARB       0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB      0x8B81
#define GL_OBJECT_LINK_STATUS_ARB         0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB     0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB     0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB    0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB     0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB 0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB 0x8B88
#endif

typedef void (APIENTRY * glDeleteObjectProc) (GLhandleARB obj);
typedef GLhandleARB (APIENTRY * glGetHandleProc) (GLenum pname);
typedef void (APIENTRY * glDetachObjectProc) (GLhandleARB containerObj, GLhandleARB attachedObj);
typedef GLhandleARB (APIENTRY * glCreateShaderObjectProc) (GLenum shaderType);
typedef void (APIENTRY * glShaderSourceProc) (GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length);
typedef void (APIENTRY * glCompileShaderProc) (GLhandleARB shaderObj);
typedef GLhandleARB (APIENTRY * glCreateProgramObjectProc) (void);
typedef void (APIENTRY * glAttachObjectProc) (GLhandleARB containerObj, GLhandleARB obj);
typedef void (APIENTRY * glLinkProgramProc) (GLhandleARB programObj);
typedef void (APIENTRY * glUseProgramObjectProc) (GLhandleARB programObj);
typedef void (APIENTRY * glValidateProgramProc) (GLhandleARB programObj);
typedef void (APIENTRY * glUniform1fProc) (GLint location, GLfloat v0);
typedef void (APIENTRY * glUniform2fProc) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY * glUniform3fProc) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY * glUniform4fProc) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY * glUniform1iProc) (GLint location, GLint v0);
typedef void (APIENTRY * glUniform2iProc) (GLint location, GLint v0, GLint v1);
typedef void (APIENTRY * glUniform3iProc) (GLint location, GLint v0, GLint v1, GLint v2);
typedef void (APIENTRY * glUniform4iProc) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void (APIENTRY * glUniform1fvProc) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * glUniform2fvProc) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * glUniform3fvProc) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * glUniform4fvProc) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * glUniform1ivProc) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * glUniform2ivProc) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * glUniform3ivProc) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * glUniform4ivProc) (GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY * glUniformMatrix2fvProc) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * glUniformMatrix3fvProc) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * glUniformMatrix4fvProc) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * glGetObjectParameterfvProc) (GLhandleARB obj, GLenum pname, GLfloat *params);
typedef void (APIENTRY * glGetObjectParameterivProc) (GLhandleARB obj, GLenum pname, GLint *params);
typedef void (APIENTRY * glGetInfoLogProc) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
typedef void (APIENTRY * glGetAttachedObjectsProc) (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
typedef GLint (APIENTRY * glGetUniformLocationProc) (GLhandleARB programObj, const GLcharARB *name);
typedef void (APIENTRY * glGetActiveUniformProc) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
typedef void (APIENTRY * glGetUniformfvProc) (GLhandleARB programObj, GLint location, GLfloat *params);
typedef void (APIENTRY * glGetUniformivProc) (GLhandleARB programObj, GLint location, GLint *params);
typedef void (APIENTRY * glGetShaderSourceProc) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);

/*
 * ARB_vertex_shader
 */
#ifndef GL_ARB_vertex_shader
#define GL_ARB_vertex_shader 1
#define GL_VERTEX_SHADER_ARB              0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB 0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB         0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB 0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB 0x8B4D
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB   0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB 0x8B8A
#endif

typedef void (APIENTRY * glBindAttribLocationProc) (GLhandleARB programObj, GLuint index, const GLcharARB *name);
typedef void (APIENTRY * glGetActiveAttribProc) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
typedef GLint (APIENTRY * glGetAttribLocationProc) (GLhandleARB programObj, const GLcharARB *name);

/*
 * ARB_fragment_shader
 */
#ifndef GL_ARB_fragment_shader
#define GL_ARB_fragment_shader1 
#define GL_FRAGMENT_SHADER_ARB            0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB 0x8B49
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB 0x8B8B
#endif

/*
 * NV_fragment_program
 */
#ifndef GL_NV_fragment_program
#define GL_NV_fragment_program 1
#define GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV 0x8868
#define GL_FRAGMENT_PROGRAM_NV            0x8870
#define GL_MAX_TEXTURE_COORDS_NV          0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_NV     0x8872
#define GL_FRAGMENT_PROGRAM_BINDING_NV    0x8873
#define GL_PROGRAM_ERROR_STRING_NV        0x8874
#endif
#ifndef GL_NV_vertex_program
#define GL_NV_vertex_program 1
#define GL_VERTEX_PROGRAM_NV              0x8620
#define GL_PROGRAM_ERROR_POSITION_NV      0x864B
#endif

typedef void (APIENTRY * glBindProgramNVProc) (GLenum target, GLuint id);
typedef void (APIENTRY * glDeleteProgramsNVProc) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * glGenProgramsNVProc) (GLsizei n, GLuint *programs);
typedef void (APIENTRY * glLoadProgramNVProc) (GLenum target, GLuint id, GLsizei len, const GLubyte *program);
typedef void (APIENTRY * glProgramParameter4fvNVProc) (GLenum target, GLuint index, const GLfloat *v);

#if defined(__cplusplus)
}
#endif

#endif // !__SG_EXTENSIONS_HXX

