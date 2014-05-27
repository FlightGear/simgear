// Interface for 2D Canvas element
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

#include "CanvasElement.hxx"
#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/CanvasEventVisitor.hxx>
#include <simgear/canvas/events/MouseEvent.hxx>
#include <simgear/math/SGMisc.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/scene/material/parseBlendFunc.hxx>

#include <osg/Drawable>
#include <osg/Geode>
#include <osg/Scissor>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include <cassert>
#include <cmath>
#include <cstring>

namespace simgear
{
namespace canvas
{
  const std::string NAME_TRANSFORM = "tf";

  /**
   * glScissor with coordinates relative to different reference frames.
   */
  class Element::RelativeScissor:
    public osg::Scissor
  {
    public:

      ReferenceFrame    _coord_reference;
      osg::Matrix       _parent_inverse;

      RelativeScissor():
        _coord_reference(GLOBAL)
      {}

      bool contains(const osg::Vec2f& pos) const
      {
        return _x <= pos.x() && pos.x() <= _x + _width
            && _y <= pos.y() && pos.y() <= _y + _height;
      }

      bool contains( const osg::Vec2f& global_pos,
                     const osg::Vec2f& parent_pos,
                     const osg::Vec2f& local_pos ) const
      {
        switch( _coord_reference )
        {
          case GLOBAL: return contains(global_pos);
          case PARENT: return contains(parent_pos);
          case LOCAL:  return contains(local_pos);
        }

        return false;
      }

      virtual void apply(osg::State& state) const
      {
        const osg::Viewport* vp = state.getCurrentViewport();
        float w2 = 0.5 * vp->width(),
              h2 = 0.5 * vp->height();

        osg::Matrix model_view
        (
          w2, 0,  0, 0,
          0,  h2, 0, 0,
          0,  0,  1, 0,
          w2, h2, 0, 1
        );
        model_view.preMult(state.getProjectionMatrix());

        if( _coord_reference != GLOBAL )
        {
          model_view.preMult(state.getModelViewMatrix());

          if( _coord_reference == PARENT )
            model_view.preMult(_parent_inverse);
        }

        const osg::Vec2 scale( model_view(0,0), model_view(1,1)),
                        offset(model_view(3,0), model_view(3,1));

        // TODO check/warn for rotation?

        GLint x = SGMiscf::roundToInt(scale.x() * _x + offset.x()),
              y = SGMiscf::roundToInt(scale.y() * _y + offset.y()),
              w = SGMiscf::roundToInt(std::fabs(scale.x()) * _width),
              h = SGMiscf::roundToInt(std::fabs(scale.y()) * _height);

        if( scale.x() < 0 )
          x -= w;
        if( scale.y() < 0 )
          y -= h;

        glScissor(x, y, w, h);
      }
  };

  //----------------------------------------------------------------------------
  Element::OSGUserData::OSGUserData(ElementPtr element):
    element(element)
  {

  }

  //----------------------------------------------------------------------------
  Element::~Element()
  {
    if( !_transform.valid() )
      return;

    for(unsigned int i = 0; i < _transform->getNumChildren(); ++i)
    {
      OSGUserData* ud =
        static_cast<OSGUserData*>(_transform->getChild(i)->getUserData());

      if( ud )
        // Ensure parent is cleared to prevent accessing released memory if an
        // element somehow survives longer than his parent.
        ud->element->_parent = 0;
    }
  }

  //----------------------------------------------------------------------------
  void Element::onDestroy()
  {
    if( !_transform.valid() )
      return;

    // The transform node keeps a reference on this element, so ensure it is
    // deleted.
    BOOST_FOREACH(osg::Group* parent, _transform->getParents())
    {
      parent->removeChild(_transform.get());
    }
  }

  //----------------------------------------------------------------------------
  ElementPtr Element::getParent()
  {
    return _parent;
  }

