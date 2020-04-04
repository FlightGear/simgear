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

#include "CompositorPass.hxx"

#include <osg/Depth>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PolygonMode>
#include <osg/io_utils>

#include <osgUtil/CullVisitor>

#include <simgear/props/vectorPropTemplates.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/OsgMath.hxx>
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
    camera->setRenderOrder(osg::Camera::POST_RENDER, pass->render_order * 10);
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    // XXX: Should we make this configurable?
    camera->setCullingMode(osg::CullSettings::SMALL_FEATURE_CULLING
                           | osg::CullSettings::VIEW_FRUSTUM_CULLING);

    osg::Node::NodeMask cull_mask =
        std::stoul(root->getStringValue("cull-mask", "0xffffffff"), nullptr, 0);
    pass->cull_mask = cull_mask;
    camera->setCullMask(pass->cull_mask);
    camera->setCullMaskLeft(pass->cull_mask);
    camera->setCullMaskRight(pass->cull_mask);

    osg::Vec4f clear_color(0.0f, 0.0f, 0.0f, 0.0f);
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
    std::stringstream ss;
    std::string bit;
    // Default clear mask as in OSG
    ss << root->getStringValue("clear-mask", "color depth");
    while (ss >> bit) {
        if (bit == "color")        clear_mask |= GL_COLOR_BUFFER_BIT;
        else if (bit == "depth")   clear_mask |= GL_DEPTH_BUFFER_BIT;
        else if (bit == "stencil") clear_mask |= GL_STENCIL_BUFFER_BIT;
    }
    camera->setClearMask(clear_mask);

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

    PropertyList p_attachments = root->getChildren("attachment");
    if (p_attachments.empty()) {
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

                if (!viewport_absolute &&
                    (p_attachment->getIndex() == viewport_attachment)) {
                    if ((buffer->width_scale  == 0.0f) &&
                        (buffer->height_scale == 0.0f)) {
                        // This is a fixed size pass. We allow the user to use
                        // relative coordinates to shape the viewport.
                        float x      = p_viewport->getFloatValue("x",      0.0f);
                        float y      = p_viewport->getFloatValue("y",      0.0f);
                        float width  = p_viewport->getFloatValue("width",  1.0f);
                        float height = p_viewport->getFloatValue("height", 1.0f);
                        camera->setViewport(x      * texture->getTextureWidth(),
                                            y      * texture->getTextureHeight(),
                                            width  * texture->getTextureWidth(),
                                            height * texture->getTextureHeight());
                    } else {
                        // This is a pass that should match the physical viewport
                        // size. Store the scales so we can resize the pass later
                        // if the physical viewport changes size.
                        pass->viewport_width_scale  = buffer->width_scale;
                        pass->viewport_height_scale = buffer->height_scale;
                        camera->setViewport(
                            0,
                            0,
                            buffer->width_scale  * compositor->getViewport()->width(),
                            buffer->height_scale * compositor->getViewport()->height());
                    }
                }
            } catch (sg_exception &e) {
                SG_LOG(SG_INPUT, SG_ALERT, "PassBuilder::build: Skipping attachment "
                       << p_attachment->getIndex() << " in pass " << pass->render_order
                       << ": " << e.what());
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
        int values = osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED;
        quad_state->setAttribute(new osg::PolygonMode(
                                     osg::PolygonMode::FRONT_AND_BACK,
                                     osg::PolygonMode::FILL),
                                 values);
        quad_state->setMode(GL_LIGHTING,   values);
        quad_state->setMode(GL_DEPTH_TEST, values);

        osg::StateSet *ss = camera->getOrCreateStateSet();
        for (const auto &uniform : compositor->getUniforms())
            ss->addUniform(uniform);

        return pass.release();
    }
