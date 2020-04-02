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

#include "ClusteredShading.hxx"

#include <thread>

#include <osg/BufferIndexBinding>
#include <osg/BufferObject>
#include <osg/RenderInfo>
#include <osg/Texture3D>
#include <osg/TextureBuffer>
#include <osg/Version>

#include <osg/io_utils>

#include <simgear/constants.h>
#include <simgear/structure/exception.hxx>

namespace simgear {
namespace compositor {

const int MAX_LIGHT_INDICES = 524288;      // 1 MB (2 bytes per index)
const int MAX_POINTLIGHTS = 256;
const int MAX_SPOTLIGHTS = 256;

// Size in floats (4 bytes) of the light struct to be passed to the GLSL shader.
// It must be a multiple of the size of a vec4 as per the std140 layout rules.
// See https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_uniform_buffer_object.txt
const int POINTLIGHT_BLOCK_SIZE = 20;
const int SPOTLIGHT_BLOCK_SIZE = 28;

ClusteredShading::ClusteredShading(osg::Camera *camera,
                                   const SGPropertyNode *config) :
    _camera(camera)
{
    _tile_size = config->getIntValue("tile-size", 128);
    _depth_slices = config->getIntValue("depth-slices", 1);
    _num_threads = config->getIntValue("num-threads", 1);
    _slices_per_thread = _depth_slices / _num_threads;
    if (_slices_per_thread == 0) {
        SG_LOG(SG_INPUT, SG_INFO, "ClusteredShading::ClusteredShading(): "
               "More threads than depth slices");
        _num_threads = _depth_slices;
    }
    _slices_remainder = _depth_slices % _num_threads;

    osg::StateSet *ss = _camera->getOrCreateStateSet();

    osg::Uniform *tile_size_uniform =
        new osg::Uniform("fg_ClusteredTileSize", _tile_size);
    ss->addUniform(tile_size_uniform);
    _slice_scale = new osg::Uniform("fg_ClusteredSliceScale", 0.0f);
    ss->addUniform(_slice_scale.get());
    _slice_bias = new osg::Uniform("fg_ClusteredSliceBias", 0.0f);
    ss->addUniform(_slice_bias.get());

    // Create and associate the light grid 3D texture
    ////////////////////////////////////////////////////////////////////////////
    _light_grid = new osg::Image;
    _light_grid->setInternalTextureFormat(GL_RGB32UI_EXT);
    // Image allocation happens in setupSubfrusta() because the light grid size
    // can change at runtime (viewport resize)

    osg::ref_ptr<osg::Texture3D> light_grid_tex = new osg::Texture3D;
    light_grid_tex->setResizeNonPowerOfTwoHint(false);
    light_grid_tex->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
    light_grid_tex->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::CLAMP_TO_BORDER);
    light_grid_tex->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::CLAMP_TO_BORDER);
    light_grid_tex->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::NEAREST);
    light_grid_tex->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::NEAREST);
    light_grid_tex->setImage(_light_grid.get());

    int light_grid_bind_unit = config->getIntValue("grid-bind-unit", 11);
    ss->setTextureAttributeAndModes(
        light_grid_bind_unit, light_grid_tex.get(), osg::StateAttribute::ON);

    osg::ref_ptr<osg::Uniform> light_grid_uniform =
        new osg::Uniform("fg_ClusteredLightGrid", light_grid_bind_unit);
    ss->addUniform(light_grid_uniform.get());

    // Create and associate the light indices TBO
    ////////////////////////////////////////////////////////////////////////////
    _light_indices = new osg::Image;
    _light_indices->allocateImage(
        MAX_LIGHT_INDICES, 1, 1, GL_RED_INTEGER_EXT, GL_UNSIGNED_SHORT);

    osg::ref_ptr<osg::TextureBuffer> light_indices_tbo =
        new osg::TextureBuffer;
    light_indices_tbo->setInternalFormat(GL_R16UI);
    light_indices_tbo->setImage(_light_indices.get());

    int light_indices_bind_unit = config->getIntValue("indices-bind-unit", 12);
    ss->setTextureAttribute(light_indices_bind_unit, light_indices_tbo.get());

    osg::ref_ptr<osg::Uniform> light_indices_uniform =
        new osg::Uniform("fg_ClusteredLightIndices", light_indices_bind_unit);
    ss->addUniform(light_indices_uniform.get());

    // Create and associate the pointlight data UBO
    ////////////////////////////////////////////////////////////////////////////
    _pointlight_data = new osg::FloatArray(MAX_POINTLIGHTS * POINTLIGHT_BLOCK_SIZE);

    osg::ref_ptr<osg::UniformBufferObject> pointlight_data_ubo =
        new osg::UniformBufferObject;
    _pointlight_data->setBufferObject(pointlight_data_ubo.get());

    int pointlight_ubo_index = config->getIntValue("pointlight-ubo-index", 5);
