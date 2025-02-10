// SPDX-FileCopyrightText: Copyright (C) 2017 Richard Harrison
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "FindGroupVisitor.hxx"

#include <simgear/debug/logstream.hxx>


namespace simgear {

FindGroupVisitor::FindGroupVisitor(const std::string& name) :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
    _name(name),
    _duplicates(false)
{
    if (name.empty())
        SG_LOG(SG_IO, SG_DEV_WARN, "FindGroupVisitor: empty name provided");
}

void FindGroupVisitor::apply(osg::Group& group)
{
    if (_name != group.getName())
        return traverse(group);

    if (!_group)
        _group = &group;
    else if (_group != &group) {
        _duplicates = true;
        SG_LOG(SG_IO, SG_DEV_WARN, "FindGroupVisitor: name not unique '" << _name << "'");
    }
}

} // namespace simgear
