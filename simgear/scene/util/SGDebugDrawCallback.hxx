/* -*-c++-*-
 *
 * Copyright (C) 2006 Mathias Froehlich 
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

#ifndef SG_SCENE_DEBUGDRAWCALLBACK_HXX
#define SG_SCENE_DEBUGDRAWCALLBACK_HXX

#include <iostream>
#include <GL/gl.h>
#include <osg/Drawable>
#include <osg/State>

struct SGDebugDrawCallback : public osg::Drawable::DrawCallback {
  virtual void drawImplementation(osg::State& state,
                                  const osg::Drawable* drawable) const
  {
//     state.dirtyColorPointer();
    printState(std::cout, drawable);
    drawable->drawImplementation(state);
    printState(std::cout, drawable);
//     state.dirtyColorPointer();
  }

  void printState(std::ostream& stream, const osg::Drawable* drawable) const
  {
    stream << "Drawable \"" << drawable->getName() << "\"";
#ifdef ERROR_CHECK
#undef ERROR_CHECK
#endif

#define ERROR_CHECK(msg)              \
do {                                  \
  GLenum errorNo = glGetError();      \
  if (errorNo != GL_NO_ERROR)         \
    stream << msg;                    \
} while(0)

#ifdef PRINT_STATE
#undef PRINT_STATE
#endif
#define PRINT_STATE(flag)             \
do {                                  \
  if (glIsEnabled(flag))              \
    stream << " " #flag;              \
  ERROR_CHECK(" ERROR " #flag);       \
} while(0)

    PRINT_STATE(GL_COLOR_ARRAY);
    PRINT_STATE(GL_EDGE_FLAG_ARRAY);
    PRINT_STATE(GL_INDEX_ARRAY);
    PRINT_STATE(GL_NORMAL_ARRAY);
    PRINT_STATE(GL_TEXTURE_COORD_ARRAY);
    PRINT_STATE(GL_VERTEX_ARRAY);

    PRINT_STATE(GL_ALPHA_TEST);
    PRINT_STATE(GL_AUTO_NORMAL);
    PRINT_STATE(GL_BLEND);
    PRINT_STATE(GL_CLIP_PLANE0);
    PRINT_STATE(GL_CLIP_PLANE1);
    PRINT_STATE(GL_CLIP_PLANE2);
    PRINT_STATE(GL_CLIP_PLANE3);
    PRINT_STATE(GL_CLIP_PLANE4);
    PRINT_STATE(GL_CLIP_PLANE5);
    PRINT_STATE(GL_COLOR_LOGIC_OP);
//     PRINT_STATE(GL_COLOR_TABLE);
//     PRINT_STATE(GL_CONVOLUTION_1D);
//     PRINT_STATE(GL_CONVOLUTION_2D);
    PRINT_STATE(GL_CULL_FACE);
    PRINT_STATE(GL_DEPTH_TEST);
    PRINT_STATE(GL_DITHER);
    PRINT_STATE(GL_FOG);
//     PRINT_STATE(GL_HISTOGRAM);
    PRINT_STATE(GL_INDEX_LOGIC_OP);
    PRINT_STATE(GL_LIGHT0);
    PRINT_STATE(GL_LIGHT1);
    PRINT_STATE(GL_LIGHT2);
    PRINT_STATE(GL_LIGHT3);
    PRINT_STATE(GL_LIGHT4);
    PRINT_STATE(GL_LIGHT5);
    PRINT_STATE(GL_LIGHT6);
    PRINT_STATE(GL_LIGHT7);
    PRINT_STATE(GL_LIGHTING);
    PRINT_STATE(GL_LINE_SMOOTH);
    PRINT_STATE(GL_LINE_STIPPLE);
    PRINT_STATE(GL_MAP1_COLOR_4);
    PRINT_STATE(GL_MAP1_INDEX);
    PRINT_STATE(GL_MAP1_NORMAL);
    PRINT_STATE(GL_MAP1_TEXTURE_COORD_1);
    PRINT_STATE(GL_MAP1_TEXTURE_COORD_2);
    PRINT_STATE(GL_MAP1_TEXTURE_COORD_3);
    PRINT_STATE(GL_MAP1_TEXTURE_COORD_4);
    PRINT_STATE(GL_MAP2_COLOR_4);
    PRINT_STATE(GL_MAP2_INDEX);
    PRINT_STATE(GL_MAP2_NORMAL);
    PRINT_STATE(GL_MAP2_TEXTURE_COORD_1);
    PRINT_STATE(GL_MAP2_TEXTURE_COORD_2);
    PRINT_STATE(GL_MAP2_TEXTURE_COORD_3);
    PRINT_STATE(GL_MAP2_TEXTURE_COORD_4);
    PRINT_STATE(GL_MAP2_VERTEX_3);
    PRINT_STATE(GL_MAP2_VERTEX_4);
//     PRINT_STATE(GL_MINMAX);
    PRINT_STATE(GL_NORMALIZE);
    PRINT_STATE(GL_POINT_SMOOTH);
    PRINT_STATE(GL_POLYGON_SMOOTH);
    PRINT_STATE(GL_POLYGON_OFFSET_FILL);
    PRINT_STATE(GL_POLYGON_OFFSET_LINE);
    PRINT_STATE(GL_POLYGON_OFFSET_POINT);
    PRINT_STATE(GL_POLYGON_STIPPLE);
//     PRINT_STATE(GL_POST_COLOR_MATRIX_COLOR_TABLE);
//     PRINT_STATE(GL_POST_CONVOLUTION_COLOR_TABLE);
    PRINT_STATE(GL_RESCALE_NORMAL);
    PRINT_STATE(GL_SCISSOR_TEST);
//     PRINT_STATE(GL_SEPARABLE_2D);
    PRINT_STATE(GL_STENCIL_TEST);
    PRINT_STATE(GL_TEXTURE_1D);
    PRINT_STATE(GL_TEXTURE_2D);
#ifdef GL_TEXTURE_3D
    PRINT_STATE(GL_TEXTURE_3D);
#endif
    PRINT_STATE(GL_TEXTURE_GEN_Q);
    PRINT_STATE(GL_TEXTURE_GEN_R);
    PRINT_STATE(GL_TEXTURE_GEN_S);
    PRINT_STATE(GL_TEXTURE_GEN_T);
#undef PRINT_STATE
#undef ERROR_CHECK


#ifdef PRINT_LIGHT
#undef PRINT_LIGHT
#endif
#define PRINT_LIGHT(pname)                    \
do {                                             \
  SGVec4f color;                                 \
  glGetLightfv(GL_LIGHT0, pname, color.data());\
  stream << " " #pname " " << color;    \
} while(0)
    PRINT_LIGHT(GL_AMBIENT);
    PRINT_LIGHT(GL_DIFFUSE);
    PRINT_LIGHT(GL_SPECULAR);
    PRINT_LIGHT(GL_POSITION);
    PRINT_LIGHT(GL_SPOT_DIRECTION);

#undef PRINT_LIGHT

    if (glIsEnabled(GL_COLOR_MATERIAL)) {
      stream << " GL_COLOR_MATERIAL(";
      GLint value;
      glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER, &value);
      if (value == GL_DIFFUSE)
        stream << "GL_DIFFUSE";
      if (value == GL_AMBIENT)
        stream << "GL_AMBIENT";
      if (value == GL_AMBIENT_AND_DIFFUSE)
        stream << "GL_AMBIENT_AND_DIFFUSE";
      if (value == GL_EMISSION)
        stream << "GL_EMISSION";
      if (value == GL_SPECULAR)
        stream << "GL_SPECULAR";

#ifdef PRINT_MATERIAL
#undef PRINT_MATERIAL
#endif
#define PRINT_MATERIAL(pname)                    \
do {                                             \
  SGVec4f color;                                 \
  glGetMaterialfv(GL_FRONT, pname, color.data());\
  stream << " " #pname " GL_FRONT " << color;    \
  glGetMaterialfv(GL_BACK, pname, color.data()); \
  stream << " " #pname " GL_BACK " << color;     \
} while(0)

      PRINT_MATERIAL(GL_AMBIENT);
      PRINT_MATERIAL(GL_DIFFUSE);
      PRINT_MATERIAL(GL_EMISSION);
      PRINT_MATERIAL(GL_SPECULAR);
#undef PRINT_MATERIAL

      stream << ")";
    }

    stream << "\n";
  }
};
  
#endif