protected:
    osg::Geometry *createFullscreenQuadGeom(float left,
                                            float bottom,
                                            float width,
                                            float height,
                                            float scale) {
        osg::ref_ptr<osg::Geometry> geom;

        // When the quad is fullscreen, it can be optimized by using a
        // a fullscreen triangle instead of a quad to avoid discarding pixels
        // in the diagonal. If the desired geometry does not occupy the entire
        // viewport, this optimization does not occur and a normal quad is drawn
        // instead.
        if (left != 0.0f || bottom != 0.0f || width != 1.0f || height != 1.0f
            || scale != 1.0f) {
            geom = osg::createTexturedQuadGeometry(
                osg::Vec3(left,  bottom,  0.0f),
                osg::Vec3(width, 0.0f,    0.0f),
                osg::Vec3(0.0f,  height,  0.0f),
                0.0f, 0.0f, scale, scale);
        } else {
            geom = new osg::Geometry;

            osg::Vec3Array *coords = new osg::Vec3Array(3);
            (*coords)[0].set(0.0f, 2.0f, 0.0f);
            (*coords)[1].set(0.0f, 0.0f, 0.0f);
            (*coords)[2].set(2.0f, 0.0f, 0.0f);
            geom->setVertexArray(coords);

            osg::Vec2Array *tcoords = new osg::Vec2Array(3);
            (*tcoords)[0].set(0.0f, 2.0f);
            (*tcoords)[1].set(0.0f, 0.0f);
            (*tcoords)[2].set(2.0f, 0.0f);
            geom->setTexCoordArray(0, tcoords);

            osg::Vec4Array *colours = new osg::Vec4Array(1);
            (*colours)[0].set(1.0f, 1.0f, 1.0, 1.0f);
            geom->setColorArray(colours, osg::Array::BIND_OVERALL);

            osg::Vec3Array *normals = new osg::Vec3Array(1);
            (*normals)[0].set(0.0f, 0.0f, 1.0f);
            geom->setNormalArray(normals, osg::Array::BIND_OVERALL);

            geom->addPrimitiveSet(new osg::DrawArrays(
                                      osg::PrimitiveSet::TRIANGLES, 0, 3));
        }

        return geom.release();
    }
};
RegisterPassBuilder<QuadPassBuilder> registerQuadPass("quad");

//------------------------------------------------------------------------------

