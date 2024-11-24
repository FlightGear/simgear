// Copyright (C) 2018 - 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "Compositor.hxx"

#include <algorithm>

#include <osg/DispatchCompute>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture2DMultisample>
#include <osg/Texture3D>
#include <osg/TextureRectangle>
#include <osg/TextureCubeMap>
#include <osgUtil/IntersectionVisitor>

#include <simgear/math/SGRect.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/structure/exception.hxx>

#include "CompositorUtil.hxx"

class SunDirectionWorldCallback : public osg::Uniform::Callback {
public:
    virtual void operator()(osg::Uniform *uniform, osg::NodeVisitor *nv) {
        assert(dynamic_cast<SGUpdateVisitor*>(nv));
        SGUpdateVisitor* uv = static_cast<SGUpdateVisitor*>(nv);
        osg::Vec3f l = toOsg(uv->getLightDirection());
        l.normalize();
        uniform->set(l);
    }
};

class MoonDirectionWorldCallback : public osg::Uniform::Callback {
public:
    virtual void operator()(osg::Uniform *uniform, osg::NodeVisitor *nv) {
        assert(dynamic_cast<SGUpdateVisitor*>(nv));
        SGUpdateVisitor* uv = static_cast<SGUpdateVisitor*>(nv);
        osg::Vec3f l = toOsg(uv->getSecondLightDirection());
        l.normalize();
        uniform->set(l);
    }
};