  //----------------------------------------------------------------------------
  void Element::update(double dt)
  {
    if( !isVisible() )
      return;

    // Trigger matrix update
    getMatrix();

    // TODO limit bounding box to scissor
    if( _attributes_dirty & SCISSOR_COORDS )
    {
      if( _scissor && _scissor->_coord_reference != GLOBAL )
        _scissor->_parent_inverse = _transform->getInverseMatrix();

      _attributes_dirty &= ~SCISSOR_COORDS;
    }

    // Update bounding box on manual update (manual updates pass zero dt)
    if( dt == 0 && _drawable )
      _drawable->getBound();

    if( _attributes_dirty & BLEND_FUNC )
    {
      parseBlendFunc(
        _transform->getOrCreateStateSet(),
        _node->getChild("blend-source"),
        _node->getChild("blend-destination"),
        _node->getChild("blend-source-rgb"),
        _node->getChild("blend-destination-rgb"),
        _node->getChild("blend-source-alpha"),
        _node->getChild("blend-destination-alpha")
      );
      _attributes_dirty &= ~BLEND_FUNC;
    }
  }

  //----------------------------------------------------------------------------
  bool Element::addEventListener( const std::string& type_str,
                                  const EventListener& cb )
  {
    SG_LOG
    (
      SG_GENERAL,
      SG_INFO,
      "addEventListener(" << _node->getPath() << ", " << type_str << ")"
    );

    _listener[ Event::getOrRegisterType(type_str) ].push_back(cb);
    return true;
  }

  //----------------------------------------------------------------------------
  void Element::clearEventListener()
  {
    _listener.clear();
  }

  //----------------------------------------------------------------------------
  bool Element::accept(EventVisitor& visitor)
  {
    if( !isVisible() )
      return false;

    return visitor.apply(*this);
  }

  //----------------------------------------------------------------------------
  bool Element::ascend(EventVisitor& visitor)
  {
    if( _parent )
      return _parent->accept(visitor);
    return true;
  }

  //----------------------------------------------------------------------------
  bool Element::traverse(EventVisitor& visitor)
  {
    return true;
  }

  //----------------------------------------------------------------------------
  bool Element::handleEvent(EventPtr event)
  {
    ListenerMap::iterator listeners = _listener.find(event->getType());
    if( listeners == _listener.end() )
      return false;

    BOOST_FOREACH(EventListener const& listener, listeners->second)
      listener(event);

    return true;
  }

  //----------------------------------------------------------------------------
  bool Element::dispatchEvent(EventPtr event)
  {
    EventPropagationPath path;
    path.push_back( EventTarget(this) );

    for( Element* parent = _parent;
                  parent != NULL;
                  parent = parent->_parent )
      path.push_front( EventTarget(parent) );

    CanvasPtr canvas = _canvas.lock();
    if( !canvas )
      return false;

    return canvas->propagateEvent(event, path);
  }

  //----------------------------------------------------------------------------
  bool Element::hitBound( const osg::Vec2f& global_pos,
                          const osg::Vec2f& parent_pos,
                          const osg::Vec2f& local_pos ) const
  {
    if( _scissor && !_scissor->contains(global_pos, parent_pos, local_pos) )
      return false;

    const osg::Vec3f pos3(parent_pos, 0);

    // Drawables have a bounding box...
    if( _drawable )
      return _drawable->getBound().contains(osg::Vec3f(local_pos, 0));
    else if( _transform.valid() )
      // ... for other elements, i.e. groups only a bounding sphere is available
      return _transform->getBound().contains(osg::Vec3f(parent_pos, 0));
    else
      return false;
  }


  //----------------------------------------------------------------------------
  void Element::setVisible(bool visible)
  {
    if( _transform.valid() )
      // TODO check if we need another nodemask
      _transform->setNodeMask(visible ? 0xffffffff : 0);
  }

  //----------------------------------------------------------------------------
  bool Element::isVisible() const
  {
    return _transform.valid() && _transform->getNodeMask() != 0;
  }

  //----------------------------------------------------------------------------
  osg::MatrixTransform* Element::getMatrixTransform()
  {
    return _transform.get();
  }

  //----------------------------------------------------------------------------
  osg::MatrixTransform const* Element::getMatrixTransform() const
  {
    return _transform.get();
  }

  //----------------------------------------------------------------------------
  osg::Vec2f Element::posToLocal(const osg::Vec2f& pos) const
  {
    getMatrix();
    const osg::Matrix& m = _transform->getInverseMatrix();
    return osg::Vec2f
    (
      m(0, 0) * pos[0] + m(1, 0) * pos[1] + m(3, 0),
      m(0, 1) * pos[0] + m(1, 1) * pos[1] + m(3, 1)
    );
  }

