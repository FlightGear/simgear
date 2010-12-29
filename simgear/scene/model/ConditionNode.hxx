// Copyright (C) 2010 Tim Moore (timoore33@gmail.com)
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef SIMGEAR_CONDITIONNODE_HXX
#define SIMGEAR_CONDITIONNODE_HXX 1

#include <simgear/props/condition.hxx>
#include <osg/Group>

namespace simgear
{
/**
 * If the condition is true, traverse the first child; otherwise,
 * traverse the second if it exists.
 */
class ConditionNode : public osg::Group
{
public:
    ConditionNode();
    ConditionNode(const ConditionNode& rhs,
              const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);
    META_Node(simgear,ConditionNode);
    ~ConditionNode();
    const SGCondition* getCondition() const { return _condition.ptr(); }
    void setCondition(const SGCondition* condition) { _condition = condition; }

    virtual void traverse(osg::NodeVisitor& nv);
protected:
    SGSharedPtr<SGCondition const> _condition;
};

}
#endif
