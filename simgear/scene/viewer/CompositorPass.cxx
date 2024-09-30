// Copyright (C) 2018 - 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "CompositorPass.hxx"

#include <osg/BindImageTexture>
#include <osg/Depth>
#include <osg/DispatchCompute>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PolygonMode>
#include <osg/io_utils>

#include <osgUtil/CullVisitor>

#include <simgear/props/vectorPropTemplates.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/ProjectionMatrix.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/structure/exception.hxx>

#include "ClusteredShading.hxx"
#include "Compositor.hxx"
#include "CompositorUtil.hxx"


namespace simgear {
namespace compositor {

PropStringMap<osg::Camera::BufferComponent> buffer_component_map = {
    {"color", osg::Camera::COLOR_BUFFER},
    {"color0", osg::Camera::COLOR_BUFFER0},
    {"color1", osg::Camera::COLOR_BUFFER1},
    {"color2", osg::Camera::COLOR_BUFFER2},
    {"color3", osg::Camera::COLOR_BUFFER3},
    {"color4", osg::Camera::COLOR_BUFFER4},
    {"color5", osg::Camera::COLOR_BUFFER5},
    {"color6", osg::Camera::COLOR_BUFFER6},
    {"color7", osg::Camera::COLOR_BUFFER7},
    {"depth", osg::Camera::DEPTH_BUFFER},
    {"stencil", osg::Camera::STENCIL_BUFFER},
    {"packed-depth-stencil", osg::Camera::PACKED_DEPTH_STENCIL_BUFFER}
};

class CSMCullCallback : public osg::NodeCallback {
public:
    CSMCullCallback(const std::string &suffix) {
        _light_matrix_uniform = new osg::Uniform(
            osg::Uniform::FLOAT_MAT4, std::string("fg_LightMatrix_") + suffix);
    }

    virtual void operator()(osg::Node *node, osg::NodeVisitor *nv) {
        osg::Camera *camera = static_cast<osg::Camera *>(node);

        traverse(node, nv);

        // The light matrix uniform is updated after the traverse in case the
        // OSG near/far plane calculations were enabled
        osg::Matrixf light_matrix =
            // Include the real camera inverse view matrix because if the shader
            // used world coordinates, there would be precision issues.
            _real_inverse_view *
            camera->getViewMatrix() *
            camera->getProjectionMatrix() *
            // Bias matrices
            osg::Matrix::translate(1.0, 1.0, 1.0) *
            osg::Matrix::scale(0.5, 0.5, 0.5);
        _light_matrix_uniform->set(light_matrix);
    }

    void setRealInverseViewMatrix(const osg::Matrix &matrix) {
        _real_inverse_view = matrix;
    }

    osg::Uniform *getLightMatrixUniform() const {
        return _light_matrix_uniform.get();
    }

protected:
    osg::Matrix                _real_inverse_view;
    osg::ref_ptr<osg::Uniform> _light_matrix_uniform;
};

class SceneCullCallback : public osg::NodeCallback {
public:
    SceneCullCallback(ClusteredShading *clustered) :
        _clustered(clustered) {}

    virtual void operator()(osg::Node *node, osg::NodeVisitor *nv) {
        osg::Camera *camera = static_cast<osg::Camera *>(node);
        EffectCullVisitor *cv = dynamic_cast<EffectCullVisitor *>(nv);

        cv->traverse(*camera);

        if (_clustered) {
            // Retrieve the light list from the cull visitor
            SGLightList light_list = cv->getLightList();
            _clustered->update(light_list);
        }
    }

