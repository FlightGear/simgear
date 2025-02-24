// SPDX-FileCopyrightText: Copyright (C) 2017 Richard Harrison
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <osg/Group>
#include <osg/NodeVisitor>

namespace simgear
{

class FindGroupVisitor : public osg::NodeVisitor
{
public:
    FindGroupVisitor(const std::string& name);

    osg::Group* getGroup() const
    {
        return _group.get();
    }

    bool foundDuplicates() const
    {
        return _duplicates;
    }

    virtual void apply(osg::Group& group);

protected:
    std::string _name;
    osg::ref_ptr<osg::Group> _group;
    bool _duplicates;
};

} // namespace simgear
