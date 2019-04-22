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

#include "ClusteredForward.hxx"

#include <osg/BufferIndexBinding>
#include <osg/BufferObject>
#include <osg/RenderInfo>
#include <osg/Texture3D>
#include <osg/TextureBuffer>
#include <osg/Version>

namespace simgear {
namespace compositor {

///// BEGIN DEBUG
#define DATA_SIZE 24
const GLfloat LIGHT_DATA[DATA_SIZE] = {
    0.0, 0.0, -10.0, 1.0,   1.0, 0.0, 0.0, 1.0,
    0.0, 0.0, 10.0, 1.0,   0.0, 1.0, 0.0, 1.0,
    0.0, 1.0, 1.0, 1.0,   1.0, 1.0, 0.0, 1.0
};

#define MAX_LIGHT_INDICES 4096
#define MAX_POINT_LIGHTS 256

struct Light {
    osg::Vec3 position;
    float range;
};

#define NUM_LIGHTS 2
Light LIGHT_LIST[NUM_LIGHTS] = {
    {osg::Vec3(0.0f, 0.0f, -10.0f), 10.0f},
    {osg::Vec3(0.0f, 0.0f,  5.0f), 1000.0f}
};
///// END DEBUG

ClusteredForwardDrawCallback::ClusteredForwardDrawCallback() :
    _initialized(false),
    _tile_size(64),
    _light_grid(new osg::Image),
    _light_indices(new osg::Image),
    _light_data(new osg::FloatArray(MAX_POINT_LIGHTS))
{
}

void
ClusteredForwardDrawCallback::operator()(osg::RenderInfo &renderInfo) const
{
    osg::Camera *camera = renderInfo.getCurrentCamera();
    const osg::Viewport *vp = camera->getViewport();
    const int width = vp->width();
    const int height = vp->height();

    // Round up
    int n_htiles = (width  + _tile_size - 1) / _tile_size;
    int n_vtiles = (height + _tile_size - 1) / _tile_size;

    if (!_initialized) {
        // Create and associate the light grid 3D texture
        _light_grid->allocateImage(n_htiles, n_vtiles, 1,
                                   GL_RGB_INTEGER_EXT, GL_UNSIGNED_SHORT);
        _light_grid->setInternalTextureFormat(GL_RGB16UI_EXT);

        osg::ref_ptr<osg::Texture3D> light_grid_tex = new osg::Texture3D;
        light_grid_tex->setResizeNonPowerOfTwoHint(false);
        light_grid_tex->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
        light_grid_tex->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::CLAMP_TO_BORDER);
        light_grid_tex->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::CLAMP_TO_BORDER);
        light_grid_tex->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::NEAREST);
        light_grid_tex->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::NEAREST);
        light_grid_tex->setImage(0, _light_grid.get());

        camera->getOrCreateStateSet()->setTextureAttributeAndModes(
            10, light_grid_tex.get(), osg::StateAttribute::ON);

        // Create and associate the light indices TBO
        _light_indices->allocateImage(4096, 1, 1, GL_RED_INTEGER_EXT, GL_UNSIGNED_SHORT);

        osg::ref_ptr<osg::TextureBuffer> light_indices_tbo =
            new osg::TextureBuffer;
        light_indices_tbo->setInternalFormat(GL_R16UI);
        light_indices_tbo->setImage(_light_indices.get());

        camera->getOrCreateStateSet()->setTextureAttribute(
            11, light_indices_tbo.get());

        // Create and associate the light data UBO
        osg::ref_ptr<osg::UniformBufferObject> light_data_ubo =
            new osg::UniformBufferObject;
        _light_data->setBufferObject(light_data_ubo.get());

#if OSG_VERSION_LESS_THAN(3,6,0)
         osg::ref_ptr<osg::UniformBufferBinding> light_data_ubb =
            new osg::UniformBufferBinding(0, light_data_ubo.get(),
                                          0, MAX_POINT_LIGHTS * 8 * sizeof(GLfloat));