    ClusteredShading *getClusteredShading() const { return _clustered.get(); }
protected:
    osg::ref_ptr<ClusteredShading> _clustered;
};

class LightFinder : public osg::NodeVisitor {
public:
    LightFinder(const std::string &name) :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _name(name) {}
    virtual void apply(osg::Node &node) {
        // Only traverse the scene graph if we haven't found a light yet (or if
        // the one we found earlier is no longer valid).
        if (getLight().valid())
            return;

        if (node.getName() == _name) {
            osg::LightSource *light_source =
                dynamic_cast<osg::LightSource *>(&node);
            if (light_source)
                _light = light_source->getLight();
        }

        traverse(node);
    }
    osg::ref_ptr<osg::Light> getLight() const {
        osg::ref_ptr<osg::Light> light_ref;
        _light.lock(light_ref);
        return light_ref;
    }
protected:
    std::string _name;
    osg::observer_ptr<osg::Light> _light;
};

//------------------------------------------------------------------------------

Pass *
PassBuilder::build(Compositor *compositor, const SGPropertyNode *root,
                   const SGReaderWriterOptions *options)
{
    osg::ref_ptr<Pass> pass = new Pass;
    // The pass index matches its render order
    pass->render_order = root->getIndex();
    pass->name = root->getStringValue("name");
    if (pass->name.empty()) {
        SG_LOG(SG_INPUT, SG_WARN, "PassBuilder::build: Pass " << pass->render_order
               << " has no name. It won't be addressable by name!");
    }
    pass->type = root->getStringValue("type");
    pass->effect_scheme = root->getStringValue("effect-scheme");
    if (!EffectSchemeSingleton::instance()->is_valid_scheme(pass->effect_scheme, options)) {
        SG_LOG(SG_INPUT, SG_WARN, "PassBuilder::build: Pass " << pass->render_order << " ("
               << pass->name << ") uses unknown Effect scheme \"" << pass->effect_scheme << "\"");
    }
    pass->render_once = root->getBoolValue("render-once", false);

    const SGPropertyNode *p_render_condition = root->getChild("render-condition");
    if (p_render_condition)
        pass->render_condition = sgReadCondition(getPropertyRoot(), p_render_condition);

    osg::Camera *camera = new osg::Camera;
    pass->camera = camera;

    camera->setName(pass->name);
    camera->setGraphicsContext(compositor->getGraphicsContext());
    // Even though this camera will be added as a slave to the view, it will
    // always be updated manually in Compositor::update()
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    // Same with the projection matrix
    camera->setProjectionResizePolicy(osg::Camera::FIXED);
    // We only use POST_RENDER. Leave PRE_RENDER for Canvas and other RTT stuff
    // that doesn't involve the rendering pipeline itself. NESTED_RENDER is also
    // not a possibility since we don't want to share RenderStage with the View
    // master camera.
    camera->setRenderOrder(osg::Camera::POST_RENDER,
                           pass->render_order + compositor->getOrderOffset() * 100);
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    // XXX: Should we make this configurable?
    camera->setCullingMode(osg::CullSettings::SMALL_FEATURE_CULLING
                           | osg::CullSettings::VIEW_FRUSTUM_CULLING);

    osg::Node::NodeMask cull_mask =
        std::stoul(root->getStringValue("cull-mask", "0xffffffff"), nullptr, 0);
    pass->cull_mask = cull_mask;
    camera->setCullMask(pass->cull_mask);
    camera->setCullMaskLeft(pass->cull_mask & ~RIGHT_BIT);
    camera->setCullMaskRight(pass->cull_mask & ~LEFT_BIT);

    osg::Vec4f clear_color(0.0f, 0.0f, 0.0f, 1.0f);
    const SGPropertyNode *p_clear_color = root->getChild("clear-color");
    if (p_clear_color)
        clear_color = toOsg(p_clear_color->getValue<SGVec4d>());
    camera->setClearColor(clear_color);
    osg::Vec4f clear_accum(0.0f, 0.0f, 0.0f, 0.0f);
    const SGPropertyNode *p_clear_accum = root->getChild("clear-accum");
    if (p_clear_accum)
        clear_accum = toOsg(p_clear_accum->getValue<SGVec4d>());
    camera->setClearAccum(clear_accum);
    camera->setClearDepth(root->getFloatValue("clear-depth", 1.0f));
    camera->setClearStencil(root->getIntValue("clear-stencil", 0));

    GLbitfield clear_mask = 0;
    std::stringstream mask_ss;
    std::string mask_bit;
    // Do not clear by default
    mask_ss << root->getStringValue("clear-mask", "");
    while (mask_ss >> mask_bit) {
        if (mask_bit == "color")        clear_mask |= GL_COLOR_BUFFER_BIT;
        else if (mask_bit == "depth")   clear_mask |= GL_DEPTH_BUFFER_BIT;
        else if (mask_bit == "stencil") clear_mask |= GL_STENCIL_BUFFER_BIT;
    }
    camera->setClearMask(clear_mask);

    osg::DisplaySettings::ImplicitBufferAttachmentMask implicit_attachments = 0;
    std::stringstream att_ss;
    std::string att_bit;
    // No implicit attachments by default
    att_ss << root->getStringValue("implicit-attachment-mask", "");
    while (att_ss >> att_bit) {
        if (att_bit == "color")        implicit_attachments |= osg::DisplaySettings::IMPLICIT_COLOR_BUFFER_ATTACHMENT;
        else if (att_bit == "depth")   implicit_attachments |= osg::DisplaySettings::IMPLICIT_DEPTH_BUFFER_ATTACHMENT;
        else if (att_bit == "stencil") implicit_attachments |= osg::DisplaySettings::IMPLICIT_STENCIL_BUFFER_ATTACHMENT;
    }
#ifdef __APPLE__
    // MacOS doesn't like when we don't attach a color buffer, so add it if
    // it wasn't set.
    if ((implicit_attachments & osg::DisplaySettings::IMPLICIT_COLOR_BUFFER_ATTACHMENT) == 0) {
        implicit_attachments |= osg::DisplaySettings::IMPLICIT_COLOR_BUFFER_ATTACHMENT;
        SG_LOG(SG_INPUT, SG_INFO, "Compositor: MacOS fix: Implicit color buffer added to pass '"
               << pass->name << "'");
    }
#endif
    camera->setImplicitBufferAttachmentMask(implicit_attachments, implicit_attachments);

    // Set some global state
    camera->getOrCreateStateSet()->setMode(GL_TEXTURE_CUBE_MAP_SEAMLESS,
                                           osg::StateAttribute::ON);

    PropertyList p_shadow_passes = root->getChildren("use-shadow-pass");
    for (const auto &p_shadow_pass : p_shadow_passes) {
        std::string shadow_pass_name = p_shadow_pass->getStringValue();
        if (!shadow_pass_name.empty()) {
            Pass *shadow_pass = compositor->getPass(shadow_pass_name);
            if (shadow_pass) {
                CSMCullCallback *cullcb =
                    dynamic_cast<CSMCullCallback *>(
                        shadow_pass->camera->getCullCallback());
                if (cullcb) {
                    camera->getOrCreateStateSet()->addUniform(
                        cullcb->getLightMatrixUniform());
                } else {
                    SG_LOG(SG_INPUT, SG_WARN, "ScenePassBuilder::build: Pass '"
                           << shadow_pass_name << "' is not a shadow pass");
                }
            }
        }
    }

    PropertyList p_bindings = root->getChildren("binding");
    for (auto const &p_binding : p_bindings) {
        if (!checkConditional(p_binding))
            continue;
        try {
            std::string buffer_name = p_binding->getStringValue("buffer");
            if (buffer_name.empty())
                throw sg_exception("No buffer specified");

            Buffer *buffer = compositor->getBuffer(buffer_name);
            if (!buffer)
                throw sg_exception(std::string("Unknown buffer '") +
                                   buffer_name + "'");

            osg::Texture *texture = buffer->texture;

            int unit = p_binding->getIntValue("unit", -1);
            if (unit < 0)
                throw sg_exception("No texture unit specified");

            // Make the texture available to every child of the pass, overriding
            // existing units
            camera->getOrCreateStateSet()->setTextureAttributeAndModes(
                unit,
                texture,
                osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        } catch (sg_exception &e) {
            SG_LOG(SG_INPUT, SG_ALERT, "PassBuilder::build: Skipping binding "
                   << p_binding->getIndex() << " in pass " << pass->render_order
                   << ": " << e.what());
        }
    }

    // Image bindings (glBindImageTexture) require OpenGL 4.2
    PropertyList p_images = root->getChildren("image-binding");
    for (auto const& p_binding : p_images) {
        if (!checkConditional(p_binding))
            continue;
        try {
            std::string buffer_name = p_binding->getStringValue("buffer");
            if (buffer_name.empty())
                throw sg_exception("No buffer specified");

            Buffer* buffer = compositor->getBuffer(buffer_name);
            if (!buffer)
                throw sg_exception(std::string("Unknown buffer '") +
                                   buffer_name + "'");

            osg::Texture* texture = buffer->texture;

            int unit = p_binding->getIntValue("unit", -1);
            if (unit < 0)
                throw sg_exception("No image unit specified");

            osg::BindImageTexture::Access access;
            std::string accessStr = p_binding->getStringValue("access");
            if (accessStr.empty())
                throw sg_exception("No access specified");
            if (accessStr == "read-only")
                access = osg::BindImageTexture::READ_ONLY;
            else if (accessStr == "write-only")
                access = osg::BindImageTexture::WRITE_ONLY;
            else if (accessStr == "read-write")
                access = osg::BindImageTexture::READ_WRITE;
            else
                throw sg_exception(std::string("Unknown access '") +
                                   accessStr + "'");

            static const struct {
                const char* name;
                GLenum format;
            } formatNames[] = {
                // see glBindImageTexture(3G)
                { "rgba32f",        GL_RGBA32F_ARB },
                { "rgba16f",        GL_RGBA16F_ARB },
                { "rg32f",          GL_RG32F },
                { "rg16f",          GL_RG16F },
                //{ "r11f_g11f_b10f", GL_R11F_G11F_B10F },
                { "r32f",           GL_R32F },
                { "r16f",           GL_R16F },
                { "rgba32ui",       GL_RGBA32UI_EXT },
                { "rgba16ui",       GL_RGBA16UI_EXT },
                //{ "rgb10_a2ui",     GL_RGB10_A2UI },
                { "rgba8ui",        GL_RGBA8UI_EXT },
                { "rg32ui",         GL_RG32UI },
                { "rg16ui",         GL_RG16UI },
                { "rg8ui",          GL_RG8UI },
                { "r32ui",          GL_R32UI },
                { "r16ui",          GL_R16UI },
                { "r8ui",           GL_R8UI },
                { "rgba32i",        GL_RGBA32I_EXT },
                { "rgba16i",        GL_RGBA16I_EXT },
                { "rgba8i",         GL_RGBA8I_EXT },
                { "rg32i",          GL_RG32I },
                { "rg16i",          GL_RG16I },
                { "rg8i",           GL_RG8I },
                { "r32i",           GL_R32I },
                { "r16i",           GL_R16I },
                { "r8i",            GL_R8I },
                { "rgba16",         GL_RGBA16 },
                { "rgb10_a2",       GL_RGB10_A2 },
                { "rgba8",          GL_RGBA8 },
                { "rg16",           GL_RG16 },
                { "rg8",            GL_RG8 },
                { "r16",            GL_R16 },
                { "r8",             GL_R8 },
                { "rgba16_snorm",   GL_RGBA16_SNORM },
                { "rgba8_snorm",    GL_RGBA8_SNORM },
                { "rg16_snorm",     GL_RG16_SNORM },
                { "rg8_snorm",      GL_RG8_SNORM },
                { "r16_snorm",      GL_R16_SNORM },
                { "r8_snorm",       GL_R8_SNORM },
            };
            std::string formatStr = p_binding->getStringValue("format");
            if (formatStr.empty())
                throw sg_exception("No format specified");
            GLenum format = 0;
            for (auto& fmt : formatNames) {
                if (formatStr == fmt.name) {
                    format = fmt.format;
                    break;
                }
            }
            if (format == 0)
                throw sg_exception(std::string("Unknown format '") +
                                   formatStr + "'");

            // Make the image available to every child of the pass, overriding
            // existing units
            auto* binding = new osg::BindImageTexture(unit, texture, access,
                                                      format);
            camera->getOrCreateStateSet()->setAttributeAndModes(binding,
                osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        } catch (sg_exception &e) {
            SG_LOG(SG_INPUT, SG_ALERT, "PassBuilder::build: Skipping image binding "
                   << p_binding->getIndex() << " in pass " << pass->render_order
                   << ": " << e.what());
        }
    }

    PropertyList p_attachments = root->getChildren("attachment");
    if (pass->type == "compute") {
        // Compute shaders don't have fixed function read or draw buffers.
        // If we set FRAME_BUFFER, OSG will attempt to resize the viewport, but
        // if we set FRAME_BUFFER_OBJECT it will create an FBO.
        // Therefore we set FRAME_BUFFER, and clone the viewport so it won't
        // mess with the compositor viewport on window resize.
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER);
        camera->setDrawBuffer(GL_NONE);
        camera->setReadBuffer(GL_NONE);
        camera->setViewport(new osg::Viewport(*compositor->getViewport()));
    } else if (p_attachments.empty()) {
        // If there are no attachments, assume the pass is rendering
        // directly to the screen
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER);
        camera->setDrawBuffer(GL_BACK);
        camera->setReadBuffer(GL_BACK);

        // Use the physical viewport. We can't let the user choose the viewport
        // size because some parts of the window might not be ours.
        camera->setViewport(compositor->getViewport());
    } else {
        // This is a RTT camera
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

        bool viewport_absolute = false;
        // The index of the attachment to be used as the size of the viewport.
        // The one with index 0 is used by default.
        int viewport_attachment = 0;
        const SGPropertyNode *p_viewport = root->getChild("viewport");
        if (p_viewport) {
            // The user has manually specified a viewport size
            viewport_absolute = p_viewport->getBoolValue("absolute", false);
            if (viewport_absolute) {
                camera->setViewport(p_viewport->getIntValue("x"),
                                    p_viewport->getIntValue("y"),
                                    p_viewport->getIntValue("width"),
                                    p_viewport->getIntValue("height"));
            }
            viewport_attachment = p_viewport->getIntValue("use-attachment", 0);
            if (!root->getChild("attachment", viewport_attachment)) {
                // Let OSG manage the viewport automatically
                camera->setViewport(new osg::Viewport);
                SG_LOG(SG_INPUT, SG_WARN, "PassBuilder::build: Can't use attachment "
                       << viewport_attachment << " to resize the viewport");
            }
        }

        bool color_buffer_present = false;
        for (auto const &p_attachment : p_attachments) {
            if (!checkConditional(p_attachment))
                continue;
            try {
                std::string buffer_name = p_attachment->getStringValue("buffer");
                if (buffer_name.empty())
                    throw sg_exception("No buffer specified");

                Buffer *buffer = compositor->getBuffer(buffer_name);
                if (!buffer)
                    throw sg_exception(std::string("Unknown buffer '") +
                                       buffer_name + "'");

                osg::Texture *texture = buffer->texture;

                osg::Camera::BufferComponent component = osg::Camera::COLOR_BUFFER;
                findPropString(p_attachment, "component", component, buffer_component_map);
                switch(component) {
                case osg::Camera::COLOR_BUFFER:
                case osg::Camera::COLOR_BUFFER0:
                case osg::Camera::COLOR_BUFFER1:
                case osg::Camera::COLOR_BUFFER2:
                case osg::Camera::COLOR_BUFFER3:
                case osg::Camera::COLOR_BUFFER4:
                case osg::Camera::COLOR_BUFFER5:
                case osg::Camera::COLOR_BUFFER6:
                case osg::Camera::COLOR_BUFFER7:
                    color_buffer_present = true;
                    break;
                default:
                    break;
                };

                unsigned int level = p_attachment->getIntValue("level", 0);
                unsigned int face = p_attachment->getIntValue("face", 0);
                bool mipmap_generation =
                    p_attachment->getBoolValue("mipmap-generation", false);
                unsigned int multisample_samples =
                    p_attachment->getIntValue("multisample-samples", 0);
                unsigned int multisample_color_samples =
                    p_attachment->getIntValue("multisample-color-samples", 0);

                camera->attach(component,
                               texture,
                               level,
                               face,
                               mipmap_generation,
                               multisample_samples,
                               multisample_color_samples);

                float mipmap_resize_factor = 1.0f / pow(2.0f, float(level));
                if (!viewport_absolute &&
                    (p_attachment->getIndex() == viewport_attachment)) {
                    float rel_x      = p_viewport->getFloatValue("x",      0.0f);
                    float rel_y      = p_viewport->getFloatValue("y",      0.0f);
                    float rel_width  = p_viewport->getFloatValue("width",  1.0f)
                        * mipmap_resize_factor;
                    float rel_height = p_viewport->getFloatValue("height", 1.0f)
                        * mipmap_resize_factor;

                    auto assign_dim = [&](float rel_dim,
                                          float buffer_dim_scale,
                                          int texture_dim,
                                          int physical_viewport_dim,
                                          float &viewport_dim_scale) -> float {
                        if (buffer_dim_scale == 0.0f) {
                            viewport_dim_scale = 0.0f;
                            return rel_dim * texture_dim;
                        } else {
                            viewport_dim_scale = rel_dim * buffer_dim_scale;
                            return viewport_dim_scale * physical_viewport_dim;
                        }
                    };

                    float x = assign_dim(rel_x,
                                         buffer->width_scale,
                                         texture->getTextureWidth(),
                                         compositor->getViewport()->width(),
                                         pass->viewport_x_scale);
                    float y = assign_dim(rel_y,
                                         buffer->height_scale,
                                         texture->getTextureHeight(),
                                         compositor->getViewport()->height(),
                                         pass->viewport_y_scale);
                    float width  = assign_dim(rel_width,
                                              buffer->width_scale,
                                              texture->getTextureWidth(),
                                              compositor->getViewport()->width(),
                                              pass->viewport_width_scale);
                    float height = assign_dim(rel_height,
                                              buffer->height_scale,
                                              texture->getTextureHeight(),
                                              compositor->getViewport()->height(),
                                              pass->viewport_height_scale);

                    camera->setViewport(x, y, width, height);
                }
            } catch (sg_exception &e) {
                SG_LOG(SG_INPUT, SG_ALERT, "PassBuilder::build: Skipping attachment "
                       << p_attachment->getIndex() << " in pass " << pass->render_order
                       << ": " << e.what());
            }
        }

        // Explicitly let OpenGL know that there are no color buffers attached.
        // This is required on GL <4.2 contexts or the framebuffer will be
        // considered incomplete.
        if (!color_buffer_present) {
            camera->setDrawBuffer(GL_NONE);
            camera->setReadBuffer(GL_NONE);
        }
    }

    osg::Viewport *viewport = camera->getViewport();
    auto &uniforms = compositor->getBuiltinUniforms();
    uniforms[Compositor::SG_UNIFORM_VIEWPORT]->set(
        osg::Vec4f(viewport->x(),
                   viewport->y(),
                   viewport->width(),
                   viewport->height()));
    uniforms[Compositor::SG_UNIFORM_PIXEL_SIZE]->set(
        osg::Vec2f(1.0f / viewport->width(),
                   1.0f / viewport->height()));

    const SGPropertyNode* p_clustered = root->getChild("use-clustered-uniforms");
    if (p_clustered) {
        std::string clustered_pass_name = p_clustered->getStringValue("pass");
        if (!clustered_pass_name.empty()) {
            Pass* clustered_pass = compositor->getPass(clustered_pass_name);
            if (clustered_pass) {
                auto* cullcb = dynamic_cast<SceneCullCallback*>(
                    clustered_pass->camera->getCullCallback());
                if (cullcb) {
                    auto* clustered = cullcb->getClusteredShading();
                    if (clustered) {
                        clustered->exposeUniformsToPass(
                            camera,
                            p_clustered->getIntValue("clusters-bind-unit", 11),
                            p_clustered->getIntValue("indices-bind-unit", 12),
                            p_clustered->getIntValue("pointlights-bind-unit", 13),
                            p_clustered->getIntValue("spotlights-bind-unit", 14));
                    } else {
                        SG_LOG(SG_INPUT, SG_WARN, "PassBuilder::build: Pass '" << clustered_pass_name << "' does not contain a clustered shading node");
                    }
                } else {
                    SG_LOG(SG_INPUT, SG_WARN, "PassBuilder::build: Pass '" << clustered_pass_name << "' is not a scene pass");
                }
            } else {
                SG_LOG(SG_INPUT, SG_WARN, "PassBuilder::build: Pass '" << clustered_pass_name << "' not found");
            }
        }
    }

    return pass.release();
}

//------------------------------------------------------------------------------

struct QuadPassBuilder : public PassBuilder {
public:
    virtual Pass *build(Compositor *compositor, const SGPropertyNode *root,
                        const SGReaderWriterOptions *options) {
        osg::ref_ptr<Pass> pass = PassBuilder::build(compositor, root, options);
        pass->useMastersSceneData = false;

        osg::Camera *camera = pass->camera;
        camera->setAllowEventFocus(false);
        camera->setViewMatrix(osg::Matrix::identity());
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0, 1, 0, 1));