class ShadowMapCullCallback : public osg::NodeCallback {
public:
    ShadowMapCullCallback(const std::string &suffix) {
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

struct ShadowMapUpdateCallback : public Pass::PassUpdateCallback {
public:
    ShadowMapUpdateCallback(ShadowMapCullCallback *cull_callback,
                            const std::string &light_name,
                            float near_m, float far_m,
                            int sm_width, int sm_height) :
        _cull_callback(cull_callback),
        _light_finder(new LightFinder(light_name)),
        _near_m(near_m),
        _far_m(far_m) {
        _half_sm_size = osg::Vec2d((double)sm_width, (double)sm_height) / 2.0;
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

        // Calculate the light's point of view transformation matrices.
        // Taken from Project Rembrandt.
        double left, right, bottom, top, zNear, zFar;
        proj_matrix.getFrustum(left, right, bottom, top, zNear, zFar);

        osg::BoundingSphere bs;
        bs.expandBy(osg::Vec3(left,  bottom, -zNear) * (_near_m / zNear));
        bs.expandBy(osg::Vec3(right, top,    -zNear) * (_far_m  / zNear));
        bs.expandBy(osg::Vec3(left,  bottom, -zNear) * (_far_m  / zNear));
        bs.expandBy(osg::Vec3(right, top,    -zNear) * (_near_m / zNear));

        osg::Vec4 aim4 = osg::Vec4(bs.center(), 1.0) * view_inverse;
        osg::Vec3 aim(aim4.x(), aim4.y(), aim4.z());

        osg::Matrixd &light_view_matrix = camera->getViewMatrix();
        light_view_matrix.makeLookAt(
            aim + light_dir * (bs.radius() + 10.0f),
            aim,
            osg::Vec3(0.0f, 0.0f, 1.0f));

        osg::Matrixd &light_proj_matrix = camera->getProjectionMatrix();
        light_proj_matrix.makeOrtho(
            -bs.radius(), bs.radius(),
            -bs.radius(), bs.radius(),
            1.0, bs.radius() * 10.0);

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
    osg::observer_ptr<ShadowMapCullCallback> _cull_callback;
    osg::ref_ptr<LightFinder>     _light_finder;
    float                         _near_m;
    float                         _far_m;
    osg::Vec2d                    _half_sm_size;
};

struct ShadowMapPassBuilder : public PassBuilder {
    virtual Pass *build(Compositor *compositor, const SGPropertyNode *root,
                        const SGReaderWriterOptions *options) {
        osg::ref_ptr<Pass> pass = PassBuilder::build(compositor, root, options);

        osg::Camera *camera = pass->camera;
        camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF_INHERIT_VIEWPOINT);
        camera->setCullingMode(osg::CullSettings::ENABLE_ALL_CULLING);
        //camera->setComputeNearFarMode(
        //    osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);

        ShadowMapCullCallback *cull_callback = new ShadowMapCullCallback(pass->name);
        camera->setCullCallback(cull_callback);

        std::string light_name = root->getStringValue("light-name");
        float near_m = root->getFloatValue("near-m");
        float far_m  = root->getFloatValue("far-m");
        int sm_width  = camera->getViewport()->width();
        int sm_height = camera->getViewport()->height();
        pass->update_callback = new ShadowMapUpdateCallback(
            cull_callback,
            light_name,
            near_m, far_m,
            sm_width, sm_height);

        return pass.release();
    }
};
RegisterPassBuilder<ShadowMapPassBuilder> registerShadowMapPass("shadow-map");

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
        if (_cubemap_face < 0) {
            camera->setViewMatrix(view_matrix);
            camera->setProjectionMatrix(proj_matrix);
        } else {
            osg::Vec3 camera_pos = osg::Vec3(0.0, 0.0, 0.0) *
                osg::Matrix::inverse(view_matrix);

            typedef std::pair<osg::Vec3, osg::Vec3> CubemapFace;
            const CubemapFace id[] = {
                CubemapFace(osg::Vec3( 1,  0,  0), osg::Vec3( 0, -1,  0)), // +X
                CubemapFace(osg::Vec3(-1,  0,  0), osg::Vec3( 0, -1,  0)), // -X
                CubemapFace(osg::Vec3( 0,  1,  0), osg::Vec3( 0,  0,  1)), // +Y
                CubemapFace(osg::Vec3( 0, -1,  0), osg::Vec3( 0,  0, -1)), // -Y
                CubemapFace(osg::Vec3( 0,  0,  1), osg::Vec3( 0, -1,  0)), // +Z
                CubemapFace(osg::Vec3( 0,  0, -1), osg::Vec3( 0, -1,  0))  // -Z
            };

            osg::Matrix cubemap_view_matrix;
            cubemap_view_matrix.makeLookAt(camera_pos,
                                           camera_pos + id[_cubemap_face].first,
                                           camera_pos + id[_cubemap_face].second);
            camera->setViewMatrix(cubemap_view_matrix);
            camera->setProjectionMatrixAsFrustum(-1.0, 1.0, -1.0, 1.0,
                                                 1.0, 10000.0);
        }

        if (_zNear != 0.0 || _zFar != 0.0) {
            osg::Matrixd given_proj = camera->getProjectionMatrix();
            double left, right, bottom, top, znear, zfar;
            given_proj.getFrustum(left, right, bottom, top, znear, zfar);
            if (_zNear != 0.0) znear = _zNear;
            if (_zFar  != 0.0) zfar  = _zFar;
            osg::Matrixd new_proj;
            makeNewProjMat(given_proj, znear, zfar, new_proj);
            camera->setProjectionMatrix(new_proj);
        }
    }
protected:
    // Given a projection matrix, return a new one with the same frustum
    // sides and new near / far values.
    void makeNewProjMat(osg::Matrixd& oldProj, double znear,
                        double zfar, osg::Matrixd& projection) {
        projection = oldProj;
        // Slightly inflate the near & far planes to avoid objects at the
        // extremes being clipped out.
        znear *= 0.999;
        zfar *= 1.001;

        // Clamp the projection matrix z values to the range (near, far)
        double epsilon = 1.0e-6;
        if (fabs(projection(0,3)) < epsilon &&
            fabs(projection(1,3)) < epsilon &&
            fabs(projection(2,3)) < epsilon) {
            // Projection is Orthographic
            epsilon = -1.0/(zfar - znear); // Used as a temp variable
            projection(2,2) = 2.0*epsilon;
            projection(3,2) = (zfar + znear)*epsilon;
        } else {
            // Projection is Perspective
            double trans_near = (-znear*projection(2,2) + projection(3,2)) /
                (-znear*projection(2,3) + projection(3,3));
            double trans_far = (-zfar*projection(2,2) + projection(3,2)) /
                (-zfar*projection(2,3) + projection(3,3));
            double ratio = fabs(2.0/(trans_near - trans_far));
            double center = -0.5*(trans_near + trans_far);

            projection.postMult(osg::Matrixd(1.0, 0.0, 0.0, 0.0,
                                             0.0, 1.0, 0.0, 0.0,
                                             0.0, 0.0, ratio, 0.0,
                                             0.0, 0.0, center*ratio, 1.0));
        }
    }

