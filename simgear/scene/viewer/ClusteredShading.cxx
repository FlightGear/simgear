// Copyright (C) 2018 - 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "ClusteredShading.hxx"

#include <thread>
#include <algorithm>

#include <osg/RenderInfo>
#include <osg/Version>

#include <osg/io_utils>

#include <simgear/constants.h>
#include <simgear/structure/exception.hxx>

#include "CompositorUtil.hxx"

namespace simgear {
namespace compositor {

ClusteredShading::ClusteredShading(osg::Camera *camera,
                                   const SGPropertyNode *config) :
    _camera(camera)
{
    _pbr_lights = config->getBoolValue("pbr-lights", false);

    const SGPropertyNode *p_max_pointlights =
        getPropertyChild(config, "max-pointlights");
    _max_pointlights = p_max_pointlights ? p_max_pointlights->getIntValue() : 1024;
    const SGPropertyNode *p_max_spotlights =
        getPropertyChild(config, "max-spotlights");
    _max_spotlights = p_max_spotlights ? p_max_spotlights->getIntValue() : 1024;

    _max_light_indices = config->getIntValue("max-light-indices", 256);
    _tile_size = config->getIntValue("tile-size", 128);
    _depth_slices = config->getIntValue("depth-slices", 1);
    _num_threads = config->getIntValue("num-threads", 1);
    if (_num_threads == 0) {
        _num_threads = std::thread::hardware_concurrency();
        if (_depth_slices < _num_threads)
            _depth_slices = _num_threads;
    }

    _slices_per_thread = _depth_slices / _num_threads;
    if (_slices_per_thread == 0) {
        SG_LOG(SG_INPUT, SG_INFO, "ClusteredShading::ClusteredShading(): "
               "More threads than depth slices");
        _num_threads = _depth_slices;
    }
    _slices_remainder = _depth_slices % _num_threads;

    _slice_scale = new osg::Uniform("fg_ClusteredSliceScale", 0.0f);
    _slice_bias = new osg::Uniform("fg_ClusteredSliceBias", 0.0f);
    _horizontal_tiles = new osg::Uniform("fg_ClusteredHorizontalTiles", 0);
    _vertical_tiles = new osg::Uniform("fg_ClusteredVerticalTiles", 0);

    // Create and associate the cluster 3D texture
    ////////////////////////////////////////////////////////////////////////////
    _clusters = new osg::Image;
    // Image allocation happens in setupSubfrusta() because the number of
    // clusters can change at runtime (viewport resize)

    _clusters_tex = new osg::Texture3D;
    _clusters_tex->setInternalFormat(GL_RGB32F_ARB);
    _clusters_tex->setResizeNonPowerOfTwoHint(false);
    _clusters_tex->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
    _clusters_tex->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::CLAMP_TO_BORDER);
    _clusters_tex->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::CLAMP_TO_BORDER);
    _clusters_tex->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::NEAREST);
    _clusters_tex->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::NEAREST);
    _clusters_tex->setImage(_clusters.get());

    // Create and associate the light indices texture
    ////////////////////////////////////////////////////////////////////////////

    _indices = new osg::Image;
    _indices->allocateImage(_max_light_indices, _max_light_indices, 1,
                            GL_RED, GL_FLOAT);

    _indices_tex = new osg::Texture2D;
    _indices_tex->setInternalFormat(GL_R32F);
    _indices_tex->setResizeNonPowerOfTwoHint(false);
    _indices_tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    _indices_tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
    _indices_tex->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_BORDER);
    _indices_tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
    _indices_tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
    _indices_tex->setImage(_indices.get());

    // Create and associate the pointlights buffer
    ////////////////////////////////////////////////////////////////////////////

    _pointlights = new osg::Image;
    _pointlights->allocateImage(_pbr_lights ? 2 : 5, _max_pointlights, 1, GL_RGBA, GL_FLOAT);

    _pointlights_tex = new osg::Texture2D;
    _pointlights_tex->setInternalFormat(GL_RGBA32F_ARB);
    _pointlights_tex->setResizeNonPowerOfTwoHint(false);
    _pointlights_tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    _pointlights_tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
    _pointlights_tex->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_BORDER);
    _pointlights_tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
    _pointlights_tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
    _pointlights_tex->setImage(_pointlights.get());

    // Create and associate the spotlights buffer
    ////////////////////////////////////////////////////////////////////////////

    _spotlights = new osg::Image;
    _spotlights->allocateImage(_pbr_lights ? 4 : 7, _max_spotlights, 1, GL_RGBA, GL_FLOAT);

    _spotlights_tex = new osg::Texture2D;
    _spotlights_tex->setInternalFormat(GL_RGBA32F_ARB);
    _spotlights_tex->setResizeNonPowerOfTwoHint(false);
    _spotlights_tex->setWrap(osg::Texture2D::WRAP_R, osg::Texture2D::CLAMP_TO_BORDER);
    _spotlights_tex->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_BORDER);
    _spotlights_tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
    _spotlights_tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
    _spotlights_tex->setImage(_spotlights.get());

    ////////////////////////////////////////////////////////////////////////////

    if (config->getBoolValue("expose-uniforms", true)) {
        exposeUniformsToPass(camera,
                             config->getIntValue("clusters-bind-unit", 11),
                             config->getIntValue("indices-bind-unit", 12),
                             config->getIntValue("pointlights-bind-unit", 13),
                             config->getIntValue("spotlights-bind-unit", 14));
    }
}