        float left = 0.0f, bottom = 0.0f, width = 1.0f, height = 1.0f, scale = 1.0f;
        const SGPropertyNode *p_geometry = root->getNode("geometry");
        if (p_geometry) {
            left   = p_geometry->getFloatValue("left",   left);
            bottom = p_geometry->getFloatValue("bottom", bottom);
            width  = p_geometry->getFloatValue("width",  width);
            height = p_geometry->getFloatValue("height", height);
            scale  = p_geometry->getFloatValue("scale",  scale);
        }

        osg::ref_ptr<EffectGeode> quad = new EffectGeode;
        camera->addChild(quad);
        quad->setCullingActive(false);

        const std::string eff_file = root->getStringValue("effect");
        if (!eff_file.empty()) {
            Effect *eff = makeEffect(eff_file, true, options);
            if (eff)
                quad->setEffect(eff);
        }

        osg::ref_ptr<osg::Geometry> geom = createFullscreenQuadGeom(
            left, bottom, width, height, scale);
        quad->addDrawable(geom);

        osg::ref_ptr<osg::StateSet> quad_state = quad->getOrCreateStateSet();
        quad_state->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF
                            | osg::StateAttribute::PROTECTED);

        osg::StateSet *ss = camera->getOrCreateStateSet();
        for (const auto &uniform : compositor->getBuiltinUniforms())
            ss->addUniform(uniform);

