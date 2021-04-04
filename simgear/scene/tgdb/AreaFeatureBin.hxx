/* -*-c++-*-
 *
 * Copyright (C) 2021 Stuart Buchanan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef AREA_BIN_HXX
#define AREA_BIN_HXX

#include <vector>
#include <string>

#include <osg/Geometry>
#include <osg/Group>
#include <osg/Matrix>

#include <simgear/misc/sg_path.hxx>

namespace simgear
{
class AreaFeatureBin {
public:
    AreaFeatureBin() = default;
    AreaFeatureBin(const SGPath& absoluteFileName, const std::string material);

    ~AreaFeatureBin() = default;

    struct AreaFeature {
        const std::list<osg::Vec3d> _nodes;
        const float _area;
        const int _attributes;
        const float _a;
        const float _b;
        const float _c;
        const float _d;
        AreaFeature(const std::list<osg::Vec3d> nodes, const float area, const int attributes, const float a, const float b, const float c, const float d) :
          _nodes(nodes), _area(area), _attributes(attributes), _a(a), _b(b), _c(c), _d(d)
        {
        }
    };

    typedef std::list<AreaFeature> AreaFeatureList;

    void insert(const AreaFeature& t) { 
        _areaFeatureList.push_back(t); 
    }

    const AreaFeatureList getAreaFeatures() const {
        return _areaFeatureList;
    }

    const std::string getMaterial() const { 
        return _material; 
    }

private:
    AreaFeatureList _areaFeatureList;
    const std::string _material;
};

typedef std::list<AreaFeatureBin> AreaFeatureBinList;

}

#endif
