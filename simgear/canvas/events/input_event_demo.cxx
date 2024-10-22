// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas keyboard event demo. Press some keys and get some info.
 */

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