        return pass.release();
    }
protected:
    osg::Geometry *createFullscreenQuadGeom(float left,
                                            float bottom,
                                            float width,
                                            float height,
                                            float scale) {
        osg::Geometry *geom = new osg::Geometry;
        geom->setSupportsDisplayList(false);

        // When the quad is fullscreen, it can be optimized by using a
        // a fullscreen triangle instead of a quad to avoid discarding pixels
        // in the diagonal. If the desired geometry does not occupy the entire
        // viewport, this optimization does not occur and a normal quad is drawn
        // instead.
        if (left != 0.0f || bottom != 0.0f || width != 1.0f || height != 1.0f || scale != 1.0f) {
            osg::Vec3Array *vertices = new osg::Vec3Array(4);
            (*vertices)[0].set(left,       bottom+height, 0.0f);
            (*vertices)[1].set(left,       bottom,        0.0f);
            (*vertices)[2].set(left+width, bottom+height, 0.0f);
            (*vertices)[3].set(left+width, bottom,        0.0f);
            geom->setVertexArray(vertices);

            osg::Vec2Array *texcoords = new osg::Vec2Array(4);
            (*texcoords)[0].set(0.0f,  scale);
            (*texcoords)[1].set(0.0f,  0.0f);
            (*texcoords)[2].set(scale, scale);
            (*texcoords)[3].set(scale, 0.0f);
            geom->setTexCoordArray(0, texcoords);

            geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, 0, 4));
        } else {
            osg::Vec3Array *vertices = new osg::Vec3Array(3);
            (*vertices)[0].set(0.0f, 2.0f, 0.0f);
            (*vertices)[1].set(0.0f, 0.0f, 0.0f);
            (*vertices)[2].set(2.0f, 0.0f, 0.0f);
            geom->setVertexArray(vertices);

            osg::Vec2Array *texcoords = new osg::Vec2Array(3);
            (*texcoords)[0].set(0.0f, 2.0f);
            (*texcoords)[1].set(0.0f, 0.0f);
            (*texcoords)[2].set(2.0f, 0.0f);
            geom->setTexCoordArray(0, texcoords);

            geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3));
        }

        return geom;
    }
};
RegisterPassBuilder<QuadPassBuilder> registerQuadPass("quad");

