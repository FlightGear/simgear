// Copyright (C) 2018 - 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <osg/Texture>

class SGPropertyNode;

namespace simgear {

class SGReaderWriterOptions;

namespace compositor {

class Compositor;

struct Buffer : public osg::Referenced {
    Buffer() : width_scale(0.0f), height_scale(0.0f), mvr(false) {}

    osg::ref_ptr<osg::Texture> texture;

    /**
     * The amount to multiply the size of the default framebuffer.
     * A factor of 0.0 means that the buffer has a fixed size.
     */
    float width_scale, height_scale;

    /// Whether this is an MVR buffer.
    bool mvr;
};

Buffer *buildBuffer(Compositor *compositor, const SGPropertyNode *node,
                    const SGReaderWriterOptions *options);

} // namespace compositor
} // namespace simgear
