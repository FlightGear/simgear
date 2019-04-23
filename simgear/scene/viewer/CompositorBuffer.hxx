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

#ifndef SG_COMPOSITOR_BUFFER_HXX
#define SG_COMPOSITOR_BUFFER_HXX

#include <osg/Texture>

class SGPropertyNode;

namespace simgear {

class SGReaderWriterOptions;

namespace compositor {

class Compositor;

struct Buffer : public osg::Referenced {
    Buffer() : width_scale(0.0f), height_scale(0.0f) {}

    osg::ref_ptr<osg::Texture> texture;

    /**
     * The amount to multiply the size of the default framebuffer.
     * A factor of 0.0 means that the buffer has a fixed size.
     */
    float width_scale, height_scale;
};

Buffer *buildBuffer(Compositor *compositor, const SGPropertyNode *node,
                    const SGReaderWriterOptions *options);

} // namespace compositor
} // namespace simgear

#endif /* SG_COMPOSITOR_BUFFER_HXX */