//------------------------------------------------------------------------------

struct ComputePassBuilder : public PassBuilder {
public:
    virtual Pass* build(Compositor* compositor, const SGPropertyNode* root,
                        const SGReaderWriterOptions* options)
    {
        osg::ref_ptr<Pass> pass = PassBuilder::build(compositor, root, options);
        pass->useMastersSceneData = false;

        osg::Camera* camera = pass->camera;
        camera->setAllowEventFocus(false);

        osg::ref_ptr<EffectGeode> compute = new EffectGeode;
        camera->addChild(compute);
        compute->setCullingActive(false);

        const std::string eff_file = root->getStringValue("effect");
        if (!eff_file.empty()) {
            Effect* eff = makeEffect(eff_file, true, options);
            if (eff)
                compute->setEffect(eff);
        }

        static const char* dimNames[] = {"x", "y", "z"};
        static const char* dimScaleNames[] = {"x-screen-scale", "y-screen-scale", "z-screen-scale"};
        const osg::Viewport* vp = compositor->getViewport();
        osg::Vec3f screenSize(vp->width(), vp->height(), 1);
        // Get workgroup size (also defined in the shader)
        osg::Vec3i wgSize(1, 1, 1);
        const SGPropertyNode* workgroupSizeNode = root->getChild("workgroup-size");
        if (workgroupSizeNode) {
            for (int dim = 0; dim < 3; ++dim) {
                const SGPropertyNode* dimNode = workgroupSizeNode->getChild(dimNames[dim]);
                if (!dimNode)
                    continue;
                wgSize[dim] = dimNode->getIntValue(1);
                if (wgSize[dim] < 1)
                    wgSize[dim] = 1;
            }
        }
        pass->compute_wg_size.set(wgSize[0], wgSize[1]);

        // Get global size (will be divided by workgroup size and rounded up)
        osg::Vec3f globalSize(1.0f, 1.0f, 1.0f);
        const SGPropertyNode* globalSizeNode = root->getChild("global-size");
        if (globalSizeNode) {
            osg::Vec3f screenScale(1.0f, 1.0f, 1.0f);
            for (int dim = 0; dim < 3; ++dim) {
                const SGPropertyNode* scaleNode = globalSizeNode->getChild(dimScaleNames[dim]);
                if (scaleNode)
                    screenScale[dim] = scaleNode->getFloatValue();
            }
            pass->compute_global_scale.set(0.0f, 0.0f);
            for (int dim = 0; dim < 3; ++dim) {
                const SGPropertyNode* dimNode = globalSizeNode->getChild(dimNames[dim]);
                if (!dimNode)
                    continue;
                const std::string dimStr = dimNode->getStringValue();
                if (dimStr == "screen") {
                    // Compositor::resized() is responsible for updating this
                    // when the compositor viewport is resized.
                    globalSize[dim] = ceil(screenSize[dim] * screenScale[dim]);
                    if (dim < 2)
                        pass->compute_global_scale[dim] = screenScale[dim];
                } else {
                    globalSize[dim] = dimNode->getIntValue(1);
                }
            }
        }

        // Divide by workgroup size
        int wgCount[3];
        for (int dim = 0; dim < 3; ++dim) {
            wgCount[dim] = (int)ceil(globalSize[dim] / wgSize[dim]);
            if (wgCount[dim] < 1)
                wgCount[dim] = 1;
        }

        osg::ref_ptr<osg::Drawable> computeNode = new osg::DispatchCompute(
            wgCount[0], wgCount[1], wgCount[2]);
        compute->addDrawable(computeNode);
        pass->compute_node = computeNode;

        osg::StateSet* ss = camera->getOrCreateStateSet();
        for (const auto& uniform : compositor->getBuiltinUniforms())
            ss->addUniform(uniform);

        return pass.release();
    }
};
RegisterPassBuilder<ComputePassBuilder> registerComputePass("compute");

