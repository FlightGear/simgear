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


#if defined(__cplusplus)
}
#endif

#endif // !__SG_EXTENSIONS_HXX

