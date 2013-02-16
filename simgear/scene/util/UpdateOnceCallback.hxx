// UpdateOnceCallback.hxx
//
// Copyright (C) 2009  Tim Moore timoore@redhat.com
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
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
// Boston, MA  02110-1301, USA.

#ifndef SIMGEAR_UPDATEONCECALLBACK_HXX
#define SIMGEAR_UPDATEONCECALLBACK_HXX 1
#include <osg/NodeCallback>

namespace simgear
{
class UpdateOnceCallback : public osg::NodeCallback
{
public:
    UpdateOnceCallback() {}
    UpdateOnceCallback(const UpdateOnceCallback& nc, const osg::CopyOp& copyop)
        : osg::NodeCallback(nc, copyop)
    {
    }

    META_Object(simgear,UpdateOnceCallback);

    virtual void doUpdate(osg::Node* node, osg::NodeVisitor* nv);
    /**
     * Do not override; use doUpdate instead!
     */
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
};
}
#endif
