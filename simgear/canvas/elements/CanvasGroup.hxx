// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief A group of 2D Canvas elements
 */

#ifndef CANVAS_GROUP_HXX_
#define CANVAS_GROUP_HXX_

#include "CanvasElement.hxx"

#include <list>

namespace simgear
{
namespace canvas
{

  typedef std::map<std::string, ElementFactory> ElementFactories;

  // forward decls
  class FocusScope;

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
             ElementWeakPtr parent = 0 );
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

      void clearEventListener() override;

      bool traverse(EventVisitor& visitor) override;

      bool handleEvent(const EventPtr& event) override;

      bool setStyle( const SGPropertyNode* child,
                     const StyleInfo* style_info = 0 ) override;

      osg::BoundingBox
      getTransformedBounds(const osg::Matrix& m) const override;


      FocusScope* getOrCreateFocusScope();
      FocusScope* getFocusScope() const;

  protected:

      static ElementFactories   _child_factories;

      /**
       * Overload in derived classes to allow for more/other types of elements
       * to be managed.
       */
      virtual ElementFactory getChildFactory(const std::string& type) const;

      void updateImpl(double dt) override;

      void childAdded(SGPropertyNode * child) override;
      void childRemoved(SGPropertyNode * child) override;
      void childChanged(SGPropertyNode * child) override;

      void handleZIndexChanged(ElementPtr child, int z_index = 0);

      ElementPtr getChildByIndex(size_t index) const;
      ElementPtr findChild( const SGPropertyNode* node,
                            const std::string& id ) const;

      std::unique_ptr<FocusScope> _focus_scope;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_GROUP_HXX_ */