namespace simgear {
namespace compositor {

int Compositor::_order_offset = 0;

Compositor *
Compositor::create(osg::View *view,
                   osg::GraphicsContext *gc,
                   osg::Viewport *viewport,
                   const SGPropertyNode *property_list,
                   const SGReaderWriterOptions *options)
{
    Compositor *compositor = new Compositor(view, gc, viewport);
    compositor->_name = property_list->getStringValue("name");

    gc->getState()->setUseModelViewAndProjectionUniforms(
        property_list->getBoolValue("use-osg-uniforms", false));
    gc->getState()->setUseVertexAttributeAliasing(
        property_list->getBoolValue("use-vertex-attribute-aliasing", false));

    // Read all buffers first so passes can use them
    PropertyList p_buffers = property_list->getChildren("buffer");
    for (auto const &p_buffer : p_buffers) {
        if (!checkConditional(p_buffer))
            continue;
        const std::string &buffer_name = p_buffer->getStringValue("name");
        if (buffer_name.empty()) {
            SG_LOG(SG_INPUT, SG_ALERT, "Compositor::build: Buffer requires "
                   "a name to be available to passes. Skipping...");
            continue;
        }
        Buffer *buffer = buildBuffer(compositor, p_buffer, options);
        if (buffer)
            compositor->addBuffer(buffer_name, buffer);
    }
    // Read passes
    PropertyList p_passes = property_list->getChildren("pass");
    for (auto const &p_pass : p_passes) {
        if (!checkConditional(p_pass))
            continue;
        Pass *pass = buildPass(compositor, p_pass, options);
        if (pass)
            compositor->addPass(pass);
    }

    ++_order_offset;

    return compositor;
}

Compositor *
Compositor::create(osg::View *view,
                   osg::GraphicsContext *gc,
                   osg::Viewport *viewport,
                   const std::string &name,
                   const SGReaderWriterOptions *options)
{
    std::string filename(name);
    filename += ".xml";
    std::string abs_filename = SGModelLib::findDataFile(filename);
    if (abs_filename.empty()) {
        SG_LOG(SG_INPUT, SG_ALERT, "Compositor::build: Could not find file '"
               << filename << "'");
        return 0;
    }

    SGPropertyNode_ptr property_list = new SGPropertyNode;
    try {
        readProperties(abs_filename, property_list.ptr(), 0, true);
    } catch (sg_io_exception &e) {
        SG_LOG(SG_INPUT, SG_ALERT, "Compositor::build: Failed to parse file '"
               << abs_filename << "'. " << e.getFormattedMessage());
        return 0;
    }

    return create(view, gc, viewport, property_list, options);
}

Compositor::Compositor(osg::View *view,
                       osg::GraphicsContext *gc,
                       osg::Viewport *viewport) :
    _view(view),
    _gc(gc),
    _viewport(viewport),
    _uniforms{
    new osg::Uniform("fg_TextureMatrix", osg::Matrixf()),
    new osg::Uniform("fg_Viewport", osg::Vec4f()),
    new osg::Uniform("fg_PixelSize", osg::Vec2f()),
    new osg::Uniform("fg_AspectRatio", 0.0f),
    new osg::Uniform("fg_ViewMatrix", osg::Matrixf()),
    new osg::Uniform("fg_ViewMatrixInverse", osg::Matrixf()),
    new osg::Uniform("fg_ProjectionMatrix", osg::Matrixf()),
    new osg::Uniform("fg_ProjectionMatrixInverse", osg::Matrixf()),
    new osg::Uniform("fg_PrevViewMatrix", osg::Matrixf()),
    new osg::Uniform("fg_PrevViewMatrixInverse", osg::Matrixf()),
    new osg::Uniform("fg_PrevProjectionMatrix", osg::Matrixf()),
    new osg::Uniform("fg_PrevProjectionMatrixInverse", osg::Matrixf()),
    new osg::Uniform("fg_CameraPositionCart", osg::Vec3f()),
    new osg::Uniform("fg_CameraPositionGeod", osg::Vec3f()),
    new osg::Uniform("fg_CameraDistanceToEarthCenter", 0.0f),
    new osg::Uniform("fg_CameraWorldUp", osg::Vec3f()),
    new osg::Uniform("fg_CameraViewUp", osg::Vec3f()),
    new osg::Uniform("fg_NearFar", osg::Vec2f()),
    new osg::Uniform("fg_Fcoef", 0.0f),
    new osg::Uniform("fg_FOVScale", osg::Vec2f()),
    new osg::Uniform("fg_SunDirection", osg::Vec3f()),
    new osg::Uniform("fg_SunDirectionWorld", osg::Vec3f()),
    new osg::Uniform("fg_SunZenithCosTheta", 0.0f),
    new osg::Uniform("fg_MoonDirection", osg::Vec3f()),
    new osg::Uniform("fg_MoonDirectionWorld", osg::Vec3f()),
    new osg::Uniform("fg_MoonZenithCosTheta", 0.0f),
    new osg::Uniform("fg_EarthRadius", 0.0f),
    }
{
    _uniforms[SG_UNIFORM_SUN_DIRECTION_WORLD]->setUpdateCallback(
        new SunDirectionWorldCallback);
    _uniforms[SG_UNIFORM_MOON_DIRECTION_WORLD]->setUpdateCallback(
        new MoonDirectionWorldCallback);
}

Compositor::~Compositor()
{
    // Remove slave cameras from the viewer
    for (const auto &pass : _passes) {
        osg::Camera *camera = pass->camera;
        // Remove all children before removing the slave to prevent the graphics
        // window from automatically cleaning up all associated OpenGL objects.
        camera->removeChildren(0, camera->getNumChildren());

        unsigned int index = _view->findSlaveIndexForCamera(camera);
        _view->removeSlave(index);
    }
}

void
Compositor::update(const osg::Matrix &view_matrix,
                   const osg::Matrix &proj_matrix)
{
    // Get the current frame number. Used for render_once passes
    int frame_number = 0;
    const osg::FrameStamp *frame_stamp = _view->getFrameStamp();
    if (frame_stamp) {
        frame_number = frame_stamp->getFrameNumber();
    }

    // Enable/disable passes by setting or unsetting their graphics context.
    // XXX: Check if this causes threading-related crashes.
    // Also run the update callback for enabled passes.
    for (auto &pass : _passes) {
        osg::Camera* camera = pass->camera;
        bool should_render = (!pass->render_condition || pass->render_condition->test())
            && (!pass->render_once || frame_number == 0);
        if (should_render) {
            // Pass is enabled
            camera->setGraphicsContext(_gc);
            if (pass->update_callback.valid()) {
                pass->update_callback->updatePass(*pass.get(), view_matrix, proj_matrix);
            }
        } else {
            // Pass is disabled
            camera->setGraphicsContext(nullptr);
        }

    }

    // Update uniforms
    osg::Matrixd view_inverse = osg::Matrix::inverse(view_matrix);
    osg::Vec4d camera_pos4 = osg::Vec4d(0.0, 0.0, 0.0, 1.0) * view_inverse;
    osg::Vec3d camera_pos = osg::Vec3d(camera_pos4.x(),
                                       camera_pos4.y(),
                                       camera_pos4.z());
    SGGeod camera_pos_geod = SGGeod::fromCart(
        SGVec3d(camera_pos.x(), camera_pos.y(), camera_pos.z()));

    osg::Vec3d world_up = camera_pos;
    world_up.normalize();
    osg::Vec3d view_up = world_up * view_matrix;
    view_up.normalize();

    double left = 0.0, right = 0.0, bottom = 0.0, top = 0.0,
        zNear = 0.0, zFar = 0.0;
    proj_matrix.getFrustum(left, right, bottom, top, zNear, zFar);

    osg::Matrixf prev_view_matrix, prev_view_matrix_inv;
    _uniforms[SG_UNIFORM_VIEW_MATRIX]->get(prev_view_matrix);
    _uniforms[SG_UNIFORM_VIEW_MATRIX_INV]->get(prev_view_matrix_inv);
    osg::Matrixf prev_proj_matrix, prev_proj_matrix_inv;
    _uniforms[SG_UNIFORM_PROJECTION_MATRIX]->get(prev_proj_matrix);
    _uniforms[SG_UNIFORM_PROJECTION_MATRIX_INV]->get(prev_proj_matrix_inv);

    _uniforms[SG_UNIFORM_PREV_VIEW_MATRIX]->set(prev_view_matrix);
    _uniforms[SG_UNIFORM_PREV_VIEW_MATRIX_INV]->set(prev_view_matrix_inv);
    _uniforms[SG_UNIFORM_PREV_PROJECTION_MATRIX]->set(prev_proj_matrix);
    _uniforms[SG_UNIFORM_PREV_PROJECTION_MATRIX_INV]->set(prev_proj_matrix_inv);

    osg::Vec3f sun_dir_world;
    _uniforms[SG_UNIFORM_SUN_DIRECTION_WORLD]->get(sun_dir_world);
    osg::Vec4f sun_dir_view = osg::Vec4f(
        sun_dir_world.x(), sun_dir_world.y(), sun_dir_world.z(), 0.0f) * view_matrix;
    
    osg::Vec3f moon_dir_world;
    _uniforms[SG_UNIFORM_MOON_DIRECTION_WORLD]->get(moon_dir_world);
    osg::Vec4f moon_dir_view = osg::Vec4f(
        moon_dir_world.x(), moon_dir_world.y(), moon_dir_world.z(), 0.0f) * view_matrix;

    float aspect_ratio = proj_matrix(1,1) / proj_matrix(0,0);
    float tan_fov_y = 1.0f / proj_matrix(1,1);
    float tan_fov_x = tan_fov_y * aspect_ratio;
    
    for (int i = 0; i < SG_TOTAL_BUILTIN_UNIFORMS; ++i) {
        osg::ref_ptr<osg::Uniform> u = _uniforms[i];
        switch (i) {
        case SG_UNIFORM_VIEW_MATRIX:
            u->set(view_matrix);
            break;
        case SG_UNIFORM_VIEW_MATRIX_INV:
            u->set(view_inverse);
            break;
        case SG_UNIFORM_PROJECTION_MATRIX:
            u->set(proj_matrix);
            break;
        case SG_UNIFORM_PROJECTION_MATRIX_INV:
            u->set(osg::Matrix::inverse(proj_matrix));
            break;
        case SG_UNIFORM_CAMERA_POSITION_CART:
            u->set(osg::Vec3f(camera_pos));
            break;
        case SG_UNIFORM_CAMERA_POSITION_GEOD:
            u->set(osg::Vec3f(camera_pos_geod.getLongitudeRad(),
                              camera_pos_geod.getLatitudeRad(),
                              camera_pos_geod.getElevationM()));
            break;
        case SG_UNIFORM_CAMERA_DISTANCE_TO_EARTH_CENTER:
            u->set(float(camera_pos.length()));
            break;
        case SG_UNIFORM_CAMERA_WORLD_UP:
            u->set(osg::Vec3f(world_up));
            break;
        case SG_UNIFORM_CAMERA_VIEW_UP:
            u->set(osg::Vec3f(view_up));
            break;
        case SG_UNIFORM_NEAR_FAR:
            u->set(osg::Vec2f(zNear, zFar));
            break;
        case SG_UNIFORM_FCOEF:
            u->set(float(2.0 / log2(zFar + 1.0)));
            break;
        case SG_UNIFORM_FOV_SCALE:
            u->set(osg::Vec2f(tan_fov_x, tan_fov_y) * 2.0f);
            break;
        case SG_UNIFORM_SUN_DIRECTION:
            u->set(osg::Vec3f(sun_dir_view.x(), sun_dir_view.y(), sun_dir_view.z()));
            break;
        case SG_UNIFORM_SUN_ZENITH_COSTHETA:
            u->set(float(sun_dir_world * world_up));
            break;
        case SG_UNIFORM_MOON_DIRECTION:
            u->set(osg::Vec3f(moon_dir_view.x(), moon_dir_view.y(), moon_dir_view.z()));
            break;
        case SG_UNIFORM_MOON_ZENITH_COSTHETA:
            u->set(float(moon_dir_world * world_up));
            break;
        case SG_UNIFORM_EARTH_RADIUS:
            u->set(float(camera_pos.length() - camera_pos_geod.getElevationM()));
            break;
        default:
            // Unknown uniform
            break;
        }
    }
}

void
Compositor::resized()
{
    // Cameras attached directly to the framebuffer were already resized by
    // osg::GraphicsContext::resizedImplementation(). However, RTT cameras were
    // ignored. Here we resize RTT cameras that need to match the physical
    // viewport size.
    for (const auto &pass : _passes) {
        osg::Camera *camera = pass->camera;
        if (!camera)
            continue;

        osg::Viewport *viewport = camera->getViewport();

        if (camera->isRenderToTextureCamera() &&
            (pass->viewport_x_scale      != 0.0f ||
             pass->viewport_y_scale      != 0.0f ||
             pass->viewport_width_scale  != 0.0f ||
             pass->viewport_height_scale != 0.0f)) {

            // Resize the viewport
            int new_x = (pass->viewport_x_scale == 0.0f) ?
                viewport->x() :
                pass->viewport_x_scale * _viewport->width();
            int new_y = (pass->viewport_y_scale == 0.0f) ?
                viewport->y() :
                pass->viewport_y_scale * _viewport->height();
            int new_width = (pass->viewport_width_scale == 0.0f) ?
                viewport->width() :
                pass->viewport_width_scale * _viewport->width();
            int new_height = (pass->viewport_height_scale == 0.0f) ?
                viewport->height() :
                pass->viewport_height_scale * _viewport->height();
            camera->setViewport(new_x, new_y, new_width, new_height);

            // Force the OSG rendering backend to handle the new sizes
            camera->dirtyAttachmentMap();
        }

        // Resize any compute dispatch related to screen size
        if (pass->compute_node.valid() &&
            (pass->compute_global_scale[0] != 0.0f ||
             pass->compute_global_scale[1] != 0.0f)) {
            auto* computeNode = static_cast<osg::DispatchCompute*>(pass->compute_node.get());
            osg::Vec2f screenSize(_viewport->width(), _viewport->height());
            osg::Vec3i groups;
            computeNode->getComputeGroups(groups[0], groups[1], groups[2]);
            for (int dim = 0; dim < 2; ++dim) {
                if (pass->compute_global_scale[dim] != 0.0f) {
                    // resize this dimention
                    groups[dim] = (int)ceil(ceil(screenSize[dim] * pass->compute_global_scale[dim]) / pass->compute_wg_size[dim]);
                    if (groups[dim] < 1)
                        groups[dim] = 1;
                }
            }
            computeNode->setComputeGroups(groups[0], groups[1], groups[2]);
        }

        // Update the uniforms even if it isn't a RTT camera
        _uniforms[SG_UNIFORM_VIEWPORT]->set(
            osg::Vec4f(viewport->x(),
                       viewport->y(),
                       viewport->width(),
                       viewport->height()));
        _uniforms[SG_UNIFORM_PIXEL_SIZE]->set(
            osg::Vec2f(1.0f / viewport->width(),
                       1.0f / viewport->height()));
        _uniforms[SG_UNIFORM_ASPECT_RATIO]->set(
            float(viewport->width() / viewport->height()));
    }

    // Resize buffers that must be a multiple of the screen size
    for (const auto &buffer : _buffers) {
        osg::Texture *texture = buffer.second->texture;
        if (texture &&
            (buffer.second->width_scale  != 0.0f ||
             buffer.second->height_scale != 0.0f)) {
            {
                auto tex = dynamic_cast<osg::Texture1D *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    tex->setTextureWidth(new_width);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture2D *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture2DArray *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height, tex->getTextureDepth());
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture2DMultisample *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture3D *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height, tex->getTextureDepth());
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::TextureRectangle *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::TextureCubeMap *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height);
                    tex->dirtyTextureObject();
                }
            }
        }
    }
}

