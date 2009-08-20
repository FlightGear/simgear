/* -*-c++-*-
 *
 * Copyright (C) 2009 Tim Moore
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

#include "PrimitiveUtils.hxx"
#include <iostream>
using namespace osg;

namespace
{
class GetPrimitive : public PrimitiveFunctor
{
public:
    simgear::Primitive result;
    GetPrimitive(unsigned int primitiveIndex)
        : _primitiveIndex(primitiveIndex), _primitivesSeen(0),
          _vertexArrayPtr(0), _modeCache(0)
    {
        result.numVerts = 0;
    }
    
    virtual void setVertexArray(unsigned int,const Vec2*)
    {
        notify(WARN)<<"GetPrimitive does not support Vec2* vertex arrays"<<std::endl;
    }

    virtual void setVertexArray(unsigned int count,const Vec3* vertices)
    {
        _vertexArrayPtr = vertices;
    }

    virtual void setVertexArray(unsigned int,const Vec4* )
    {
        notify(WARN)<<"GetPrimitive does not support Vec4* vertex arrays"<<std::endl;
    }

    virtual void setVertexArray(unsigned int,const Vec2d*)
    {
        notify(WARN)<<"GetPrimitive does not support Vec2d* vertex arrays"<<std::endl;
    }

    virtual void setVertexArray(unsigned int,const Vec3d*)
    {
        notify(WARN)<<"GetPrimitive does not support Vec3d* vertex arrays"<<std::endl;
    }

    virtual void setVertexArray(unsigned int,const Vec4d* )
    {
        notify(WARN)<<"GetPrimitive does not support Vec4d* vertex arrays"<<std::endl;
    }
    
    void drawArrays(GLenum mode_, GLint first, GLsizei count)
    {
        if (_primitiveIndex < _primitivesSeen) {
            return;
        }
        int numPrims = getNumPrims(mode_, count);
        if (_primitivesSeen + numPrims < _primitiveIndex) {
            _primitivesSeen += numPrims;
            return;
        }
        int primInSet = _primitiveIndex - _primitivesSeen;
        switch (mode_) {
        case GL_POINTS:
            result.numVerts = 1;
            result.vertices[0] = _vertexArrayPtr[first + primInSet];
            break;
        case GL_LINES:
            result.numVerts = 2;
            result.vertices[0] = _vertexArrayPtr[first + primInSet * 2];
            result.vertices[1] = _vertexArrayPtr[first + primInSet * 2 + 1];
            break;
        case GL_TRIANGLES:
            result.numVerts = 3;
            for (int i = 0; i < 3; ++i)
                result.vertices[i] = _vertexArrayPtr[first + primInSet * 3 + i];
            break;
        case GL_QUADS:
            result.numVerts = 4;
            for (int i = 0; i < 4; ++i)
                result.vertices[i] = _vertexArrayPtr[first + primInSet * 4 + i];
            break;
        case GL_LINE_STRIP:
            result.numVerts = 2;
            result.vertices[0] = _vertexArrayPtr[first + primInSet];
            result.vertices[1] = _vertexArrayPtr[first + primInSet + 1];
            break;
        case GL_LINE_LOOP:
            result.numVerts = 2;
            if (primInSet < numPrims - 1) {
                result.vertices[0] = _vertexArrayPtr[first + primInSet];
                result.vertices[1] = _vertexArrayPtr[first + primInSet + 1];
            } else {
                result.vertices[0] = _vertexArrayPtr[first + count - 1];
                result.vertices[1] = _vertexArrayPtr[first];
            }
            break;
        case GL_TRIANGLE_STRIP:
            result.numVerts = 3;
                result.vertices[0] = _vertexArrayPtr[first + primInSet];
            if (primInSet % 2) {
                result.vertices[1] = _vertexArrayPtr[first + primInSet + 2];
                result.vertices[2] = _vertexArrayPtr[first + primInSet + 1];
            } else {
                result.vertices[1] = _vertexArrayPtr[first + primInSet + 1];
                result.vertices[2] = _vertexArrayPtr[first + primInSet + 2];
            }
            break;
        case GL_TRIANGLE_FAN:
        case GL_POLYGON:
            result.numVerts = 3;
            result.vertices[0] = _vertexArrayPtr[first];
            result.vertices[1] = _vertexArrayPtr[first + 1 + primInSet];
            result.vertices[2] = _vertexArrayPtr[first + 1 + primInSet + 1];
            break;
        case GL_QUAD_STRIP:
            result.numVerts = 4;
            result.vertices[0] = _vertexArrayPtr[first + primInSet / 2];
            result.vertices[1] = _vertexArrayPtr[first + primInSet / 2 + 1];
            result.vertices[2] = _vertexArrayPtr[first + primInSet / 2 + 3];
            result.vertices[3] = _vertexArrayPtr[first + primInSet / 2 + 2];
            break;
        default:
            break;
        }
        _primitivesSeen += numPrims;        
    }

    template<class IndexType>
    void drawElementsTemplate(GLenum mode_, GLsizei count,
                              const IndexType* indices)
    {
        if (_primitiveIndex < _primitivesSeen) {
            return;
        }
        int numPrims = getNumPrims(mode_, count);
        if (_primitivesSeen + numPrims < _primitiveIndex) {
            _primitivesSeen += numPrims;
            return;
        }
        int primInSet = _primitiveIndex - _primitivesSeen;
        switch (mode_) {
        case GL_POINTS:
            result.numVerts = 1;            
            result.vertices[0] = _vertexArrayPtr[indices[primInSet]];
            break;
        case GL_LINES:
            result.numVerts = 2;            
            result.vertices[0] = _vertexArrayPtr[indices[primInSet * 2]];
            result.vertices[1] = _vertexArrayPtr[indices[primInSet * 2 + 1]];
            break;
        case GL_TRIANGLES:
            result.numVerts = 3;            
            for (int i = 0; i < 3; ++i)
                result.vertices[i] = _vertexArrayPtr[indices[primInSet * 3 + i]];
            break;
        case GL_QUADS:
            result.numVerts = 4;            
            for (int i = 0; i < 4; ++i)
                result.vertices[i] = _vertexArrayPtr[indices[primInSet * 4 + i]];
            break;
        case GL_LINE_STRIP:
            result.numVerts = 2;            
            result.vertices[0] = _vertexArrayPtr[indices[primInSet]];
            result.vertices[1] = _vertexArrayPtr[indices[primInSet + 1]];
            break;
        case GL_LINE_LOOP:
            result.numVerts = 2;            
            if (primInSet < numPrims - 1) {
                result.vertices[0] = _vertexArrayPtr[indices[primInSet]];
                result.vertices[1] = _vertexArrayPtr[indices[primInSet + 1]];
            } else {
                result.vertices[0] = _vertexArrayPtr[indices[count - 1]];
                result.vertices[1] = _vertexArrayPtr[indices[0]];
            }
            break;
        case GL_TRIANGLE_STRIP:
            result.numVerts = 3;            
                result.vertices[0] = _vertexArrayPtr[indices[primInSet]];
            if (primInSet % 2) {
                result.vertices[1] = _vertexArrayPtr[indices[primInSet + 2]];
                result.vertices[2] = _vertexArrayPtr[indices[primInSet + 1]];
            } else {
                result.vertices[1] = _vertexArrayPtr[indices[primInSet + 1]];
                result.vertices[2] = _vertexArrayPtr[indices[primInSet + 2]];
            }
            break;
        case GL_TRIANGLE_FAN:
        case GL_POLYGON:
            result.numVerts = 3;            
            result.vertices[0] = _vertexArrayPtr[indices[0]];
            result.vertices[1] = _vertexArrayPtr[indices[1 + primInSet]];
            result.vertices[2] = _vertexArrayPtr[indices[1 + primInSet + 1]];
            break;
        case GL_QUAD_STRIP:
            result.numVerts = 4;            
            result.vertices[0] = _vertexArrayPtr[indices[primInSet / 2]];
            result.vertices[1] = _vertexArrayPtr[indices[primInSet / 2 + 1]];
            result.vertices[2] = _vertexArrayPtr[indices[primInSet / 2 + 3]];
            result.vertices[3] = _vertexArrayPtr[indices[primInSet / 2 + 2]];
            break;
        default:
            break;
        }
        _primitivesSeen += numPrims;        
    }

    void drawElements(GLenum mode_, GLsizei count, const GLubyte* indices)
    {
        drawElementsTemplate(mode_, count, indices);
    }

    void drawElements(GLenum mode_, GLsizei count, const GLushort* indices)
    {
        drawElementsTemplate(mode_, count, indices);
    }

    void drawElements(GLenum mode_,GLsizei count,const GLuint* indices)
    {
        drawElementsTemplate(mode_, count, indices);
    }

    virtual void begin(GLenum mode)
    {
        _modeCache = mode;
        _vertexCache.clear();
    }

    void vertex(const Vec2& vert)
    {
        _vertexCache.push_back(osg::Vec3(vert[0],vert[1],0.0f));
    }
    void vertex(const Vec3& vert)
    {
        _vertexCache.push_back(vert);
    }
    void vertex(const Vec4& vert)
    {
        _vertexCache.push_back(osg::Vec3(vert[0],vert[1],vert[2])/vert[3]);
    }
    void vertex(float x,float y)
    {
        _vertexCache.push_back(osg::Vec3(x,y,0.0f));
    }
    void vertex(float x,float y,float z)
    {
        _vertexCache.push_back(osg::Vec3(x,y,z));
    }
    void vertex(float x,float y,float z,float w)
    {
        _vertexCache.push_back(osg::Vec3(x,y,z)/w);
    
    }
    
    void end()
    {
        if (!_vertexCache.empty()) {
            const Vec3* oldVert = _vertexArrayPtr;
            setVertexArray(_vertexCache.size(), &_vertexCache.front());
            drawArrays(_modeCache, 0, _vertexCache.size());
            _vertexArrayPtr = oldVert;
        }
    }
    
protected:
    int getNumPrims(GLenum mode, int count)
    {
        switch (mode) {
        case GL_POINTS:
            return count;
        case GL_LINES:
            return count / 2;
        case GL_TRIANGLES:
            return count / 3;
        case GL_QUADS:
            return count / 4;
        case GL_LINE_STRIP:
            return count - 1;
        case GL_LINE_LOOP:
            return count;
        case GL_TRIANGLE_STRIP:
            return count - 2;
        case GL_TRIANGLE_FAN:
        case GL_POLYGON:
            return count - 2;
        case GL_QUAD_STRIP:
            return (count - 2) / 2;
        default:
            std::cerr << "FATAL: unknown GL mode " << mode << std::endl;
            throw new std::exception();
        }
    }
    unsigned _primitiveIndex;
    unsigned _primitivesSeen;
    const Vec3* _vertexArrayPtr;
    GLenum _modeCache;
    std::vector<Vec3>  _vertexCache;
};
}

namespace simgear
{
Primitive getPrimitive(Drawable* drawable, unsigned primitiveIndex)
{
    GetPrimitive getPrim(primitiveIndex);
    drawable->accept(getPrim);
    return getPrim.result;
}
}
