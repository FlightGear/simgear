// A group of 2D Canvas elements
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

#ifndef CANVAS_GROUP_HXX_
#define CANVAS_GROUP_HXX_

#include "CanvasElement.hxx"

#include <list>

namespace simgear
{
namespace canvas
{

  typedef std::map<std::string, ElementFactory> ElementFactories;

  class Group:
    public Element
  {
    public:
      static const std::string TYPE_NAME;
      static void staticInit();

      typedef std::list< std::pair< const SGPropertyNode*,
                                    ElementPtr
                                  >
                       > ChildList;

      Group( const CanvasWeakPtr& canvas,
             const SGPropertyNode_ptr& node,
             const Style& parent_style = Style(),
             Element* parent = 0 );
      virtual ~Group();

      ElementPtr createChild( const std::string& type,
                              const std::string& id = "" );
      ElementPtr getChild(const SGPropertyNode* node);
      ElementPtr getChild(const std::string& id);
      ElementPtr getOrCreateChild( const std::string& type,
                                   const std::string& id );

      template<class T>
      SGSharedPtr<T> createChild(const std::string& id = "")
      {
        return dynamic_cast<T*>( createChild(T::TYPE_NAME, id).get() );
      }

      template<class T>
      SGSharedPtr<T> getChild(const SGPropertyNode* node)
      {
        return dynamic_cast<T*>( getChild(node).get() );
      }

      template<class T>
      SGSharedPtr<T> getChild(const std::string& id)
      {
        return dynamic_cast<T*>( getChild(id).get() );
      }

      template<class T>
      SGSharedPtr<T> getOrCreateChild(const std::string& id)
      {
        return dynamic_cast<T*>( getOrCreateChild(T::TYPE_NAME, id).get() );
      }

      /**
       * Get first child with given id (breadth-first search)
       *
       * @param id  Id (value if property node 'id') of element
       */
      ElementPtr getElementById(const std::string& id);

      virtual void clearEventListener();

      virtual void update(double dt);

      virtual bool traverse(EventVisitor& visitor);

      virtual bool setStyle( const SGPropertyNode* child,
                             const StyleInfo* style_info = 0 );

      virtual osg::BoundingBox getTransformedBounds(const osg::Matrix& m) const;

    protected:

      static ElementFactories   _child_factories;

      /**
       * Overload in derived classes to allow for more/other types of elements
       * to be managed.
       */
      virtual ElementFactory getChildFactory(const std::string& type) const;

      virtual void childAdded(SGPropertyNode * child);
      virtual void childRemoved(SGPropertyNode * child);
      virtual void childChanged(SGPropertyNode * child);

      void handleZIndexChanged(ElementPtr child, int z_index = 0);

      ElementPtr getChildByIndex(size_t index) const;
      ElementPtr findChild( const SGPropertyNode* node,
                            const std::string& id ) const;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_GROUP_HXX_ */
