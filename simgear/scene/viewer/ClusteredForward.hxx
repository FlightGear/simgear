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

#ifndef SG_CLUSTERED_FORWARD_HXX
#define SG_CLUSTERED_FORWARD_HXX

#include <osg/Camera>

namespace simgear {
namespace compositor {

class ClusteredForwardDrawCallback : public osg::Camera::DrawCallback {
public:
    ClusteredForwardDrawCallback();
    virtual void operator()(osg::RenderInfo &renderInfo) const;
protected:
    mutable bool                   _initialized;
    int                            _tile_size;
    osg::ref_ptr<osg::Image>       _light_grid;
    osg::ref_ptr<osg::Image>       _light_indices;
    osg::ref_ptr<osg::FloatArray>  _light_data;
};

} // namespace compositor
} // namespace simgear

#endif /* SG_CLUSTERED_FORWARD_HXX */
