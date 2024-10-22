// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas forward declarations
 */

#ifndef SG_CANVAS_FWD_HXX_
#define SG_CANVAS_FWD_HXX_

#include <simgear/props/propsfwd.hxx>
#include <simgear/structure/SGWeakPtr.hxx>

#include <osg/ref_ptr>
#include <osgText/Font>

#include <functional>
#include <map>
#include <memory>
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
  typedef std::shared_ptr<name> name##Ptr;\
  typedef std::weak_ptr<name> name##WeakPtr;

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
  typedef std::function<Placements( SGPropertyNode*,
                                    CanvasPtr )> PlacementFactory;

  typedef std::function<void(const EventPtr&)> EventListener;

} // namespace canvas
} // namespace simgear


#endif /* SG_CANVAS_FWD_HXX_ */
