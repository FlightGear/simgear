// Copyright (C) 2018  Fernando García Liñán <fernandogarcialinan@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include "Compositor.hxx"

#include <algorithm>

#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture2DMultisample>
#include <osg/Texture3D>
#include <osg/TextureRectangle>
#include <osg/TextureCubeMap>
#include <osgUtil/IntersectionVisitor>
#include <osgViewer/Renderer>
#include <osgViewer/Viewer>

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
        SGUpdateVisitor *uv = dynamic_cast<SGUpdateVisitor *>(nv);
        osg::Vec3f l = toOsg(uv->getLightDirection());
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
    new osg::Uniform("fg_Viewport", osg::Vec4f()),
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
    new osg::Uniform("fg_Planes", osg::Vec3f()),
    new osg::Uniform("fg_SunDirection", osg::Vec3f()),
    new osg::Uniform("fg_SunDirectionWorld", osg::Vec3f()),
    new osg::Uniform("fg_SunZenithCosTheta", 0.0f),
    new osg::Uniform("fg_EarthRadius", 0.0f),
    }
{
    _uniforms[SG_UNIFORM_SUN_DIRECTION_WORLD]->setUpdateCallback(
        new SunDirectionWorldCallback);
}

Compositor::~Compositor()
{
    // Remove slave cameras from the viewer
    for (const auto &pass : _passes) {
        unsigned int index = _view->findSlaveIndexForCamera(pass->camera);
        _view->removeSlave(index);
    }
}

void
Compositor::update(const osg::Matrix &view_matrix,
                   const osg::Matrix &proj_matrix)
{
    for (auto &pass : _passes) {
        if (pass->update_callback.valid())
            pass->update_callback->updatePass(*pass.get(), view_matrix, proj_matrix);
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
    osg::Vec3d view_up = world_up * view_inverse;
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

    for (int i = 0; i < SG_TOTAL_BUILTIN_UNIFORMS; ++i) {
        osg::ref_ptr<osg::Uniform> u = _uniforms[i];
        switch (i) {
        case SG_UNIFORM_VIEWPORT:
            u->set(osg::Vec4f(_viewport->x(), _viewport->y(),
                              _viewport->width(), _viewport->height()));
            break;
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
        case SG_UNIFORM_PLANES:
            u->set(osg::Vec3f(-zFar, -zFar * zNear, zFar - zNear));
            break;
        case SG_UNIFORM_SUN_DIRECTION:
            u->set(osg::Vec3f(sun_dir_view.x(), sun_dir_view.y(), sun_dir_view.z()));
            break;
        case SG_UNIFORM_SUN_ZENITH_COSTHETA:
            u->set(float(sun_dir_world * world_up));
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
        if (camera && camera->isRenderToTextureCamera()
            && pass->viewport_width_scale  != 0.0f
            && pass->viewport_height_scale != 0.0f) {

            // Resize the viewport
            camera->setViewport(0, 0,
                                pass->viewport_width_scale  * _viewport->width(),
                                pass->viewport_height_scale * _viewport->height());
            // Force the OSG rendering backend to handle the new sizes
            camera->dirtyAttachmentMap();
        }
    }

    // Resize buffers that must be a multiple of the screen size
    for (const auto &buffer : _buffers) {
        osg::Texture *texture = buffer.second->texture;
        if (texture
            && buffer.second->width_scale  != 0.0f
            && buffer.second->height_scale != 0.0f) {

            int width  = buffer.second->width_scale  * _viewport->width();
            int height = buffer.second->height_scale * _viewport->height();
            {
                auto tex = dynamic_cast<osg::Texture1D *>(texture);
                if (tex) {
                    tex->setTextureWidth(width);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture2D *>(texture);
                if (tex) {
                    tex->setTextureSize(width, height);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture2DArray *>(texture);
                if (tex) {
                    tex->setTextureSize(width, height, tex->getTextureDepth());
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture2DMultisample *>(texture);
                if (tex) {
                    tex->setTextureSize(width, height);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture3D *>(texture);
                if (tex) {
                    tex->setTextureSize(width, height, tex->getTextureDepth());
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::TextureRectangle *>(texture);
                if (tex) {
                    tex->setTextureSize(width, height);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::TextureCubeMap *>(texture);
                if (tex) {
                    tex->setTextureSize(width, height);
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
        if (pass->inherit_cull_mask) {
            osg::Camera *camera = pass->camera;
            camera->setCullMask(pass->cull_mask & cull_mask);
            camera->setCullMaskLeft(pass->cull_mask & cull_mask);
            camera->setCullMaskRight(pass->cull_mask & cull_mask);
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

    // Install the Effect cull visitor
    osgViewer::Renderer* renderer
        = static_cast<osgViewer::Renderer*>(pass->camera->getRenderer());
    for (int i = 0; i < 2; ++i) {
        osgUtil::SceneView* sceneView = renderer->getSceneView(i);

        osg::ref_ptr<osgUtil::CullVisitor::Identifier> identifier;
        identifier = sceneView->getCullVisitor()->getIdentifier();

        sceneView->setCullVisitor(
            new EffectCullVisitor(pass->collect_lights, pass->effect_scheme));
        sceneView->getCullVisitor()->setIdentifier(identifier.get());

        identifier = sceneView->getCullVisitorLeft()->getIdentifier();
        sceneView->setCullVisitorLeft(sceneView->getCullVisitor()->clone());
        sceneView->getCullVisitorLeft()->setIdentifier(identifier.get());

        identifier = sceneView->getCullVisitorRight()->getIdentifier();
        sceneView->setCullVisitorRight(sceneView->getCullVisitor()->clone());
        sceneView->getCullVisitorRight()->setIdentifier(identifier.get());
    }

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