void
Compositor::setCullMask(osg::Node::NodeMask cull_mask)
{
    for (auto &pass : _passes) {
        osg::Camera *camera = pass->camera;
        if (pass->inherit_cull_mask) {
            camera->setCullMask(pass->cull_mask & cull_mask);
            camera->setCullMaskLeft(pass->cull_mask & cull_mask & ~RIGHT_BIT);
            camera->setCullMaskRight(pass->cull_mask & cull_mask & ~LEFT_BIT);
        } else {
            camera->setCullMask(pass->cull_mask);
            camera->setCullMaskLeft(pass->cull_mask & ~RIGHT_BIT);
            camera->setCullMaskRight(pass->cull_mask & ~LEFT_BIT);
        }
    }
}

void
Compositor::addBuffer(const std::string &name, Buffer *buffer)
{
    _buffers[name] = buffer;
}

void
Compositor::addPass(Pass *pass)
{
    if (!_view) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Compositor::addPass: Couldn't add camera "
               "as a slave to the view. View doesn't exist!");
        return;
    }
    _view->addSlave(pass->camera, pass->useMastersSceneData);
    installEffectCullVisitor(pass->camera, pass->collect_lights, pass->effect_scheme);
    _passes.push_back(pass);
}

Buffer *
Compositor::getBuffer(const std::string &name) const
{
    auto it = _buffers.find(name);
    if (it == _buffers.end())
        return 0;
    return it->second.get();
}

Pass *
Compositor::getPass(const std::string &name) const
{
    auto it = std::find_if(_passes.begin(), _passes.end(),
                           [&name](const osg::ref_ptr<Pass> &p) {
                               return p->name == name;
                           });
    if (it == _passes.end())
        return 0;
    return (*it);
}

} // namespace compositor
} // namespace simgear
