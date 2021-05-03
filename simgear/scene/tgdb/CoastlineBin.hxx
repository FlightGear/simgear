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

#ifndef COAST_BIN_HXX
#define COAST_BIN_HXX

#include <vector>
#include <string>

#include <osg/Geometry>
#include <osg/Group>
#include <osg/Matrix>

#include <simgear/misc/sg_path.hxx>

namespace simgear
{
class CoastlineBin {
public:
    CoastlineBin() = default;
    CoastlineBin(const SGPath& absoluteFileName);

    ~CoastlineBin() = default;

    struct Coastline {
        const std::list<osg::Vec3d> _nodes;
        Coastline(const std::list<osg::Vec3d> nodes) :
          _nodes(nodes)
        {
        }
    };

    typedef std::list<Coastline> CoastlineList;

    void insert(const Coastline& t) { 
        _coastFeatureList.push_back(t); 
    }

    const CoastlineList getCoastlines() const {
        return _coastFeatureList;
    }

private:
    CoastlineList _coastFeatureList;
};

typedef std::list<CoastlineBin> CoastlineBinList;

}

#endif
