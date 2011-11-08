// Copyright (C) 2008 Till Busch buti@bux.at
// Copyright (C) 2011 Mathias Froehlich
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef CHECKSCENERYVISITOR_HXX
#define CHECKSCENERYVISITOR_HXX

#include <osg/NodeVisitor>

namespace osgDB {
class DatabasePager;
}


namespace simgear
{

// Checks the scene graph for PagedLOD or ProxyNodes that are within range
// (compared to postion) and injects them into the DatabasePager.
// After visiting, isLoaded() returns true if all models in range
// are available.

class CheckSceneryVisitor : public osg::NodeVisitor
{
public:
    CheckSceneryVisitor(osgDB::DatabasePager* dbp, const osg::Vec3 &position, double range, osg::FrameStamp* framestamp);

    virtual void apply(osg::Node& node);
    virtual void apply(osg::ProxyNode& node);
    virtual void apply(osg::PagedLOD& node);
    virtual void apply(osg::Transform& node);

    bool isLoaded() const {
        return _loaded;
    }
    void setLoaded(bool l) {
        _loaded=l;
    }
    const osg::Vec3 &getPosition() const {
        return _position;
    }

private:
    osg::Vec3 _position;
    double _range;
    bool _loaded;
    osg::Matrix _matrix;
};

}

#endif
