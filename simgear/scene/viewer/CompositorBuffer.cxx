// Copyright (C) 2018 - 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "CompositorBuffer.hxx"

#include <osg/GL>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture2DMultisample>
#include <osg/Texture3D>
#include <osg/TextureRectangle>
#include <osg/TextureCubeMap>

#include <simgear/props/props.hxx>
#include <simgear/props/vectorPropTemplates.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "Compositor.hxx"
#include "CompositorUtil.hxx"

#ifndef GL_R11F_G11F_B10F
#define GL_R11F_G11F_B10F                 0x8C3A
#endif
#ifndef GL_UNSIGNED_INT_10F_11F_11F_REV
#define GL_UNSIGNED_INT_10F_11F_11F_REV   0x8C3B
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8               0x88F0
#endif
#ifndef GL_DEPTH32F_STENCIL8
#define GL_DEPTH32F_STENCIL8              0x8CAD
#endif
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL                  0x84F9
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8              0x84FA
#endif
#ifndef GL_FLOAT_32_UNSIGNED_INT_24_8_REV
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
#endif

using std::max;
using std::min;

namespace simgear {
namespace compositor {

struct BufferFormat {
    GLint internal_format;
    GLenum source_format;
    GLenum source_type;
};

PropStringMap<BufferFormat> buffer_format_map {
    {"rgb8", {GL_RGB8, GL_RGBA, GL_UNSIGNED_BYTE}},
    {"rgba8", {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}},
    {"rgb16f", {GL_RGB16F_ARB, GL_RGBA, GL_HALF_FLOAT}},
    {"rgb32f", {GL_RGB32F_ARB, GL_RGBA, GL_FLOAT}},
    {"rgba16f", {GL_RGBA16F_ARB, GL_RGBA, GL_HALF_FLOAT}},
    {"rgba32f", {GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT}},
    {"r8", {GL_R8, GL_RED, GL_UNSIGNED_BYTE}},
    {"r16f", {GL_R16F, GL_RED, GL_HALF_FLOAT}},
    {"r32f", {GL_R32F, GL_RED, GL_FLOAT}},
    {"rg16f", {GL_RG16F, GL_RG, GL_HALF_FLOAT}},
    {"rg32f", {GL_RG32F, GL_RG, GL_FLOAT}},
    {"rgb10-a2", {GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_10_10_10_2}},
    {"r11f-g11f-b10f", {GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV}},
    {"depth16", {GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT}},
    {"depth24", {GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT}},
    {"depth24-stencil8", {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8}},
    {"depth32f", {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT}},
    {"depth32f-stencil8", {GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV}},
    {"r8ui", {GL_R8UI, GL_RED, GL_UNSIGNED_BYTE}},
    {"r16ui", {GL_R16UI, GL_RED, GL_UNSIGNED_SHORT}},
    {"r32ui", {GL_R32UI, GL_RED, GL_UNSIGNED_INT}}
};

PropStringMap<osg::Texture::WrapMode> wrap_mode_map = {
    {"clamp", osg::Texture::CLAMP},
    {"clamp-to-edge", osg::Texture::CLAMP_TO_EDGE},
    {"clamp-to-border", osg::Texture::CLAMP_TO_BORDER},
    {"repeat", osg::Texture::REPEAT},
    {"mirror", osg::Texture::MIRROR}
};

PropStringMap<osg::Texture::FilterMode> filter_mode_map = {
    {"linear", osg::Texture::LINEAR},
    {"linear-mipmap-linear", osg::Texture::LINEAR_MIPMAP_LINEAR},
    {"linear-mipmap-nearest", osg::Texture::LINEAR_MIPMAP_NEAREST},
    {"nearest", osg::Texture::NEAREST},
    {"nearest-mipmap-linear", osg::Texture::NEAREST_MIPMAP_LINEAR},
    {"nearest-mipmap-nearest", osg::Texture::NEAREST_MIPMAP_NEAREST}
};

PropStringMap<osg::Texture::ShadowTextureMode> shadow_texture_mode_map = {
    {"luminance", osg::Texture::LUMINANCE},
    {"intensity", osg::Texture::INTENSITY},
    {"alpha", osg::Texture::ALPHA}
};

PropStringMap<osg::Texture::ShadowCompareFunc> shadow_compare_func_map = {
    {"never", osg::Texture::NEVER},
    {"less", osg::Texture::LESS},
    {"equal", osg::Texture::EQUAL},
    {"lequal", osg::Texture::LEQUAL},
    {"greater", osg::Texture::GREATER},
    {"notequal", osg::Texture::NOTEQUAL},
    {"gequal", osg::Texture::GEQUAL},
    {"always", osg::Texture::ALWAYS}
};

Buffer *
buildBuffer(Compositor *compositor, const SGPropertyNode *node,
            const SGReaderWriterOptions *options)
{
    std::string type = node->getStringValue("type");
    if (type.empty()) {
        SG_LOG(SG_INPUT, SG_ALERT, "buildBuffer: No type specified");
        return 0;
    }

    osg::ref_ptr<Buffer> buffer = new Buffer;
    osg::Texture *texture = nullptr;

    int width = 0;
    const SGPropertyNode *p_width = getPropertyChild(node, "width");
    if (p_width) {
        if (p_width->getStringValue() == std::string("screen")) {
            buffer->width_scale = 1.0f;
            const SGPropertyNode *p_w_scale = getPropertyChild(node, "screen-width-scale");
            if (p_w_scale)
                buffer->width_scale = p_w_scale->getFloatValue();
            width = buffer->width_scale * compositor->getViewport()->width();
        } else {
            width = p_width->getIntValue();
        }
    }
    int height = 0;
    const SGPropertyNode *p_height = getPropertyChild(node, "height");
    if (p_height) {
        if (p_height->getStringValue() == std::string("screen")) {
            buffer->height_scale = 1.0f;
            const SGPropertyNode *p_h_scale = getPropertyChild(node, "screen-height-scale");
            if (p_h_scale)
                buffer->height_scale = p_h_scale->getFloatValue();
            height = buffer->height_scale * compositor->getViewport()->height();
        } else {
            height = p_height->getIntValue();
        }
    }
    int depth = 0;
    const SGPropertyNode *p_depth = getPropertyChild(node, "depth");
    if (p_depth)
        depth = p_depth->getIntValue();

    auto get_mipmap_levels = [&]() -> int {
        int mipmap_levels = 0;
        const SGPropertyNode *p_mipmap_levels = node->getNode("mipmap-levels");
        if (p_mipmap_levels) {
            if (p_mipmap_levels->getStringValue() == std::string("auto"))
                mipmap_levels = 1 + floor(log2((float)max(max(width, height), depth)));
            else
                mipmap_levels = p_mipmap_levels->getIntValue();
        }
        return mipmap_levels;
    };

    if (type == "1d") {
        osg::Texture1D *tex1D = new osg::Texture1D;
        tex1D->setTextureWidth(width);
        tex1D->setNumMipmapLevels(get_mipmap_levels());
        texture = tex1D;
    } else if (type == "2d") {
        osg::Texture2D *tex2D = new osg::Texture2D;
        tex2D->setTextureSize(width, height);
        tex2D->setNumMipmapLevels(get_mipmap_levels());
        texture = tex2D;
    } else if (type == "2d-array") {
        osg::Texture2DArray *tex2D_array = new osg::Texture2DArray;
        tex2D_array->setTextureSize(width, height, depth);
        tex2D_array->setNumMipmapLevels(get_mipmap_levels());
        texture = tex2D_array;
    } else if (type == "2d-multisample") {
        osg::Texture2DMultisample *tex2DMS = new osg::Texture2DMultisample;
        tex2DMS->setTextureSize(width, height);
        tex2DMS->setNumSamples(node->getIntValue("num-samples", 0));
        texture = tex2DMS;
    } else if (type == "3d") {
        osg::Texture3D *tex3D = new osg::Texture3D;
        tex3D->setTextureSize(width, height, depth);
        tex3D->setNumMipmapLevels(get_mipmap_levels());
        texture = tex3D;
    } else if (type == "rect") {
        osg::TextureRectangle *tex_rect = new osg::TextureRectangle;
        tex_rect->setTextureSize(width, height);
        texture = tex_rect;
    } else if (type == "cubemap") {
        osg::TextureCubeMap *tex_cubemap = new osg::TextureCubeMap;
        tex_cubemap->setTextureSize(width, height);
        tex_cubemap->setNumMipmapLevels(get_mipmap_levels());
        texture = tex_cubemap;
    } else {
        SG_LOG(SG_INPUT, SG_ALERT, "Unknown texture type '" << type << "'");
        return 0;
    }
    buffer->texture = texture;

    bool resize_npot = node->getBoolValue("resize-npot", false);
    texture->setResizeNonPowerOfTwoHint(resize_npot);

    BufferFormat format;
    if (findPropString(node, "format", format, buffer_format_map)) {
        texture->setInternalFormat(format.internal_format);
        texture->setSourceFormat(format.source_format);
        texture->setSourceType(format.source_type);
    } else {
        texture->setInternalFormat(GL_RGBA);
        SG_LOG(SG_INPUT, SG_WARN, "Unknown buffer format '"
               << node->getStringValue("format") << "', using RGBA");
    }

    osg::Texture::FilterMode filter_mode = osg::Texture::LINEAR;
    findPropString(node, "min-filter", filter_mode, filter_mode_map);
    texture->setFilter(osg::Texture::MIN_FILTER, filter_mode);
    findPropString(node, "mag-filter", filter_mode, filter_mode_map);
    texture->setFilter(osg::Texture::MAG_FILTER, filter_mode);

    osg::Texture::WrapMode wrap_mode = osg::Texture::CLAMP_TO_BORDER;
    findPropString(node, "wrap-s", wrap_mode, wrap_mode_map);
    texture->setWrap(osg::Texture::WRAP_S, wrap_mode);
    findPropString(node, "wrap-t", wrap_mode, wrap_mode_map);
    texture->setWrap(osg::Texture::WRAP_T, wrap_mode);
    findPropString(node, "wrap-r", wrap_mode, wrap_mode_map);
    texture->setWrap(osg::Texture::WRAP_R, wrap_mode);

    float anis = node->getFloatValue("anisotropy", 1.0f);
    texture->setMaxAnisotropy(anis);

    osg::Vec4f border_color(0.0f, 0.0f, 0.0f, 0.0f);
    const SGPropertyNode *p_border_color = node->getChild("border-color");
    if (p_border_color)
        border_color = toOsg(p_border_color->getValue<SGVec4d>());
    texture->setBorderColor(border_color);

    bool shadow_comparison = node->getBoolValue("shadow-comparison", false);
    texture->setShadowComparison(shadow_comparison);
    if (shadow_comparison) {
        osg::Texture::ShadowTextureMode shadow_texture_mode =
            osg::Texture::LUMINANCE;
        findPropString(node, "shadow-texture-mode",
                       shadow_texture_mode, shadow_texture_mode_map);
        texture->setShadowTextureMode(shadow_texture_mode);

        osg::Texture::ShadowCompareFunc shadow_compare_func =
            osg::Texture::LEQUAL;
        findPropString(node, "shadow-compare-func",
                       shadow_compare_func, shadow_compare_func_map);
        texture->setShadowCompareFunc(shadow_compare_func);
    }

    return buffer.release();
}

} // namespace compositor
} // namespace simgear
