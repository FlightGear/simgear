// Copyright (C) 2008 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "PrimitiveCollector.hxx"

#include <simgear/scene/util/OsgMath.hxx>

namespace simgear {

PrimitiveCollector::PrimitiveCollector() :
    _mode(0)
{
}

PrimitiveCollector::~PrimitiveCollector()
{
}
    
void
PrimitiveCollector::swap(PrimitiveCollector& primitiveFunctor)
{
    _vertices.swap(primitiveFunctor._vertices);
    std::swap(_mode, primitiveFunctor._mode);
}

void
PrimitiveCollector::setVertexArray(unsigned int count, const osg::Vec2* vertices)
{
    _vertices.resize(0);
    _vertices.reserve(count);
    for (unsigned i = 0; i < count; ++i)
        addVertex(osg::Vec3d(vertices[i][0], vertices[i][1], 0));
}
    
void
PrimitiveCollector::setVertexArray(unsigned int count, const osg::Vec3* vertices)
{
    _vertices.resize(0);
    _vertices.reserve(count);
    for (unsigned i = 0; i < count; ++i)
        addVertex(osg::Vec3d(vertices[i][0], vertices[i][1], vertices[i][2]));
}

void
PrimitiveCollector::setVertexArray(unsigned int count, const osg::Vec4* vertices)
{
    _vertices.resize(0);
    _vertices.reserve(count);
    for (unsigned i = 0; i < count; ++i)
        addVertex(osg::Vec4d(vertices[i][0], vertices[i][1], vertices[i][2], vertices[i][3]));
}

void
PrimitiveCollector::setVertexArray(unsigned int count, const osg::Vec2d* vertices)
{
    _vertices.resize(0);
    _vertices.reserve(count);
    for (unsigned i = 0; i < count; ++i)
        addVertex(osg::Vec3d(vertices[i][0], vertices[i][1], 0));
}

void
PrimitiveCollector::setVertexArray(unsigned int count, const osg::Vec3d* vertices)
{
    _vertices.resize(0);
    _vertices.reserve(count);
    for (unsigned i = 0; i < count; ++i)
        addVertex(osg::Vec3d(vertices[i][0], vertices[i][1], vertices[i][2]));
}

void
PrimitiveCollector::setVertexArray(unsigned int count, const osg::Vec4d* vertices) 
{
    _vertices.resize(0);
    _vertices.reserve(count);
    for (unsigned i = 0; i < count; ++i)
        addVertex(osg::Vec4d(vertices[i][0], vertices[i][1], vertices[i][2], vertices[i][3]));
}

void
PrimitiveCollector::drawArrays(GLenum mode, GLint first, GLsizei count)
{
    if (_vertices.empty() || count <= 0)
        return;

    GLsizei end = first + count;
    switch(mode) {
    case (GL_TRIANGLES):
        for (GLsizei i = first; i < end - 2; i += 3) {
            addTriangle(i, i + 1, i + 2);
        }
        break;

    case (GL_TRIANGLE_STRIP):
        for (GLsizei i = first; i < end - 2; ++i) {
            addTriangle(i, i + 1, i + 2);
        }
        break;

    case (GL_QUADS):
        for (GLsizei i = first; i < end - 3; i += 4) {
            addQuad(i, i + 1, i + 2, i + 3);
        }
        break;

    case (GL_QUAD_STRIP):
        for (GLsizei i = first; i < end - 3; i += 2) {
            addQuad(i, i + 1, i + 2, i + 3);
        }
        break;

    case (GL_POLYGON): // treat polygons as GL_TRIANGLE_FAN
    case (GL_TRIANGLE_FAN):
        for (GLsizei i = first; i < end - 2; ++i) {
            addTriangle(first, i + 1, i + 2);
        }
    break;

    case (GL_POINTS):
        for (GLsizei i = first; i < end; ++i) {
            addPoint(i);
        }
        break;

    case (GL_LINES):
        for (GLsizei i = first; i < end - 1; i += 2) {
            addLine(i, i + 1);
        }
        break;

    case (GL_LINE_STRIP):
        for (GLsizei i = first; i < end - 1; ++i) {
            addLine(i, i + 1);
        }
        break;

    case (GL_LINE_LOOP):
        for (GLsizei i = first; i < end - 1; ++i) {
            addLine(i, i + 1);
        }
        addLine(end - 1, first);
        break;

    default:
        break;
    }
}
  
template<typename index_type>
void
PrimitiveCollector::drawElementsTemplate(GLenum mode, GLsizei count, const index_type* indices)
{
    if (_vertices.empty() || indices == 0 || count <= 0)
        return;
    
    switch(mode) {
    case (GL_TRIANGLES):
        for (GLsizei i = 0; i < count - 2; i += 3) {
            addTriangle(indices[i], indices[i + 1], indices[i + 2]);
        }
        break;

    case (GL_TRIANGLE_STRIP):
        for (GLsizei i = 0; i < count - 2; ++i) {
            addTriangle(indices[i], indices[i + 1], indices[i + 2]);
        }
        break;

    case (GL_QUADS):
        for (GLsizei i = 0; i < count - 3; i += 4) {
            addQuad(indices[i], indices[i + 1], indices[i + 2], indices[i + 3]);
        }
        break;

    case (GL_QUAD_STRIP):
        for (GLsizei i = 0; i < count - 3; i += 2) {
            addQuad(indices[i], indices[i + 1], indices[i + 2], indices[i + 3]);
        }
        break;

    case (GL_POLYGON):
    case (GL_TRIANGLE_FAN):
        for (GLsizei i = 0; i < count - 2; ++i) {
            addTriangle(indices[0], indices[i + 1], indices[i + 2]);
        }
    break;

    case (GL_POINTS):
        for(GLsizei i = 0; i < count; ++i) {
            addPoint(indices[i]);
        }
        break;

    case (GL_LINES):
        for (GLsizei i = 0; i < count - 1; i += 2) {
            addLine(indices[i], indices[i + 1]);
        }
        break;

    case (GL_LINE_STRIP):
        for (GLsizei i = 0; i < count - 1; ++i) {
            addLine(indices[i], indices[i + 1]);
        }
        break;

    case (GL_LINE_LOOP):
        for (GLsizei i = 0; i < count - 1; ++i) {
            addLine(indices[i], indices[i + 1]);
        }
        addLine(indices[count - 1], indices[0]);
        break;

    default:
        break;
    }
}

void
PrimitiveCollector::drawElements(GLenum mode, GLsizei count, const GLubyte* indices)
{
    drawElementsTemplate(mode, count, indices);
}

void PrimitiveCollector::drawElements(GLenum mode, GLsizei count, const GLushort* indices)
{
    drawElementsTemplate(mode, count, indices);
}

void
PrimitiveCollector::drawElements(GLenum mode, GLsizei count, const GLuint* indices)
{
    drawElementsTemplate(mode, count, indices);
}

void
PrimitiveCollector::begin(GLenum mode)
{
    _mode = mode;
    _vertices.resize(0);
}

void
PrimitiveCollector::vertex(const osg::Vec2& v)
{
    addVertex(osg::Vec3d(v[0], v[1], 0));
}

void
PrimitiveCollector::vertex(const osg::Vec3& v)
{
    addVertex(osg::Vec3d(v[0], v[1], v[2]));
}

void
PrimitiveCollector::vertex(const osg::Vec4& v)
{
    addVertex(osg::Vec4d(v[0], v[1], v[2], v[3]));
}

void
PrimitiveCollector::vertex(float x, float y)
{
    addVertex(osg::Vec3d(x, y, 0));
}

void
PrimitiveCollector::vertex(float x, float y, float z)
{
    addVertex(osg::Vec3d(x, y, z));
}

void
PrimitiveCollector::vertex(float x, float y, float z, float w)
{
    addVertex(osg::Vec4d(x, y, z, w));
}

void
PrimitiveCollector::end()
{
    if (_vertices.empty())
        return;

    drawArrays(_mode, 0, _vertices.size());
}

void
PrimitiveCollector::addVertex(const osg::Vec3d& v)
{
    _vertices.push_back(v);
}

void
PrimitiveCollector::addVertex(const osg::Vec4d& v)
{
    _vertices.push_back(osg::Vec3d(v[0]/v[3], v[1]/v[3], v[2]/v[3]));
}

void
PrimitiveCollector::addPoint(unsigned i1)
{
    if (_vertices.size() <= i1)
        return;
    addPoint(_vertices[i1]);
}

void
PrimitiveCollector::addLine(unsigned i1, unsigned i2)
{
    size_t size = _vertices.size();
    if (size <= i1 || size <= i2)
        return;
    addLine(_vertices[i1], _vertices[i2]);
}

void
PrimitiveCollector::addTriangle(unsigned i1, unsigned i2, unsigned i3)
{
    size_t size = _vertices.size();
    if (size <= i1 || size <= i2 || size <= i3)
        return;
    addTriangle(_vertices[i1], _vertices[i2], _vertices[i3]);
}

void
PrimitiveCollector::addQuad(unsigned i1, unsigned i2, unsigned i3, unsigned i4)
{
    addTriangle(i1, i2, i3);
    addTriangle(i1, i3, i4);
}

}