//------------------------------------------------------------------------------

struct CSMUpdateCallback : public Pass::PassUpdateCallback {
public:
    CSMUpdateCallback(CSMCullCallback *cull_callback,
                      const std::string &light_name,
                      bool render_at_night,
                      float near_m, float far_m,
                      int sm_width, int sm_height) :
        _cull_callback(cull_callback),
        _light_finder(new LightFinder(light_name)),
        _render_at_night(render_at_night),
        _near_m(near_m),
        _far_m(far_m) {
        _half_sm_size = osg::Vec2d((double)sm_width, (double)sm_height) * 0.5;
    }
    virtual void updatePass(Pass &pass,
                            const osg::Matrix &view_matrix,
                            const osg::Matrix &proj_matrix) {
        osg::Camera *camera = pass.camera;
        // Look for the light
        camera->accept(*_light_finder);
        osg::ref_ptr<osg::Light> light = _light_finder->getLight();
        if (!light) {
            // We could not find any light
            return;
        }
        osg::Vec4 light_pos = light->getPosition();
        if (light_pos.w() != 0.0) {
            // We only support directional light sources for now
            return;
        }
        osg::Vec3 light_dir =
            osg::Vec3(light_pos.x(), light_pos.y(), light_pos.z());

        // The light direction we've just queried is from the previous frame.
        // This is because the position of the osg::LightSource gets updated
        // during the update traversal, and this function happens before that
        // in the SubsystemMgr update.
        // This is not a problem though (for now).

        osg::Matrix view_inverse = osg::Matrix::inverse(view_matrix);
        _cull_callback->setRealInverseViewMatrix(view_inverse);

        if (!_render_at_night) {
            osg::Vec3 camera_pos = osg::Vec3(0.0f, 0.0f, 0.0f) * view_inverse;
            camera_pos.normalize();
            float cos_light_angle = camera_pos * light_dir;
            if (cos_light_angle < -0.1) {
                // Night
                camera->setCullMask(0);
            } else {
                // Day
                camera->setCullMask(pass.cull_mask);
            }
        }

        // Calculate the light's point of view transformation matrices.
        // Taken from Project Rembrandt.
        double left = 0.0, right = 0.0, bottom = 0.0, top = 0.0,
            zNear = 0.0, zFar = 0.0;
        proj_matrix.getFrustum(left, right, bottom, top, zNear, zFar);

        osg::BoundingSphere bs;
        bs.expandBy(osg::Vec3(left,  bottom, -zNear) * (_near_m / zNear));
        bs.expandBy(osg::Vec3(right, top,    -zNear) * (_far_m  / zNear));
        bs.expandBy(osg::Vec3(left,  bottom, -zNear) * (_far_m  / zNear));
        bs.expandBy(osg::Vec3(right, top,    -zNear) * (_near_m / zNear));

        osg::Vec4d aim4 = osg::Vec4d(bs.center(), 1.0) * view_inverse;
        osg::Vec3d aim(aim4.x(), aim4.y(), aim4.z());

        osg::Matrixd &light_view_matrix = camera->getViewMatrix();
        light_view_matrix.makeLookAt(
            aim + light_dir * (bs.radius() + 100.f),
            aim,
            osg::Vec3(0.0f, 0.0f, 1.0f));

        osg::Matrixd &light_proj_matrix = camera->getProjectionMatrix();
        light_proj_matrix.makeOrtho(
            -bs.radius(), bs.radius(),
            -bs.radius(), bs.radius(),
            1.f, bs.radius() * 6.f + 100.f);

        // Do texel snapping to prevent flickering or shimmering.
        // We are using double precision vectors and matrices because in FG
        // world coordinates are relative to the center of the Earth, which can
        // (and will) cause precision issues due to their magnitude.
        osg::Vec4d shadow_origin4 = osg::Vec4d(0.0, 0.0, 0.0, 1.0) *
            light_view_matrix * light_proj_matrix;
        osg::Vec2d shadow_origin(shadow_origin4.x(), shadow_origin4.y());
        shadow_origin = osg::Vec2d(shadow_origin.x() * _half_sm_size.x(),
                                   shadow_origin.y() * _half_sm_size.y());
        osg::Vec2d rounded_origin(std::floor(shadow_origin.x()),
                                  std::floor(shadow_origin.y()));
        osg::Vec2d rounding = rounded_origin - shadow_origin;
        rounding = osg::Vec2d(rounding.x() / _half_sm_size.x(),
                              rounding.y() / _half_sm_size.y());

        osg::Matrixd round_matrix = osg::Matrixd::translate(
            rounding.x(), rounding.y(), 0.0);
        light_proj_matrix *= round_matrix;
    }

protected:
    osg::observer_ptr<CSMCullCallback> _cull_callback;
    osg::ref_ptr<LightFinder>     _light_finder;
    bool                          _render_at_night;
    float                         _near_m;
    float                         _far_m;
    osg::Vec2d                    _half_sm_size;
};