ClusteredShading::~ClusteredShading()
{
}

void
ClusteredShading::exposeUniformsToPass(osg::Camera *camera,
                                       int clusters_bind_unit,
                                       int indices_bind_unit,
                                       int pointlights_bind_unit,
                                       int spotlights_bind_unit)
{
    osg::StateSet *ss = camera->getOrCreateStateSet();

    // Constant uniforms
    osg::ref_ptr<osg::Uniform> max_pointlights_uniform =
        new osg::Uniform("fg_ClusteredMaxPointLights", _max_pointlights);
    ss->addUniform(max_pointlights_uniform);
    osg::ref_ptr<osg::Uniform> max_spotlights_uniform =
        new osg::Uniform("fg_ClusteredMaxSpotLights", _max_pointlights);
    ss->addUniform(max_spotlights_uniform);
    osg::ref_ptr<osg::Uniform> max_light_indices_uniform =
        new osg::Uniform("fg_ClusteredMaxLightIndices", _max_light_indices);
    ss->addUniform(max_light_indices_uniform);
    osg::ref_ptr<osg::Uniform> tile_size_uniform =
        new osg::Uniform("fg_ClusteredTileSize", _tile_size);
    ss->addUniform(tile_size_uniform);
    osg::ref_ptr<osg::Uniform> depth_slices_uniform =
        new osg::Uniform("fg_ClusteredDepthSlices", _depth_slices);
    ss->addUniform(depth_slices_uniform);

    // Dynamic uniforms
    ss->addUniform(_slice_scale);
    ss->addUniform(_slice_bias);
    ss->addUniform(_horizontal_tiles);
    ss->addUniform(_vertical_tiles);

    // Textures
    osg::ref_ptr<osg::Uniform> clusters_uniform =
        new osg::Uniform("fg_Clusters", clusters_bind_unit);
    ss->addUniform(clusters_uniform.get());
    ss->setTextureAttributeAndModes(
        clusters_bind_unit, _clusters_tex.get(), osg::StateAttribute::ON);

    osg::ref_ptr<osg::Uniform> indices_uniform =
        new osg::Uniform("fg_ClusteredIndices", indices_bind_unit);
    ss->addUniform(indices_uniform.get());
    ss->setTextureAttributeAndModes(
        indices_bind_unit, _indices_tex.get(), osg::StateAttribute::ON);

    osg::ref_ptr<osg::Uniform> pointlights_uniform =
        new osg::Uniform("fg_ClusteredPointLights", pointlights_bind_unit);
    ss->addUniform(pointlights_uniform.get());
    ss->setTextureAttributeAndModes(
        pointlights_bind_unit, _pointlights_tex.get(), osg::StateAttribute::ON);

    osg::ref_ptr<osg::Uniform> spotlights_uniform =
        new osg::Uniform("fg_ClusteredSpotLights", spotlights_bind_unit);
    ss->addUniform(spotlights_uniform.get());
    ss->setTextureAttributeAndModes(
        spotlights_bind_unit, _spotlights_tex.get(), osg::StateAttribute::ON);
}