#if OSG_VERSION_LESS_THAN(3,6,0)
    osg::ref_ptr<osg::UniformBufferBinding> pointlight_data_ubb =
        new osg::UniformBufferBinding(
            pointlight_ubo_index,
            pointlight_data_ubo.get(),
            0,
            MAX_POINTLIGHTS * POINTLIGHT_BLOCK_SIZE * sizeof(GLfloat));
#else
    osg::ref_ptr<osg::UniformBufferBinding> pointlight_data_ubb =
        new osg::UniformBufferBinding(
            pointlight_ubo_index,
            _pointlight_data.get(),
            0,
            MAX_POINTLIGHTS * POINTLIGHT_BLOCK_SIZE * sizeof(GLfloat));
#endif
    pointlight_data_ubb->setDataVariance(osg::Object::DYNAMIC);
    ss->setAttribute(pointlight_data_ubb.get(), osg::StateAttribute::ON);

    // Create and associate the spotlight data UBO
    ////////////////////////////////////////////////////////////////////////////
    _spotlight_data = new osg::FloatArray(MAX_SPOTLIGHTS * SPOTLIGHT_BLOCK_SIZE);

    osg::ref_ptr<osg::UniformBufferObject> spotlight_data_ubo =
        new osg::UniformBufferObject;
    _spotlight_data->setBufferObject(spotlight_data_ubo.get());

    int spotlight_ubo_index = config->getIntValue("spotlight-ubo-index", 6);
#if OSG_VERSION_LESS_THAN(3,6,0)
    osg::ref_ptr<osg::UniformBufferBinding> spotlight_data_ubb =
        new osg::UniformBufferBinding(
            spotlight_ubo_index,
            spotlight_data_ubo.get(),
            0,
            MAX_SPOTLIGHTS * SPOTLIGHT_BLOCK_SIZE * sizeof(GLfloat));
#else
    osg::ref_ptr<osg::UniformBufferBinding> spotlight_data_ubb =
        new osg::UniformBufferBinding(
            spotlight_ubo_index,
            _spotlight_data.get(),
            0,
            MAX_SPOTLIGHTS * SPOTLIGHT_BLOCK_SIZE * sizeof(GLfloat));
#endif
    spotlight_data_ubb->setDataVariance(osg::Object::DYNAMIC);
    ss->setAttribute(spotlight_data_ubb.get(), osg::StateAttribute::ON);
}

ClusteredShading::~ClusteredShading()
{
}

