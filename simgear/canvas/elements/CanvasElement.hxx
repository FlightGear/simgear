// Interface for 2D Canvas elements
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

#ifndef CANVAS_ELEMENT_HXX_
#define CANVAS_ELEMENT_HXX_

#include <simgear/canvas/canvas_fwd.hxx>
#include <simgear/canvas/CanvasEvent.hxx>
#include <simgear/props/PropertyBasedElement.hxx>
#include <simgear/misc/stdint.hxx> // for uint32_t
#include <simgear/nasal/cppbind/Ghost.hxx>

#include <osg/BoundingBox>
#include <osg/MatrixTransform>

#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace osg
{
  class Drawable;
}

namespace simgear
{
namespace canvas
{

  class Element:
    public PropertyBasedElement
  {
    public:

      /**
       * Store pointer to window as user data
       */
      class OSGUserData:
        public osg::Referenced
      {
        public:
          ElementPtr element;
          OSGUserData(ElementPtr element);
      };

      typedef boost::function<bool(Element&, const SGPropertyNode*)>
              StyleSetterFunc;
      typedef boost::function<void(Element&, const SGPropertyNode*)>
              StyleSetterFuncUnchecked;
      struct StyleSetter:
        public SGReferenced
      {
        StyleSetterFunc func;
        SGSharedPtr<StyleSetter> next;
      };
      struct StyleInfo
      {
        StyleSetter setter; ///!< Function(s) for setting this style
        std::string type;   ///!< Interpolation type
      };

      /**
       *
       */
      virtual ~Element() = 0;

      virtual void setSelf(const PropertyBasedElementPtr& self);
      virtual void onDestroy();

      ElementWeakPtr getWeakPtr() const;

      /**
       * Called every frame to update internal state
       *
       * @param dt  Frame time in seconds
       */
      virtual void update(double dt);

      naRef addEventListener(const nasal::CallContext& ctx);
      virtual void clearEventListener();

      virtual bool accept(EventVisitor& visitor);
      virtual bool ascend(EventVisitor& visitor);
      virtual bool traverse(EventVisitor& visitor);

      virtual bool handleEvent(canvas::EventPtr event);

      virtual bool hitBound( const osg::Vec2f& pos,
                             const osg::Vec2f& local_pos ) const;

      /**
       * Get whether the element is visible or hidden (Can be changed with
       * setting property "visible" accordingly).
       */
      bool isVisible() const;

      osg::MatrixTransform* getMatrixTransform();
      osg::MatrixTransform const* getMatrixTransform() const;

      virtual void childAdded( SGPropertyNode * parent,
                               SGPropertyNode * child );
      virtual void childRemoved( SGPropertyNode * parent,
                                 SGPropertyNode * child );
      virtual void valueChanged(SGPropertyNode * child);

      virtual bool setStyle(const SGPropertyNode* child);

      /**
       * Set clipping shape
       *
       * @note Only "rect(<top>, <right>, <bottom>, <left>)" is supported
       * @see http://www.w3.org/TR/CSS21/visufx.html#propdef-clip
       */
      void setClip(const std::string& clip);

      /**
       * Write the given bounding box to the property tree
       */
      void setBoundingBox(const osg::BoundingBox& bb);

      /**
       * Get bounding box with children/drawables transformed by passed matrix
       */
      virtual osg::BoundingBox getTransformedBounds(const osg::Matrix& m) const;

    protected:

      enum Attributes
      {
        BLEND_FUNC = 0x0001,
        LAST_ATTRIBUTE  = BLEND_FUNC << 1
      };

      enum TransformType
      {
        TT_NONE,
        TT_MATRIX,
        TT_TRANSLATE,
        TT_ROTATE,
        TT_SCALE
      };

      CanvasWeakPtr _canvas;
      Element      *_parent;

      uint32_t _attributes_dirty;

      bool _transform_dirty;
      osg::observer_ptr<osg::MatrixTransform> _transform;
      std::vector<TransformType>              _transform_types;

      Style                             _style;
      std::vector<SGPropertyNode_ptr>   _bounding_box;

      typedef std::vector<EventListenerPtr> Listener;
      typedef std::map<Event::Type, Listener> ListenerMap;

      ListenerMap _listener;

      typedef std::map<std::string, StyleInfo> StyleSetters;
      static StyleSetters       _style_setters;

      Element( const CanvasWeakPtr& canvas,
               const SGPropertyNode_ptr& node,
               const Style& parent_style,
               Element* parent );

      /**
       * Returns false on first call and true on any successive call. Use to
       * perform initialization tasks which are only required once per element
       * type.
       *
       * @tparam Derived    (Derived) class type
       */
      template<class Derived>
      bool isInit() const
      {
        static bool is_init = false;
        if( is_init )
          return true;

        is_init = true;
        return false;
      }

      /**
       * Register a function for setting a style specified by the given property
       *
       * @param name    Property name
       * @param type    Interpolation type
       * @param setter  Setter function
       *
       * @tparam T1         Type of value used to retrieve value from property
       *                    node
       * @tparam T2         Type of value the setter function expects
       * @tparam Derived    Type of class the setter can be applied to
       *
       * @note T1 needs to be convertible to T2
       */
      template<
        typename T1,
        typename T2,
        class Derived
      >
      StyleSetter
      addStyle( const std::string& name,
                const std::string& type,
                const boost::function<void (Derived&, T2)>& setter )
      {
        StyleInfo& style_info = _style_setters[ name ];
        if( !type.empty() )
        {
          if( !style_info.type.empty() && type != style_info.type )
            SG_LOG
            (
              SG_GENERAL,
              SG_WARN,
              "changing animation type for '" << name << "': "
                << style_info.type << " -> " << type
            );

          style_info.type = type;
        }

        StyleSetter* style = &style_info.setter;
        while( style->next )
          style = style->next;
        if( style->func )
          style = style->next = new StyleSetter;

        style->func = boost::bind
        (
          &type_match<Derived>::call,
          _1,
          _2,
          bindStyleSetter<T1>(name, setter)
        );
        return *style;
      }

      template<
        typename T,
        class Derived
      >
      StyleSetter
      addStyle( const std::string& name,
                const std::string& type,
                const boost::function<void (Derived&, T)>& setter )
      {
        return addStyle<T, T>(name, type, setter);
      }

      template<
        typename T,
        class Derived
      >
      StyleSetter
      addStyle( const std::string& name,
                const std::string& type,
                void (Derived::*setter)(T) )
      {
        return addStyle<T, T>
        (
          name,
          type,
          boost::function<void (Derived&, T)>(setter)
        );
      }

      template<
        typename T1,
        typename T2,
        class Derived
      >
      StyleSetterFunc
      addStyle( const std::string& name,
                const std::string& type,
                void (Derived::*setter)(T2) )
      {
        return addStyle<T1>
        (
          name,
          type,
          boost::function<void (Derived&, T2)>(setter)
        );
      }

      template<
        class Derived
      >
      StyleSetter
      addStyle( const std::string& name,
                const std::string& type,
                void (Derived::*setter)(const std::string&) )
      {
        return addStyle<const char*, const std::string&>
        (
          name,
          type,
          boost::function<void (Derived&, const std::string&)>(setter)
        );
      }

      template<
        typename T,
        class Derived,
        class Other,
        class OtherRef
      >
      StyleSetter
      addStyle( const std::string& name,
                const std::string& type,
                void (Other::*setter)(T),
                OtherRef Derived::*instance_ref )
      {
        return addStyle<T, T>(name, type, bindOther(setter, instance_ref));
      }

      template<
        typename T1,
        typename T2,
        class Derived,
        class Other,
        class OtherRef
      >
      StyleSetter
      addStyle( const std::string& name,
                const std::string& type,
                void (Other::*setter)(T2),
                OtherRef Derived::*instance_ref )
      {
        return addStyle<T1>(name, type, bindOther(setter, instance_ref));
      }

      template<
        typename T1,
        typename T2,
        class Derived,
        class Other,
        class OtherRef
      >
      StyleSetter
      addStyle( const std::string& name,
                const std::string& type,
                const boost::function<void (Other&, T2)>& setter,
                OtherRef Derived::*instance_ref )
      {
        return addStyle<T1>(name, type, bindOther(setter, instance_ref));
      }

      template<
        class Derived,
        class Other,
        class OtherRef
      >
      StyleSetter
      addStyle( const std::string& name,
                const std::string& type,
                void (Other::*setter)(const std::string&),
                OtherRef Derived::*instance_ref )
      {
        return addStyle<const char*, const std::string&>
        (
          name,
          type,
          boost::function<void (Other&, const std::string&)>(setter),
          instance_ref
        );
      }

      template<typename T, class Derived, class Other, class OtherRef>
      boost::function<void (Derived&, T)>
      bindOther( void (Other::*setter)(T), OtherRef Derived::*instance_ref )
      {
        return boost::bind(setter, boost::bind(instance_ref, _1), _2);
      }

      template<typename T, class Derived, class Other, class OtherRef>
      boost::function<void (Derived&, T)>
      bindOther( const boost::function<void (Other&, T)>& setter,
                 OtherRef Derived::*instance_ref )
      {
        return boost::bind
        (
          setter,
          boost::bind
          (
            &reference_from_pointer<Other, OtherRef>,
            boost::bind(instance_ref, _1)
          ),
          _2
        );
      }

      template<typename T1, typename T2, class Derived>
      StyleSetterFuncUnchecked
      bindStyleSetter( const std::string& name,
                       const boost::function<void (Derived&, T2)>& setter )
      {
        return boost::bind
        (
          setter,
          // We will only call setters with Derived instances, so we can safely
          // cast here.
          boost::bind(&derived_cast<Derived>, _1),
          boost::bind(&getValue<T1>, _2)
        );
      }

      virtual void childAdded(SGPropertyNode * child)  {}
      virtual void childRemoved(SGPropertyNode * child){}
      virtual void childChanged(SGPropertyNode * child){}

      void setDrawable(osg::Drawable* drawable);

      /**
       * Get stateset of drawable if available or use transform otherwise
       */
      osg::StateSet* getOrCreateStateSet();

      void setupStyle();

    private:

      osg::ref_ptr<osg::Drawable> _drawable;

      Element(const Element&);// = delete

      template<class Derived>
      static Derived& derived_cast(Element& el)
      {
        return static_cast<Derived&>(el);
      }

      template<class T, class SharedPtr>
      static T& reference_from_pointer(const SharedPtr& p)
      {
        return *get_pointer(p);
      }

      /**
       * Helper to call a function only of the element type can be converted to
       * the required type.
       *
       * @return Whether the function has been called
       */
      template<class Derived>
      struct type_match
      {
        static bool call( Element& el,
                          const SGPropertyNode* prop,
                          const StyleSetterFuncUnchecked& func )
        {
          Derived* d = dynamic_cast<Derived*>(&el);
          if( !d )
            return false;
          func(*d, prop);
          return true;
        }
      };
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_ELEMENT_HXX_ */