void
ClusteredShading::update(const SGLightList &light_list)
{
    // Transform every light to a more comfortable data structure for collision
    // testing, separating point and spot lights in the process.
    updateLightBounds(light_list);
    // We prefer to render higher priority lights and lights that are close
    // to the viewer, so we sort them if necessary.
    sortLightBounds();

    recreateSubfrustaIfNeeded();
    updateUniforms();
    updateSubfrusta();

    _global_light_count = 0;

    if (_depth_slices == 1) {
        // Just run the light assignment on the main thread to avoid the
        // unnecessary threading overhead.
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
    _clusters->dirty();
    _indices->dirty();

    if (!_pbr_lights) {
        writePointlightData();
        writeSpotlightData();
    } else {
        writePointlightDataPBR();
        writeSpotlightDataPBR();
    }
}

void
ClusteredShading::updateLightBounds(const SGLightList &light_list)
{
    _point_bounds.clear();
    _spot_bounds.clear();

    for (const auto &light : light_list) {
        if (light->getType() == SGLight::POINT) {
            PointlightBound point;
            point.light = light;
            point.position = osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f) *
                // The parenthesis are very important: if the vector is
                // multiplied by the local to world matrix first we'll have
                // precision issues
                (light->getWorldMatrices()[0] * _camera->getViewMatrix());
            point.range = light->getRange();

            _point_bounds.push_back(point);
        } else if (light->getType() == SGLight::SPOT) {
            SpotlightBound spot;
            spot.light = light;
            spot.position = osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f) *
                (light->getWorldMatrices()[0] * _camera->getViewMatrix());
            spot.direction = osg::Vec4f(0.0f, 0.0f, -1.0f, 0.0f) *
                (light->getWorldMatrices()[0] * _camera->getViewMatrix());
            spot.direction.normalize();

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
}

void
ClusteredShading::sortLightBounds()
{
    if (_point_bounds.size() > static_cast<unsigned int>(_max_pointlights)) {
        std::sort(_point_bounds.begin(), _point_bounds.end(),
                  [](auto &a, auto &b) {
                      if (a.light->getPriority() != b.light->getPriority())
                          return a.light->getPriority() < b.light->getPriority();
                      return a.position.length() < b.position.length();
                  });
        _point_bounds.resize(_max_pointlights);
    }

    if (_spot_bounds.size() > static_cast<unsigned int>(_max_spotlights)) {
        std::sort(_spot_bounds.begin(), _spot_bounds.end(),
                  [](auto &a, auto &b) {
                      if (a.light->getPriority() != b.light->getPriority())
                          return a.light->getPriority() < b.light->getPriority();
                      return a.bounding_sphere.center.length() <
                          b.bounding_sphere.center.length();
                  });
        _spot_bounds.resize(_max_spotlights);
    }
}

void
ClusteredShading::recreateSubfrustaIfNeeded()
{
    const osg::Viewport *vp = _camera->getViewport();
    int width = vp->width(); int height = vp->height();
    if (width != _old_width || height != _old_height) {
        _old_width = width; _old_height = height;

        _n_htiles = (width  + _tile_size - 1) / _tile_size;
        _n_vtiles = (height + _tile_size - 1) / _tile_size;

        _x_step = (_tile_size / float(width)) * 2.0;
        _y_step = (_tile_size / float(height)) * 2.0;

        _clusters->allocateImage(_n_htiles, _n_vtiles, _depth_slices,
                                 GL_RGB, GL_FLOAT);
        _subfrusta.reset(new Subfrustum[_n_htiles * _n_vtiles]);
    }
}

void
ClusteredShading::updateUniforms()
{
    float l = 0.f, r = 0.f, b = 0.f, t = 0.f;
    _camera->getProjectionMatrix().getFrustum(l, r, b, t, _zNear, _zFar);

    _slice_scale->set(_depth_slices / std::log2(_zFar / _zNear));
    _slice_bias->set(-_depth_slices * std::log2(_zNear) / std::log2(_zFar / _zNear));

    _horizontal_tiles->set(_n_htiles);
    _vertical_tiles->set(_n_vtiles);
}

void
ClusteredShading::updateSubfrusta()
{
    for (int y = 0; y < _n_vtiles; ++y) {
        float ymin = -1.0f + _y_step * float(y);
        float ymax = ymin  + _y_step;
        for (int x = 0; x < _n_htiles; ++x) {
            float xmin = -1.0f + _x_step * float(x);
            float xmax = xmin  + _x_step;

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
                float inv_length = 1.0f / sqrtf(p._v[0]*p._v[0] +
                                                p._v[1]*p._v[1] +
                                                p._v[2]*p._v[2]);
                p *= inv_length;
            }
        }
    }
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

    GLfloat *clusters = reinterpret_cast<GLfloat *>(_clusters->data());
    GLfloat *indices  = reinterpret_cast<GLfloat *>(_indices->data());

    for (int i = 0; i < (_n_htiles * _n_vtiles); ++i) {
        Subfrustum subfrustum = _subfrusta[i];
        subfrustum.plane[4] = near_plane;
        subfrustum.plane[5] = far_plane;

        GLint start_offset = _global_light_count;
        GLint local_point_count = 0;
        GLint local_spot_count = 0;

        // Test point lights
        for (GLushort point_iterator = 0;
             point_iterator < _point_bounds.size();
             ++point_iterator) {
            PointlightBound point = _point_bounds[point_iterator];

            // Perform frustum-sphere collision tests
            float distance = 0.0f;
            for (int n = 0; n < 6; ++n) {
                distance = subfrustum.plane[n] * point.position + point.range;
                if (distance <= 0.0f)
                    break;
            }

            if (distance > 0.0f) {
                indices[_global_light_count] = GLfloat(point_iterator);
                ++local_point_count;
                ++_global_light_count; // Atomic increment
            }

            if (_global_light_count >= (_max_light_indices * _max_light_indices)) {
                throw sg_range_exception(
                    "Clustered shading light index count is over the hardcoded limit ("
                    + std::to_string(_max_light_indices * _max_light_indices) + ")");
            }
        }

        // Test spot lights
        for (GLushort spot_iterator = 0;
             spot_iterator < _spot_bounds.size();
             ++spot_iterator) {
            SpotlightBound spot = _spot_bounds[spot_iterator];

            // Perform frustum-sphere collision tests
            float distance = 0.0f;
            for (int n = 0; n < 6; ++n) {
                distance = subfrustum.plane[n] * spot.bounding_sphere.center
                    + spot.bounding_sphere.radius;
                if (distance <= 0.0f)
                    break;
            }

            if (distance > 0.0f) {
                indices[_global_light_count] = GLfloat(spot_iterator);
                ++local_spot_count;
                ++_global_light_count; // Atomic increment
            }

            if (_global_light_count >= (_max_light_indices * _max_light_indices)) {
                throw sg_range_exception(
                    "Clustered shading light index count is over the hardcoded limit ("
                    + std::to_string(_max_light_indices * _max_light_indices) + ")");
            }
        }

        clusters[(z_offset + i) * 3 + 0] = GLfloat(start_offset);
        clusters[(z_offset + i) * 3 + 1] = GLfloat(local_point_count);
        clusters[(z_offset + i) * 3 + 2] = GLfloat(local_spot_count);
    }
}