  //----------------------------------------------------------------------------
  void Element::childAdded(SGPropertyNode* parent, SGPropertyNode* child)
  {
    if(    parent == _node
        && child->getNameString() == NAME_TRANSFORM )
    {
      if( child->getIndex() >= static_cast<int>(_transform_types.size()) )
        _transform_types.resize( child->getIndex() + 1 );

      _transform_types[ child->getIndex() ] = TT_NONE;
      _attributes_dirty |= TRANSFORM;
      return;
    }
    else if(    parent->getParent() == _node
             && parent->getNameString() == NAME_TRANSFORM )
    {
      assert(parent->getIndex() < static_cast<int>(_transform_types.size()));

      const std::string& name = child->getNameString();

      TransformType& type = _transform_types[parent->getIndex()];

      if(      name == "m" )
        type = TT_MATRIX;
      else if( name == "t" )
        type = TT_TRANSLATE;
      else if( name == "rot" )
        type = TT_ROTATE;
      else if( name == "s" )
        type = TT_SCALE;

      _attributes_dirty |= TRANSFORM;
      return;
    }

    childAdded(child);
  }

  //----------------------------------------------------------------------------
  void Element::childRemoved(SGPropertyNode* parent, SGPropertyNode* child)
  {
    if( parent == _node )
    {
      if( child->getNameString() == NAME_TRANSFORM )
      {
        if( !_transform.valid() )
          return;

        if( child->getIndex() >= static_cast<int>(_transform_types.size()) )
        {
          SG_LOG
          (
            SG_GENERAL,
            SG_WARN,
            "Element::childRemoved: unknown transform: " << child->getPath()
          );
          return;
        }

        _transform_types[ child->getIndex() ] = TT_NONE;

        while( !_transform_types.empty() && _transform_types.back() == TT_NONE )
          _transform_types.pop_back();

        _attributes_dirty |= TRANSFORM;
        return;
      }
      else if( StyleInfo const* style = getStyleInfo(child->getNameString()) )
      {
        if( setStyle(getParentStyle(child), style) )
          return;
      }
    }

    childRemoved(child);
  }

  //----------------------------------------------------------------------------
  void Element::valueChanged(SGPropertyNode* child)
  {
    SGPropertyNode *parent = child->getParent();
    if( parent == _node )
    {
      const std::string& name = child->getNameString();
      if( boost::starts_with(name, "data-") )
        return;
      else if( StyleInfo const* style_info = getStyleInfo(name) )
      {
        SGPropertyNode const* style = child;
        if( isStyleEmpty(child) )
        {
          child->clearValue();
          style = getParentStyle(child);
        }
        setStyle(style, style_info);
        return;
      }
      else if( name == "update" )
        return update(0);
      else if( boost::starts_with(name, "blend-") )
        return (void)(_attributes_dirty |= BLEND_FUNC);
    }
    else if(   parent->getParent() == _node
            && parent->getNameString() == NAME_TRANSFORM )
    {
      _attributes_dirty |= TRANSFORM;
      return;
    }

    childChanged(child);
  }

  //----------------------------------------------------------------------------
  bool Element::setStyle( const SGPropertyNode* child,
                          const StyleInfo* style_info )
  {
    return canApplyStyle(child) && setStyleImpl(child, style_info);
  }