void
ClusteredShading::update(const SGLightList &light_list)
{
    // Transform every light to a more comfortable data structure for collision
    // testing, separating point and spot lights in the process
    _point_bounds.clear();
    _spot_bounds.clear();
    for (const auto &light : light_list) {
        if (light->getType() == SGLight::Type::POINT) {
            PointlightBound point;
            point.light = light;
            point.position = osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f) *
                // The parenthesis are very important: if the vector is
                // multiplied by the local to world matrix first we'll have
                // precision issues
                (osg::computeLocalToWorld(light->getParentalNodePaths()[0]) *
                 _camera->getViewMatrix());
            point.range = light->getRange();

            _point_bounds.push_back(point);
        } else if (light->getType() == SGLight::Type::SPOT) {
            SpotlightBound spot;
            spot.light = light;
            spot.position = osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f) *
                (osg::computeLocalToWorld(light->getParentalNodePaths()[0]) *
                 _camera->getViewMatrix());
            spot.direction = osg::Vec4f(0.0f, 0.0f, -1.0f, 0.0f) *
                (osg::computeLocalToWorld(light->getParentalNodePaths()[0]) *
                 _camera->getViewMatrix());

            float range = light->getRange();
            float angle = light->getSpotCutoff() * SG_DEGREES_TO_RADIANS;
            spot.cos_cutoff = cos(angle);
            if(angle > SGD_PI_4) {
                spot.bounding_sphere.radius = range * tan(angle);
            } else {
                spot.bounding_sphere.radius =
                    range * 0.5f / pow(spot.cos_cutoff, 2.0f);
            }

            spot.bounding_sphere.center =
                spot.position + spot.direction * spot.bounding_sphere.radius;

            _spot_bounds.push_back(spot);
        }
    }
    if (_point_bounds.size() > MAX_POINTLIGHTS ||
        _spot_bounds.size()  > MAX_SPOTLIGHTS) {
        throw sg_range_exception("Maximum amount of visible lights surpassed");
    }

    float l, r, b, t;
    _camera->getProjectionMatrix().getFrustum(l, r, b, t, _zNear, _zFar);
    _slice_scale->set(_depth_slices / log2(_zFar / _zNear));
    _slice_bias->set(-_depth_slices * log2(_zNear) / log2(_zFar / _zNear));

    const osg::Viewport *vp = _camera->getViewport();
    static int old_width = 0, old_height = 0;
    int width = vp->width(); int height = vp->height();
    if (width != old_width || height != old_height) {
        old_width = width; old_height = height;

        _n_htiles = (width  + _tile_size - 1) / _tile_size;
        _n_vtiles = (height + _tile_size - 1) / _tile_size;

        _x_step = (_tile_size / float(width)) * 2.0;
        _y_step = (_tile_size / float(height)) * 2.0;

        _light_grid->allocateImage(_n_htiles, _n_vtiles, _depth_slices,
                                   GL_RGB_INTEGER_EXT, GL_UNSIGNED_INT);
        _subfrusta.reset(new Subfrustum[_n_htiles * _n_vtiles]);
    }

    for (int y = 0; y < _n_vtiles; ++y) {
        float ymin = -1.0 + _y_step * float(y);
        float ymax = ymin + _y_step;
        for (int x = 0; x < _n_htiles; ++x) {
            float xmin = -1.0 + _x_step * float(x);
            float xmax = xmin + _x_step;

            // Create the subfrustum in clip space
            // The near and far planes will be filled later as they change from
            // slice to slice
            Subfrustum &subfrustum = _subfrusta[y*_n_htiles + x];
            subfrustum.plane[0].set(1.0f,0.0f,0.0f,-xmin); // left plane.
            subfrustum.plane[1].set(-1.0f,0.0f,0.0f,xmax); // right plane.
            subfrustum.plane[2].set(0.0f,1.0f,0.0f,-ymin); // bottom plane.
            subfrustum.plane[3].set(0.0f,-1.0f,0.0f,ymax); // top plane.

            // Transform it to view space
            for (int i = 0; i < 4; ++i) {
                osg::Vec4f &p = subfrustum.plane[i];
                p = _camera->getProjectionMatrix() * p;
                float inv_length = 1.0 / sqrt(p._v[0]*p._v[0] +
                                              p._v[1]*p._v[1] +
                                              p._v[2]*p._v[2]);
                p *= inv_length;
            }
        }
    }

    _global_light_count = 0;

    if (_depth_slices == 1) {
        // Just run the light assignment on the main thread to avoid the
        // unnecessary threading overhead
        assignLightsToSlice(0);
    } else if (_num_threads == 1) {
        // Again, avoid the unnecessary threading overhead
        threadFunc(0);
    } else {
        std::vector<std::thread> threads;
        threads.reserve(_num_threads);
        for (int i = 0; i < _num_threads; ++i)
            threads.emplace_back(&ClusteredShading::threadFunc, this, i);

        for (auto &t : threads) t.join();
    }

    // Force upload of the image data
    _light_grid->dirty();

    // Upload pointlight and spotlight data
    writePointlightData();
    writeSpotlightData();
}

void
ClusteredShading::threadFunc(int thread_id)
{
    for (int i = 0; i < _slices_per_thread; ++i)
        assignLightsToSlice(thread_id * _slices_per_thread + i);

    if (_slices_remainder > thread_id)
        assignLightsToSlice(_slices_per_thread * _num_threads + thread_id);
}

void
ClusteredShading::assignLightsToSlice(int slice)
{
    size_t z_offset = slice * _n_htiles * _n_vtiles;

    float near = getDepthForSlice(slice);
    float far  = getDepthForSlice(slice + 1);
    osg::Vec4f near_plane(0.0f, 0.0f, -1.0f, -near);
    osg::Vec4f far_plane (0.0f, 0.0f,  1.0f,  far);

    GLuint *grid = reinterpret_cast<GLuint *>(_light_grid->data());
    GLushort *indices = reinterpret_cast<GLushort *>(_light_indices->data());

    for (int i = 0; i < (_n_htiles * _n_vtiles); ++i) {
        Subfrustum subfrustum = _subfrusta[i];
        subfrustum.plane[4] = near_plane;
        subfrustum.plane[5] = far_plane;

        GLuint start_offset = _global_light_count;
        GLuint local_point_count = 0;
        GLuint local_spot_count = 0;

        // Test point lights
        for (GLushort point_iterator = 0;
             point_iterator < _point_bounds.size();
             ++point_iterator) {
            PointlightBound point = _point_bounds[point_iterator];

            // Perform frustum-sphere collision tests
            float distance = 0.0f;
            for (int j = 0; j < 6; j++) {
                distance = subfrustum.plane[j] * point.position + point.range;
                if (distance <= 0.0f)
                    break;
            }

            if (distance > 0.0f) {
                // Update light index list
                indices[_global_light_count] = point_iterator;
                ++local_point_count;
                ++_global_light_count; // Atomic increment
            }

            if (_global_light_count >= MAX_LIGHT_INDICES) {
                throw sg_range_exception(
                    "Clustered shading light index count is over the hardcoded limit ("
                    + std::to_string(MAX_LIGHT_INDICES) + ")");
            }
        }

        // Test spot lights
        for (GLushort spot_iterator = 0;
             spot_iterator < _spot_bounds.size();
             ++spot_iterator) {
            SpotlightBound spot = _spot_bounds[spot_iterator];

            // Perform frustum-sphere collision tests
            float distance = 0.0f;
            for (int j = 0; j < 6; j++) {
                distance = subfrustum.plane[j] * spot.bounding_sphere.center
                    + spot.bounding_sphere.radius;
                if (distance <= 0.0f)
                    break;
            }

            if (distance > 0.0f) {
                // Update light index list
                indices[_global_light_count] = spot_iterator;
                ++local_spot_count;
                ++_global_light_count; // Atomic increment
            }

            if (_global_light_count >= MAX_LIGHT_INDICES) {
                throw sg_range_exception(
                    "Clustered shading light index count is over the hardcoded limit ("
                    + std::to_string(MAX_LIGHT_INDICES) + ")");
            }
        }

        // Update light grid
        grid[(z_offset + i) * 3 + 0] = start_offset;
        grid[(z_offset + i) * 3 + 1] = local_point_count;
        grid[(z_offset + i) * 3 + 2] = local_spot_count;
    }
}