#else
         osg::ref_ptr<osg::UniformBufferBinding> light_data_ubb =
            new osg::UniformBufferBinding(0, _light_data.get(),
                                          0, MAX_POINT_LIGHTS * 8 * sizeof(GLfloat));
#endif
light_data_ubb->setDataVariance(osg::Object::DYNAMIC);

        camera->getOrCreateStateSet()->setAttribute(
            light_data_ubb.get(), osg::StateAttribute::ON);

        _initialized = true;
    }

    std::vector<osg::Polytope> subfrustums;
    const osg::Matrix &view_matrix = camera->getViewMatrix();
    const osg::Matrix &proj_matrix = camera->getProjectionMatrix();
    osg::Matrix view_proj_inverse = osg::Matrix::inverse(view_matrix * proj_matrix);

    double x_step = (_tile_size / width) * 2.0;
    double y_step = (_tile_size / height) * 2.0;
    for (int y = 0; y < n_vtiles; ++y) {
        for (int x = 0; x < n_htiles; ++x) {
            // Create the subfrustum in clip space
            double x_min = -1.0 + x_step * x; double x_max = x_min + x_step;
            double y_min = -1.0 + y_step * y; double y_max = y_min + y_step;
            double z_min = 1.0;               double z_max = -1.0;
            osg::BoundingBox subfrustum_bb(
                x_min, y_min, z_min, x_max, y_max, z_max);
            osg::Polytope subfrustum;
            subfrustum.setToBoundingBox(subfrustum_bb);

            // Transform it to world space
            subfrustum.transformProvidingInverse(view_proj_inverse);

            subfrustums.push_back(subfrustum);
        }
    }

    GLushort *grid_data = reinterpret_cast<GLushort *>
        (_light_grid->data());
    GLushort *index_data = reinterpret_cast<GLushort *>
        (_light_indices->data());

    GLushort global_light_count = 0;
    for (size_t i = 0; i < subfrustums.size(); ++i) {
        GLushort start_offset = global_light_count;
        GLushort local_light_count = 0;

        for (GLushort light_list_index = 0;
             light_list_index < NUM_LIGHTS;
             ++light_list_index) {
            const Light &light = LIGHT_LIST[light_list_index];
            osg::BoundingSphere bs(light.position, light.range);

            if (subfrustums[i].contains(bs)) {
                index_data[global_light_count] = light_list_index;
                ++local_light_count;
                ++global_light_count;
            }
        }
        grid_data[i * 3 + 0] = start_offset;
        grid_data[i * 3 + 1] = local_light_count;
        grid_data[i * 3 + 2] = 0;
    }

    _light_grid->dirty();
    _light_indices->dirty();

    // Upload light data
    for (int i = 0; i < DATA_SIZE; ++i) {
        (*_light_data)[i] = LIGHT_DATA[i];
    }

            // DEBUG
        /*
        if (!_debug) {
            for (int y = 0; y < num_vtiles; ++y) {
                for (int x = 0; x < num_htiles; ++x) {
                    std::cout << grid_data[(y * num_htiles + x) * 3 + 0] << ","
                              << grid_data[(y * num_htiles + x) * 3 + 1] << " ";
                }
                std::cout << std::endl;
            }
            std::cout << "\n\n";

            for (int i = 0; i < num_vtiles * num_htiles; ++i) {
                std::cout << index_data[i] << " ";
            }
            std::cout << "\n";
            _debug = true;
        }
        */
/*
        for (int y = 0; y < num_vtiles; ++y) {
            for (int x = 0; x < num_htiles; ++x) {
                data[(y * num_htiles + x) * 3 + 0] = (unsigned short)x;
                data[(y * num_htiles + x) * 3 + 1] = (unsigned short)y;
                data[(y * num_htiles + x) * 3 + 2] = 0;
            }
        }
        _light_grid->dirty();
*/
}

} // namespace compositor
} // namespace simgear
