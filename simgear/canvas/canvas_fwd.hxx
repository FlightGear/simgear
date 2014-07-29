// Canvas forward declarations
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SG_CANVAS_FWD_HXX_
#define SG_CANVAS_FWD_HXX_

#include <simgear/props/propsfwd.hxx>
#include <simgear/structure/SGWeakPtr.hxx>

#include <osg/ref_ptr>
#include <osgText/Font>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <map>
#include <vector>

namespace simgear
{
namespace canvas
{

#define SG_FWD_DECL(name)\
  class name;\
  typedef SGSharedPtr<name> name##Ptr;\
  typedef SGWeakPtr<name> name##WeakPtr;

  SG_FWD_DECL(Canvas)
  SG_FWD_DECL(Element)
  SG_FWD_DECL(Group)
  SG_FWD_DECL(Image)
  SG_FWD_DECL(Map)
  SG_FWD_DECL(Path)
  SG_FWD_DECL(Text)
  SG_FWD_DECL(Window)

  SG_FWD_DECL(Event)
  SG_FWD_DECL(CustomEvent)
  SG_FWD_DECL(DeviceEvent)
  SG_FWD_DECL(KeyboardEvent)
  SG_FWD_DECL(MouseEvent)

#undef SG_FWD_DECL

#define SG_FWD_DECL(name)\
  class name;\
  typedef boost::shared_ptr<name> name##Ptr;\
  typedef boost::weak_ptr<name> name##WeakPtr;

  SG_FWD_DECL(Placement)
  SG_FWD_DECL(SystemAdapter)

#undef SG_FWD_DECL

  class EventManager;
  class EventVisitor;

  struct EventTarget;
  typedef std::deque<EventTarget> EventPropagationPath;

  typedef std::map<std::string, const SGPropertyNode*> Style;
  typedef ElementPtr (*ElementFactory)( const CanvasWeakPtr&,
                                        const SGPropertyNode_ptr&,
                                        const Style&,
                                        Element* );

  typedef osg::ref_ptr<osgText::Font> FontPtr;

  typedef std::vector<PlacementPtr> Placements;
  typedef boost::function<Placements( SGPropertyNode*,
                                      CanvasPtr )> PlacementFactory;

  typedef boost::function<void(const EventPtr&)> EventListener;

} // namespace canvas
} // namespace simgear


#endif /* SG_CANVAS_FWD_HXX_ */
