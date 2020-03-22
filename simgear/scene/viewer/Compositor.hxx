// Copyright (C) 2018  Fernando García Liñán <fernandogarcialinan@gmail.com>
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef SG_COMPOSITOR_HXX
#define SG_COMPOSITOR_HXX

#include <unordered_map>
#include <vector>

// For osgUtil::LineSegmentIntersector::Intersections, which is a typedef.
#include <osgUtil/LineSegmentIntersector>

#include "CompositorBuffer.hxx"
#include "CompositorPass.hxx"

class SGPropertyNode;

namespace simgear {
namespace compositor {

/**
 * A Compositor manages the rendering pipeline of a single physical camera,
 * usually via a property tree interface.
 *
 * The building blocks that define a Compositor are:
 *   - Buffers. They represent a zone of GPU memory. This is implemented in the
 *     form of an OpenGL texture, but any type of information can be stored
 *     (which can be useful in compute shaders for example).
 *   - Passes. They represent render operations. They can get buffers as input
 *     and they can output to other buffers. They are also integrated with the
 *     Effects framework, so the OpenGL internal state is configurable per pass.
 */
class Compositor : public osg::Referenced {
public:
    enum BuiltinUniform {
        VIEWPORT_SIZE = 0,
        VIEW_MATRIX,
        VIEW_MATRIX_INV,
        PROJECTION_MATRIX,
        PROJECTION_MATRIX_INV,
        PREV_VIEW_MATRIX,
        PREV_VIEW_MATRIX_INV,
        PREV_PROJECTION_MATRIX,
        PREV_PROJECTION_MATRIX_INV,
        CAMERA_POSITION_CART,
        CAMERA_POSITION_GEOD,
        NEAR_FAR_PLANES,
        FCOEF,
        LIGHT_DIRECTION,
        TOTAL_BUILTIN_UNIFORMS
    };

    Compositor(osg::View *view,
               osg::GraphicsContext *gc,
               osg::Viewport *viewport);
    ~Compositor();

    /**
     * \brief Create a Compositor from a property tree.
     *
     * @param view The View where the passes will be added as slaves.
     * @param gc The context where the internal osg::Cameras will draw on.
     * @param viewport The viewport position and size inside the window.
     * @param property_list A valid property list that describes the Compositor.
     * @return A Compositor or a null pointer if there was an error.
     */
    static Compositor *create(osg::View *view,
                              osg::GraphicsContext *gc,
                              osg::Viewport *viewport,
                              const SGPropertyNode *property_list,
                              const SGReaderWriterOptions *options);
    /**
     * \overload
     * \brief Create a Compositor from a file.
     *
     * @param name Name of the compositor. The function will search for a file
     *             named <name>.xml in $FG_ROOT.
     */
    static Compositor *create(osg::View *view,
                              osg::GraphicsContext *gc,
                              osg::Viewport *viewport,
                              const std::string &name,
                              const SGReaderWriterOptions *options);

    void               update(const osg::Matrix &view_matrix,
                              const osg::Matrix &proj_matrix);

    void               resized();

    osg::View         *getView() const { return _view; }

    osg::GraphicsContext *getGraphicsContext() const { return _gc; }

    osg::Viewport     *getViewport() const { return _viewport; }

    typedef std::array<
        osg::ref_ptr<osg::Uniform>,
        TOTAL_BUILTIN_UNIFORMS> BuiltinUniforms;
    const BuiltinUniforms &getUniforms() const { return _uniforms; }

    void               addBuffer(const std::string &name, Buffer *buffer);
    void               addPass(Pass *pass);

    void               setName(const std::string &name) { _name = name; }
    const std::string &getName() const { return _name; }

    typedef std::unordered_map<std::string, osg::ref_ptr<Buffer>> BufferMap;
    const BufferMap &  getBufferMap() const { return _buffers; }
    Buffer *           getBuffer(const std::string &name) const;

    typedef std::vector<osg::ref_ptr<Pass>> PassList;
    const PassList &   getPassList() const { return _passes; }
    unsigned int       getNumPasses() const { return _passes.size(); }
    Pass *             getPass(size_t index) const { return _passes[index]; }
    Pass *             getPass(const std::string &name) const;

protected:
    osg::View                   *_view;
    osg::GraphicsContext        *_gc;
    osg::ref_ptr<osg::Viewport>  _viewport;
    std::string                  _name;
    BufferMap                    _buffers;
    PassList                     _passes;
    BuiltinUniforms              _uniforms;
};

} // namespace compositor
} // namespace simgear

#endif /* SG_COMPOSITOR_HXX */