struct CSMPassBuilder : public PassBuilder {
    virtual Pass *build(Compositor *compositor, const SGPropertyNode *root,
                        const SGReaderWriterOptions *options) {
        osg::ref_ptr<Pass> pass = PassBuilder::build(compositor, root, options);

        osg::Camera *camera = pass->camera;
        camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF_INHERIT_VIEWPOINT);
        camera->setCullingMode(osg::CullSettings::ENABLE_ALL_CULLING);
        //camera->setComputeNearFarMode(
        //    osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);

        CSMCullCallback *cull_callback = new CSMCullCallback(pass->name);
        camera->setCullCallback(cull_callback);

        // Use the Sun as the default light source
        std::string light_name = root->getStringValue("light-name", "FGLightSource");
        bool render_at_night = root->getBoolValue("render-at-night", true);
        float near_m = root->getFloatValue("near-m");
        float far_m  = root->getFloatValue("far-m");
        int sm_width  = camera->getViewport()->width();
        int sm_height = camera->getViewport()->height();
        pass->update_callback = new CSMUpdateCallback(
            cull_callback,
            light_name,
            render_at_night,
            near_m, far_m,
            sm_width, sm_height);

        return pass.release();
    }
};
RegisterPassBuilder<CSMPassBuilder> registerCSMPass("csm");

//------------------------------------------------------------------------------

class SceneUpdateCallback : public Pass::PassUpdateCallback {
public:
    SceneUpdateCallback(int cubemap_face, double zNear, double zFar) :
        _cubemap_face(cubemap_face),
        _zNear(zNear),
        _zFar(zFar) {}

