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

#ifndef SG_CLUSTERED_SHADING_HXX
#define SG_CLUSTERED_SHADING_HXX

#include <atomic>

#include <osg/Camera>
#include <osg/Uniform>

#include <simgear/scene/model/SGLight.hxx>

namespace simgear {
namespace compositor {

class ClusteredShading : public osg::Referenced {
public:
    ClusteredShading(osg::Camera *camera, const SGPropertyNode *config);
    ~ClusteredShading();

    void update(const SGLightList &light_list);
protected:
    // We could make use of osg::Polytope, but it does a lot of std::vector
    // push_back() calls, so we make our own frustum structure for huge
    // performance gains.
    struct Subfrustum {
        osg::Vec4f plane[6];
    };

    struct PointlightBound {
        SGLight *light = nullptr;
        osg::Vec4f position;
        float range = 0.0f;
    };
    struct SpotlightBound {
        SGLight *light = nullptr;
        osg::Vec4f position;
        osg::Vec4f direction;
        float cos_cutoff = 0.0f;
        struct {
            osg::Vec4f center;
            float radius = 0.0f;
        } bounding_sphere;
    };

    void threadFunc(int thread_id);
    void assignLightsToSlice(int slice);
    void writePointlightData();
    void writeSpotlightData();
    float getDepthForSlice(int slice) const;

    osg::observer_ptr<osg::Camera>  _camera;

    osg::ref_ptr<osg::Uniform>      _slice_scale;
    osg::ref_ptr<osg::Uniform>      _slice_bias;

    int                             _tile_size;
    int                             _depth_slices;
    int                             _num_threads;
    int                             _slices_per_thread;
    int                             _slices_remainder;

    float                           _zNear;
    float                           _zFar;

    int                             _n_htiles;
    int                             _n_vtiles;

    float                           _x_step;
    float                           _y_step;

    osg::ref_ptr<osg::Image>        _light_grid;
    osg::ref_ptr<osg::Image>        _light_indices;
    osg::ref_ptr<osg::FloatArray>   _pointlight_data;
    osg::ref_ptr<osg::FloatArray>   _spotlight_data;

    std::unique_ptr<Subfrustum[]>   _subfrusta;

    std::vector<PointlightBound>    _point_bounds;
    std::vector<SpotlightBound>     _spot_bounds;

    std::atomic<GLuint>             _global_light_count;
};

} // namespace compositor
} // namespace simgear

#endif /* SG_CLUSTERED_SHADING_HXX */