    int _cubemap_face;
    double _zNear;
    double _zFar;
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
protected:
    osg::ref_ptr<ClusteredShading> _clustered;
};

struct ScenePassBuilder : public PassBuilder {
public:
    virtual Pass *build(Compositor *compositor, const SGPropertyNode *root,
                        const SGReaderWriterOptions *options) {
        osg::ref_ptr<Pass> pass = PassBuilder::build(compositor, root, options);
        pass->inherit_cull_mask = true;

        osg::Camera *camera = pass->camera;
        camera->setAllowEventFocus(true);

        const SGPropertyNode *p_clustered = root->getNode("clustered-shading");
        ClusteredShading *clustered = 0;
        if (p_clustered)
            clustered = new ClusteredShading(camera, p_clustered);

        camera->setCullCallback(new SceneCullCallback(clustered));

        int cubemap_face = root->getIntValue("cubemap-face", -1);
        float zNear = root->getFloatValue("z-near", 0.0f);
        float zFar  = root->getFloatValue("z-far",  0.0f);
        pass->update_callback = new SceneUpdateCallback(cubemap_face, zNear, zFar);

        PropertyList p_shadow_passes = root->getChildren("use-shadow-pass");
        for (const auto &p_shadow_pass : p_shadow_passes) {
            std::string shadow_pass_name = p_shadow_pass->getStringValue();
            if (!shadow_pass_name.empty()) {
                Pass *shadow_pass = compositor->getPass(shadow_pass_name);
                if (shadow_pass) {
                    ShadowMapCullCallback *cullcb =
                        dynamic_cast<ShadowMapCullCallback *>(
                            shadow_pass->camera->getCullCallback());
                    if (cullcb) {
                        camera->getOrCreateStateSet()->addUniform(
                            cullcb->getLightMatrixUniform());
                    } else {
                        SG_LOG(SG_INPUT, SG_WARN, "ScenePassBuilder::build: Pass '"
                               << shadow_pass_name << "is not a shadow pass");
                    }
                }
            }
        }

        osg::StateSet *ss = camera->getOrCreateStateSet();
        auto &uniforms = compositor->getUniforms();
        ss->addUniform(uniforms[Compositor::FCOEF]);

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
