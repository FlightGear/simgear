// CopyOp.cxx - Simgear CopyOp for copying our own classes
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "CopyOp.hxx"

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/Technique.hxx>

namespace simgear
{
osg::Object* CopyOp::operator()(const osg::Object* obj) const
{
    if (dynamic_cast<const Effect*>(obj)
        || dynamic_cast<const Technique*>(obj)) {
        if (_flags & DEEP_COPY_STATESETS)
            return obj->clone(*this);
        else
            return const_cast<osg::Object*>(obj);
    }
    else {
        return osg::CopyOp::operator()(obj);
    }
}
}