  //----------------------------------------------------------------------------
  void Element::setClip(const std::string& clip)
  {
    if( clip.empty() || clip == "auto" )
    {
      getOrCreateStateSet()->removeAttribute(osg::StateAttribute::SCISSOR);
      _scissor = 0;
      return;
    }

    // TODO generalize CSS property parsing
    const std::string RECT("rect(");
    if(    !boost::ends_with(clip, ")")
        || !boost::starts_with(clip, RECT) )
    {
      SG_LOG(SG_GENERAL, SG_WARN, "Canvas: invalid clip: " << clip);
      return;
    }

    const std::string sep(", \t\npx");
    int comp = 0;
    float values[4];

    for(size_t pos = RECT.size(); comp < 4; ++comp)
    {
      pos = clip.find_first_not_of(sep, pos);
      if( pos == std::string::npos || pos == clip.size() - 1 )
        break;

      char *end = 0;
      values[comp] = strtod(&clip[pos], &end);
      if( end == &clip[pos] || !end )
        break;

      pos = end - &clip[0];
    }

    if( comp < 4 )
    {
      SG_LOG(SG_GENERAL, SG_WARN, "Canvas: invalid clip: " << clip);
      return;
    }

    float width = values[1] - values[3],
          height = values[2] - values[0];

    if( width < 0 || height < 0 )
    {
      SG_LOG(SG_GENERAL, SG_WARN, "Canvas: negative clip size: " << clip);
      return;
    }

    _scissor = new RelativeScissor();
    // <top>, <right>, <bottom>, <left>
    _scissor->x() = SGMiscf::roundToInt(values[3]);
    _scissor->y() = SGMiscf::roundToInt(values[0]);
    _scissor->width() = SGMiscf::roundToInt(width);
    _scissor->height() = SGMiscf::roundToInt(height);

    getOrCreateStateSet()->setAttributeAndModes(_scissor);

    SGPropertyNode* clip_frame = _node->getChild("clip-frame", 0);
    if( clip_frame )
      valueChanged(clip_frame);
  }