void
ClusteredShading::writePointlightData()
{
    GLfloat *data = reinterpret_cast<GLfloat *>(&(*_pointlight_data)[0]);

    for (const auto &point : _point_bounds) {
        // vec4 position
        *data++ = point.position.x();
        *data++ = point.position.y();
        *data++ = point.position.z();
        *data++ = 1.0f;
        // vec4 ambient
        *data++ = point.light->getAmbient().x();
        *data++ = point.light->getAmbient().y();
        *data++ = point.light->getAmbient().z();
        *data++ = point.light->getAmbient().w();
        // vec4 diffuse
        *data++ = point.light->getDiffuse().x();
        *data++ = point.light->getDiffuse().y();
        *data++ = point.light->getDiffuse().z();
        *data++ = point.light->getDiffuse().w();
        // vec4 specular
        *data++ = point.light->getSpecular().x();
        *data++ = point.light->getSpecular().y();
        *data++ = point.light->getSpecular().z();
        *data++ = point.light->getSpecular().w();
        // vec4 attenuation (x = constant, y = linear, z = quadratic, w = range)
        *data++ = point.light->getConstantAttenuation();
        *data++ = point.light->getLinearAttenuation();
        *data++ = point.light->getQuadraticAttenuation();
        *data++ = point.light->getRange();
        // No padding needed as the resulting size is a multiple of vec4
    }
    _pointlight_data->dirty();
}

void
ClusteredShading::writeSpotlightData()
{
    GLfloat *data = reinterpret_cast<GLfloat *>(&(*_spotlight_data)[0]);

    for (const auto &spot : _spot_bounds) {
        // vec4 position
        *data++ = spot.position.x();
        *data++ = spot.position.y();
        *data++ = spot.position.z();
        *data++ = 1.0f;
        // vec4 direction
        *data++ = spot.direction.x();
        *data++ = spot.direction.y();
        *data++ = spot.direction.z();
        *data++ = 0.0f;
        // vec4 ambient
        *data++ = spot.light->getAmbient().x();
        *data++ = spot.light->getAmbient().y();
        *data++ = spot.light->getAmbient().z();
        *data++ = spot.light->getAmbient().w();
        // vec4 diffuse
        *data++ = spot.light->getDiffuse().x();
        *data++ = spot.light->getDiffuse().y();
        *data++ = spot.light->getDiffuse().z();
        *data++ = spot.light->getDiffuse().w();
        // vec4 specular
        *data++ = spot.light->getSpecular().x();
        *data++ = spot.light->getSpecular().y();
        *data++ = spot.light->getSpecular().z();
        *data++ = spot.light->getSpecular().w();
        // vec4 attenuation (x = constant, y = linear, z = quadratic, w = range)
        *data++ = spot.light->getConstantAttenuation();
        *data++ = spot.light->getLinearAttenuation();
        *data++ = spot.light->getQuadraticAttenuation();
        *data++ = spot.light->getRange();
        // float cos_cutoff
        *data++ = spot.cos_cutoff;
        // float exponent
        *data++ = spot.light->getSpotExponent();
        // Needs 2N padding (8 bytes)
    }
    _spotlight_data->dirty();
}

float
ClusteredShading::getDepthForSlice(int slice) const
{
    return _zNear * pow(_zFar / _zNear, float(slice) / _depth_slices);
}

} // namespace compositor
} // namespace simgear
