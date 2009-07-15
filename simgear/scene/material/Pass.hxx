// Copyright (C) 2008  Timothy Moore timoore@redhat.com
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

#ifndef SIMGEAR_PASS_HXX
#define SIMGEAR_PASS_HXX 1

#include <osg/ref_ptr>
#include <osg/StateSet>

namespace simgear
{

class Pass : public osg::StateSet
{
public:
    META_Object(simgear,Pass);
    Pass() {}
    Pass(const Pass& rhs,
         const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
};

}

#endif