    virtual void updatePass(Pass &pass,
                            const osg::Matrix &view_matrix,
                            const osg::Matrix &proj_matrix) {
        osg::Camera *camera = pass.camera;

        double left = 0.0, right = 0.0, bottom = 0.0, top = 0.0,
            znear = 0.0, zfar = 0.0;
        proj_matrix.getFrustum(left, right, bottom, top, znear, zfar);

        osg::Matrixd given_proj_matrix = proj_matrix;
        osg::Matrixd new_proj_matrix = given_proj_matrix;
        if (_zNear != 0.0 || _zFar != 0.0) {
            if (_zNear != 0.0) znear = _zNear;
            if (_zFar  != 0.0) zfar  = _zFar;
            ProjectionMatrix::clampNearFarPlanes(given_proj_matrix, znear, zfar,
                                                 new_proj_matrix);
        }

        if (_cubemap_face < 0) {
            camera->setViewMatrix(view_matrix);
            camera->setProjectionMatrix(new_proj_matrix);
        } else {
            osg::Vec4d camera_pos4 = osg::Vec4d(0.0, 0.0, 0.0, 1.0) *
                osg::Matrixd::inverse(view_matrix);
            osg::Vec3d camera_pos(camera_pos4.x(), camera_pos4.y(), camera_pos4.z());

            typedef std::pair<osg::Vec3d, osg::Vec3d> CubemapFace;
            const CubemapFace id[] = {
                CubemapFace(osg::Vec3d( 1.0,  0.0,  0.0), osg::Vec3d( 0.0, -1.0,  0.0)), // +X
                CubemapFace(osg::Vec3d(-1.0,  0.0,  0.0), osg::Vec3d( 0.0, -1.0,  0.0)), // -X
                CubemapFace(osg::Vec3d( 0.0,  1.0,  0.0), osg::Vec3d( 0.0,  0.0,  1.0)), // +Y
                CubemapFace(osg::Vec3d( 0.0, -1.0,  0.0), osg::Vec3d( 0.0,  0.0, -1.0)), // -Y
                CubemapFace(osg::Vec3d( 0.0,  0.0,  1.0), osg::Vec3d( 0.0, -1.0,  0.0)), // +Z
                CubemapFace(osg::Vec3d( 0.0,  0.0, -1.0), osg::Vec3d( 0.0, -1.0,  0.0))  // -Z
            };

            osg::Matrixd cubemap_view_matrix;
            cubemap_view_matrix.makeLookAt(camera_pos,
                                           camera_pos + id[_cubemap_face].first,
                                           id[_cubemap_face].second);
            camera->setViewMatrix(cubemap_view_matrix);
            camera->setProjectionMatrixAsPerspective(90.0, 1.0, znear, zfar);
        }
    }
protected:
    int _cubemap_face;
    double _zNear;
    double _zFar;
};

struct ScenePassBuilder : public PassBuilder {
public:
    virtual Pass *build(Compositor *compositor, const SGPropertyNode *root,
                        const SGReaderWriterOptions *options) {
        osg::ref_ptr<Pass> pass = PassBuilder::build(compositor, root, options);
        pass->inherit_cull_mask = true;

        osg::Camera *camera = pass->camera;
        camera->setAllowEventFocus(true);

        const SGPropertyNode *p_lod_scale = root->getNode("lod-scale");
        if (p_lod_scale)
            camera->setLODScale(p_lod_scale->getFloatValue());

        const SGPropertyNode *p_clustered = root->getNode("clustered-shading");
        ClusteredShading *clustered = nullptr;
        if (p_clustered) {
            if (checkConditional(p_clustered)) {
                clustered = new ClusteredShading(camera, p_clustered);
                pass->collect_lights = true;
            }
        }

        camera->setCullCallback(new SceneCullCallback(clustered));

        int cubemap_face = root->getIntValue("cubemap-face", -1);
        float zNear = root->getFloatValue("z-near", 0.0f);
        float zFar  = root->getFloatValue("z-far",  0.0f);
        pass->update_callback = new SceneUpdateCallback(cubemap_face, zNear, zFar);

        osg::StateSet *ss = camera->getOrCreateStateSet();
        auto &uniforms = compositor->getBuiltinUniforms();
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_TEXTURE_MATRIX]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_VIEWPORT]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_CAMERA_POSITION_CART]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_CAMERA_POSITION_GEOD]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_CAMERA_DISTANCE_TO_EARTH_CENTER]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_CAMERA_WORLD_UP]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_CAMERA_VIEW_UP]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_FCOEF]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_FOV_SCALE]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_SUN_DIRECTION]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_SUN_DIRECTION_WORLD]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_SUN_ZENITH_COSTHETA]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_MOON_DIRECTION]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_MOON_DIRECTION_WORLD]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_MOON_ZENITH_COSTHETA]);
        ss->addUniform(uniforms[Compositor::SG_UNIFORM_EARTH_RADIUS]);

        osg::ref_ptr<osg::Uniform> clustered_shading_enabled =
            new osg::Uniform("fg_ClusteredEnabled", clustered ? true : false);
        ss->addUniform(clustered_shading_enabled);

        return pass.release();
    }
};

RegisterPassBuilder<ScenePassBuilder> registerScenePass("scene");

//------------------------------------------------------------------------------

Pass *
buildPass(Compositor *compositor, const SGPropertyNode *root,
          const SGReaderWriterOptions *options)
{
    std::string type = root->getStringValue("type");
    if (type.empty()) {
        SG_LOG(SG_INPUT, SG_ALERT, "buildPass: Unspecified pass type");
        return 0;
    }
    PassBuilder *builder = PassBuilder::find(type);
    if (!builder) {
        SG_LOG(SG_INPUT, SG_ALERT, "buildPass: Unknown pass type '"
               << type << "'");
        return 0;
    }

    return builder->build(compositor, root, options);
}

} // namespace compositor
} // namespace simgear
