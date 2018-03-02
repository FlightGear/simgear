///@file
/// Keyboard event demo. Press some keys and get some info...
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
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
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include <simgear_config.h>

#include "KeyboardEvent.hxx"

#include <osgViewer/Viewer>
#include <iostream>

class DemoEventHandler:
  public osgGA::GUIEventHandler
{
  public:
    bool handle( const osgGA::GUIEventAdapter& ea,
                 osgGA::GUIActionAdapter&,
                 osg::Object*,
                 osg::NodeVisitor* )
    {
      switch( ea.getEventType() )
      {
        case osgGA::GUIEventAdapter::PUSH:
        case osgGA::GUIEventAdapter::RELEASE:
        case osgGA::GUIEventAdapter::DRAG:
        case osgGA::GUIEventAdapter::MOVE:
        case osgGA::GUIEventAdapter::SCROLL:
          return handleMouse(ea);
        case osgGA::GUIEventAdapter::KEYDOWN:
        case osgGA::GUIEventAdapter::KEYUP:
          return handleKeyboard(ea);
        default:
          return false;
      }
    }

  protected:
    bool handleMouse(const osgGA::GUIEventAdapter&)
    {
      return false;
    }

    bool handleKeyboard(const osgGA::GUIEventAdapter& ea)
    {
      simgear::canvas::KeyboardEvent evt(ea);
      std::cout << evt.getTypeString() << " '" << evt.key() << "'"
                                       << ", loc=" << evt.location()
                                       << ", char=" << evt.charCode()
                                       << ", key=" << evt.keyCode()
                                       << (evt.isPrint() ? ", printable" : "")
                                       << std::endl;
      return true;
    }
};

int main()
{
  osgViewer::Viewer viewer;

  osg::ref_ptr<DemoEventHandler> handler( new DemoEventHandler );
  viewer.addEventHandler(handler);

  viewer.setUpViewInWindow(100, 100, 200, 100, 0);
  viewer.setRunMaxFrameRate(5);
  return viewer.run();
}
