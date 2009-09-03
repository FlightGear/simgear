// Copyright (C) 2008 Till Busch buti@bux.at
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

#include <simgear/debug/logstream.hxx>

#include "CheckSceneryVisitor.hxx"
#include "SGPagedLOD.hxx"

#include <simgear/math/SGMath.hxx>

using namespace simgear;

CheckSceneryVisitor::CheckSceneryVisitor(osgDB::DatabasePager* dbp, const osg::Vec3 &position, double range)
:osg::NodeVisitor(osg::NodeVisitor::NODE_VISITOR,
                  osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
_position(position), _range(range), _loaded(true), _dbp(dbp)
{
    _viewMatrices.push_back(osg::Matrix::identity());
}

void CheckSceneryVisitor::apply(osg::Node& node)
{
    traverse(node);
}

void CheckSceneryVisitor::apply(osg::PagedLOD& node)
{
    SGPagedLOD *sgplod = dynamic_cast<SGPagedLOD*>(&node);
    if (sgplod) {
        osg::Vec3 pos = sgplod->getCenter() * _viewMatrices.back();
        double dist = (pos-_position).length();
        if (dist < _range) {
            if (sgplod->getNumChildren() < 1) {
                // if the DatabasePager would load LODs while the splashscreen
                // is there, we could just wait for the models to be loaded
                // by only setting setLoaded(false) here
                sgplod->forceLoad(_dbp);
                setLoaded(false);
            }
        }
    }
    traverse(node);
}

void CheckSceneryVisitor::apply(osg::Transform &node)
{
    osg::Matrix currMatrix = _viewMatrices.back();
    bool pushMatrix = node.computeLocalToWorldMatrix(currMatrix, this);

    if (pushMatrix) {
        _viewMatrices.push_back(currMatrix);
    }
    traverse(node);
    if (pushMatrix) {
        _viewMatrices.pop_back();
    }
}