void
ClusteredShading::writePointlightData()
{
    GLfloat *data = reinterpret_cast<GLfloat *>(_pointlights->data());

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
    _pointlights->dirty();
}

void
ClusteredShading::writeSpotlightData()
{
    GLfloat *data = reinterpret_cast<GLfloat *>(_spotlights->data());

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
        // Needs 2 float padding
        *data++ = 0.0f;
        *data++ = 0.0f;
    }
    _spotlights->dirty();
}

//------------------------------------------------------------------------------

void
ClusteredShading::writePointlightDataPBR()
{
    GLfloat *data = reinterpret_cast<GLfloat *>(_pointlights->data());

    for (const auto &point : _point_bounds) {
        // vec3 position
        *data++ = point.position.x();
        *data++ = point.position.y();
        *data++ = point.position.z();
        // float range
        *data++ = point.light->getRange();
        // vec3 color
        *data++ = point.light->getColor().x();
        *data++ = point.light->getColor().y();
        *data++ = point.light->getColor().z();
        // float intensity
        *data++ = point.light->getIntensity();
        // No padding needed as the resulting size is a multiple of vec4
    }
    _pointlights->dirty();
}

void
ClusteredShading::writeSpotlightDataPBR()
{
    GLfloat *data = reinterpret_cast<GLfloat *>(_spotlights->data());

    for (const auto &spot : _spot_bounds) {
        // vec3 position
        *data++ = spot.position.x();
        *data++ = spot.position.y();
        *data++ = spot.position.z();
        // float range
        *data++ = spot.light->getRange();
        // vec3 direction
        *data++ = spot.direction.x();
        *data++ = spot.direction.y();
        *data++ = spot.direction.z();
        // float cos_cutoff
        *data++ = spot.cos_cutoff;
        // vec3 color
        *data++ = spot.light->getColor().x();
        *data++ = spot.light->getColor().y();
        *data++ = spot.light->getColor().z();
        // float intensity
        *data++ = spot.light->getIntensity();
        // float exponent
        *data++ = spot.light->getSpotExponent();
        // 3 float padding
        *data++ = 0.0f;
        *data++ = 0.0f;
        *data++ = 0.0f;
    }
    _spotlights->dirty();
}

//------------------------------------------------------------------------------

float
ClusteredShading::getDepthForSlice(int slice) const
{
    return _zNear * pow(_zFar / _zNear, float(slice) / _depth_slices);
}

} // namespace compositor
} // namespace simgear
