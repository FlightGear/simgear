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

#include "DeletionManager.hxx"

#include <OpenThreads/ScopedLock>
#include <osg/Node>
#include "OsgSingleton.hxx"

namespace simgear
{
using namespace osg;

bool DeletionManager::handle(const osgGA::GUIEventAdapter& ea,
                             osgGA::GUIActionAdapter&,
                             osg::Object*, osg::NodeVisitor*)
{
    if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        _staleObjects.resize(0);
    }
    return false;
}

void DeletionManager::addStaleObject(Referenced* obj)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    _staleObjects.push_back(obj);
}

void DeletionManager::uninstall(Node* node)
{
    node->removeEventCallback(instance());
}

void DeletionManager::install(Node* node)
{
    node->addEventCallback(instance());
}

DeletionManager* DeletionManager::instance()
{
    return SingletonRefPtr<DeletionManager>::instance();
}
}

