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

#ifndef ROAD_BIN_HXX
#define ROAD_BIN_HXX

#include <vector>
#include <string>

#include <osg/Geometry>
#include <osg/Group>
#include <osg/Matrix>

#include <simgear/misc/sg_path.hxx>

namespace simgear
{
class LineFeatureBin {
public:
    LineFeatureBin() = default;
    LineFeatureBin(const SGPath& absoluteFileName, const std::string material);

    ~LineFeatureBin() = default;

    struct LineFeature {
        const std::list<osg::Vec3d> _nodes;
        const float _width;
        const int _attributes;
        const float _a;
        const float _b;
        const float _c;
        const float _d;
        LineFeature(const std::list<osg::Vec3d> nodes, const float w, const int attributes, const float a, const float b, const float c, const float d) :
          _nodes(nodes), _width(w), _attributes(attributes), _a(a), _b(b), _c(c), _d(d)
        {
        }
    };

    typedef std::list<LineFeature> LineFeatureList;

    void insert(const LineFeature& t) { 
        _lineFeatureList.push_back(t); 
    }

    const LineFeatureList getLineFeatures() const {
        return _lineFeatureList;
    }

    const std::string getMaterial() const { 
        return _material; 
    }

private:
    LineFeatureList _lineFeatureList;
    const std::string _material;
};

typedef std::list<LineFeatureBin> LineFeatureBinList;

}

#endif
