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
// Boston, MA  02110-1301, USA

#include "UpdateOnceCallback.hxx"

#include <osg/Node>
#include <osg/NodeVisitor>

namespace simgear
{
using namespace osg;

void UpdateOnceCallback::operator()(Node* node, NodeVisitor* nv)
{
    ref_ptr<UpdateOnceCallback> prevent_premature_deletion=this;
    // workaround for crash bug in OSG 3.2.1
    // https://bugs.debian.org/765855
    doUpdate(node, nv);
    node->removeUpdateCallback(this);
    // The callback could be deleted now.
}

void UpdateOnceCallback::doUpdate(Node* node, NodeVisitor* nv)
{
    traverse(node, nv);
}
}
