// DeletionManager.hxx -- defer deletions to a safe time
//
// Copyright (C) 2012  Tim Moore timoore33@gmail.com
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

#ifndef SIMGEAR_DELETIONMANAGER_HXX
#define SIMGEAR_DELETIONMANAGER_HXX 1

#include <vector>

#include <OpenThreads/Mutex>
#include <osg/ref_ptr>
#include <osg/NodeCallback>
#include <osg/Referenced>
#include <osgGA/GUIEventHandler>

namespace simgear
{
class DeletionManager : public osgGA::GUIEventHandler
{
public:
    virtual bool handle(const osgGA::GUIEventAdapter& ea,
                        osgGA::GUIActionAdapter& aa,
                        osg::Object* object, osg::NodeVisitor* nv);
    void addStaleObject(osg::Referenced* obj);
    static void install(osg::Node* node);
    static void uninstall(osg::Node* node);
    static DeletionManager* instance();
protected:
    OpenThreads::Mutex _mutex;
    std::vector<osg::ref_ptr<osg::Referenced> > _staleObjects;
};
}

#endif
