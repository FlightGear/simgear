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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <osg/Transform>
#include <osg/ProxyNode>
#include <osgDB/DatabasePager>

#include <simgear/debug/logstream.hxx>

#include "CheckSceneryVisitor.hxx"

using namespace simgear;

CheckSceneryVisitor::CheckSceneryVisitor(osgDB::DatabasePager* dbp, const osg::Vec3 &position, double range,
                                         osg::FrameStamp* framestamp)
:osg::NodeVisitor(osg::NodeVisitor::NODE_VISITOR,
                  osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
 _position(position), _range(range), _loaded(true), _matrix(osg::Matrix::identity())
{
    setDatabaseRequestHandler(dbp);
    setFrameStamp(framestamp);
}

CheckSceneryVisitor::~CheckSceneryVisitor()
{
}

void CheckSceneryVisitor::apply(osg::Node& node)
{
    traverse(node);
}

void CheckSceneryVisitor::apply(osg::ProxyNode& node)
{
    osg::Vec3 pos = node.getCenter() * _matrix;
    double dist = (pos - _position).length();
    if (dist < _range) {
        for (unsigned i = 0; i < node.getNumFileNames(); ++i) {
            if (node.getFileName(i).empty())
                continue;
            // Check if this is already loaded.
            if (i < node.getNumChildren() && node.getChild(i))
                continue;

            // if the DatabasePager would load LODs while the splashscreen
            // is there, we could just wait for the models to be loaded
            // by only setting setLoaded(false) here
            osg::NodePath nodePath = getNodePath();
            DatabaseRequestHandler* db = getDatabaseRequestHandler();
            const osg::FrameStamp* fs = getFrameStamp();
            db->requestNodeFile(node.getFileName(i), nodePath, 1.0 /*priority*/, fs,
                                node.getDatabaseRequest(i), node.getDatabaseOptions());
            setLoaded(false);
        }
    }
    traverse(node);
}

void CheckSceneryVisitor::apply(osg::PagedLOD& node)
{
    osg::Vec3 pos = node.getCenter() * _matrix;
    double dist = (pos - _position).length();
    if (dist < _range) {
        for (unsigned i = 0; i < node.getNumFileNames(); ++i) {
            if (node.getFileName(i).empty())
                continue;
            // Check if this is already loaded.
            if (i < node.getNumChildren() && node.getChild(i))
                continue;

            // if the DatabasePager would load LODs while the splashscreen
            // is there, we could just wait for the models to be loaded
            // by only setting setLoaded(false) here
            osg::NodePath nodePath = getNodePath();
            DatabaseRequestHandler* db = getDatabaseRequestHandler();
            const osg::FrameStamp* fs = getFrameStamp();
            db->requestNodeFile(node.getFileName(i), nodePath, 1.0 /*priority*/, fs,
                                node.getDatabaseRequest(i), node.getDatabaseOptions());
            setLoaded(false);
        }
    }
    traverse(node);
}

void CheckSceneryVisitor::apply(osg::Transform &node)
{
    osg::Matrix matrix = _matrix;
    node.computeLocalToWorldMatrix(_matrix, this);
    traverse(node);
    _matrix = matrix;
}