  //----------------------------------------------------------------------------
  void Element::setClipFrame(ReferenceFrame rf)
  {
    if( _scissor )
    {
      _scissor->_coord_reference = rf;
      _attributes_dirty |= SCISSOR_COORDS;
    }
  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Element::getBoundingBox() const
  {
    if( _drawable )
      return _drawable->getBound();

    osg::BoundingBox bb;

    if( _transform.valid() )
      bb.expandBy(_transform->getBound());

    return bb;
  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Element::getTightBoundingBox() const
  {
    return getTransformedBounds(getMatrix());
  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Element::getTransformedBounds(const osg::Matrix& m) const
  {
    if( !_drawable )
      return osg::BoundingBox();

    osg::BoundingBox transformed;
    const osg::BoundingBox& bb = _drawable->getBound();
    for(int i = 0; i < 4; ++i)
      transformed.expandBy( bb.corner(i) * m );

    return transformed;
  }

  //----------------------------------------------------------------------------
  osg::Matrix Element::getMatrix() const
  {
    if( !_transform )
      return osg::Matrix::identity();

    if( !(_attributes_dirty & TRANSFORM) )
      return _transform->getMatrix();

    osg::Matrix m;
    for( size_t i = 0; i < _transform_types.size(); ++i )
    {
      // Skip unused indizes...
      if( _transform_types[i] == TT_NONE )
        continue;

      SGPropertyNode* tf_node = _node->getChild("tf", i, true);

      // Build up the matrix representation of the current transform node
      osg::Matrix tf;
      switch( _transform_types[i] )
      {
        case TT_MATRIX:
          tf = osg::Matrix( tf_node->getDoubleValue("m[0]", 1),
                            tf_node->getDoubleValue("m[1]", 0),
                            0,
                            tf_node->getDoubleValue("m[6]", 0),

                            tf_node->getDoubleValue("m[2]", 0),
                            tf_node->getDoubleValue("m[3]", 1),
                            0,
                            tf_node->getDoubleValue("m[7]", 0),

                            0,
                            0,
                            1,
                            0,

                            tf_node->getDoubleValue("m[4]", 0),
                            tf_node->getDoubleValue("m[5]", 0),
                            0,
                            tf_node->getDoubleValue("m[8]", 1) );
          break;
        case TT_TRANSLATE:
          tf.makeTranslate( osg::Vec3f( tf_node->getDoubleValue("t[0]", 0),
                                        tf_node->getDoubleValue("t[1]", 0),
                                        0 ) );
          break;
        case TT_ROTATE:
          tf.makeRotate( tf_node->getDoubleValue("rot", 0), 0, 0, 1 );
          break;
        case TT_SCALE:
        {
          float sx = tf_node->getDoubleValue("s[0]", 1);
          // sy defaults to sx...
          tf.makeScale( sx, tf_node->getDoubleValue("s[1]", sx), 1 );
          break;
        }
        default:
          break;
      }
      m.postMult( tf );
    }
    _transform->setMatrix(m);
    _attributes_dirty &= ~TRANSFORM;
    _attributes_dirty |= SCISSOR_COORDS;

    return m;
  }

  //----------------------------------------------------------------------------
  Element::StyleSetters Element::_style_setters;

  //----------------------------------------------------------------------------
  Element::Element( const CanvasWeakPtr& canvas,
                    const SGPropertyNode_ptr& node,
                    const Style& parent_style,
                    Element* parent ):
    PropertyBasedElement(node),
    _canvas( canvas ),
    _parent( parent ),
    _attributes_dirty( 0 ),
    _transform( new osg::MatrixTransform ),
    _style( parent_style ),
    _scissor( 0 ),
    _drawable( 0 )
  {
    staticInit();

    SG_LOG
    (
      SG_GL,
      SG_DEBUG,
      "New canvas element " << node->getPath()
    );

    // Ensure elements are drawn in order they appear in the element tree
    _transform->getOrCreateStateSet()
              ->setRenderBinDetails
              (
                0,
                "PreOrderBin",
                osg::StateSet::OVERRIDE_RENDERBIN_DETAILS
              );

    _transform->setUserData( new OSGUserData(this) );
  }

  //----------------------------------------------------------------------------
  void Element::staticInit()
  {
    if( isInit<Element>() )
      return;

    addStyle("clip", "", &Element::setClip, false);
    addStyle("clip-frame", "", &Element::setClipFrame, false);
    addStyle("visible", "", &Element::setVisible, false);
  }

  //----------------------------------------------------------------------------
  bool Element::isStyleEmpty(const SGPropertyNode* child) const
  {
    return !child
        || simgear::strutils::strip(child->getStringValue()).empty();
  }

  //----------------------------------------------------------------------------
  bool Element::canApplyStyle(const SGPropertyNode* child) const
  {
    if( _node == child->getParent() )
      return true;

    // Parent values do not override if element has own value
    return isStyleEmpty( _node->getChild(child->getName()) );
  }

  //----------------------------------------------------------------------------
  bool Element::setStyleImpl( const SGPropertyNode* child,
                              const StyleInfo* style_info )
  {
    const StyleSetter* style_setter = style_info
                                    ? &style_info->setter
                                    : getStyleSetter(child->getNameString());
    while( style_setter )
    {
      if( style_setter->func(*this, child) )
        return true;
      style_setter = style_setter->next;
    }
    return false;
  }

  //----------------------------------------------------------------------------
  const Element::StyleInfo*
  Element::getStyleInfo(const std::string& name) const
  {
    StyleSetters::const_iterator setter = _style_setters.find(name);
    if( setter == _style_setters.end() )
      return 0;

    return &setter->second;
  }

  //----------------------------------------------------------------------------
  const Element::StyleSetter*
  Element::getStyleSetter(const std::string& name) const
  {
    const StyleInfo* info = getStyleInfo(name);
    return info ? &info->setter : 0;
  }

  //----------------------------------------------------------------------------
  const SGPropertyNode*
  Element::getParentStyle(const SGPropertyNode* child) const
  {
    // Try to get value from parent...
    if( _parent )
    {
      Style::const_iterator style =
        _parent->_style.find(child->getNameString());
      if( style != _parent->_style.end() )
        return style->second;
    }

    // ...or reset to default if none is available
    return child; // TODO somehow get default value for each style?
  }

  //----------------------------------------------------------------------------
  void Element::setDrawable( osg::Drawable* drawable )
  {
    _drawable = drawable;
    assert( _drawable );

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(_drawable);
    _transform->addChild(geode);
  }

  //----------------------------------------------------------------------------
  osg::StateSet* Element::getOrCreateStateSet()
  {
    return _drawable ? _drawable->getOrCreateStateSet()
                     : _transform->getOrCreateStateSet();
  }

  //----------------------------------------------------------------------------
  void Element::setupStyle()
  {
    BOOST_FOREACH( Style::value_type style, _style )
      setStyle(style.second);
  }

} // namespace canvas
} // namespace simgear
