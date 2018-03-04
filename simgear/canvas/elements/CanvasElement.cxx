///@file
/// Interface for 2D Canvas element
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

#include <simgear_config.h>

#include "CanvasElement.hxx"
#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/CanvasEventVisitor.hxx>
#include <simgear/canvas/events/MouseEvent.hxx>
#include <simgear/math/SGMisc.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/scene/material/parseBlendFunc.hxx>

#include <osg/Drawable>
#include <osg/Geode>
#include <osg/StateAttribute>
#include <osg/Version>

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
    public osg::StateAttribute
  {
    public:

      ReferenceFrame                _coord_reference;
      osg::observer_ptr<osg::Node>  _node;

      explicit RelativeScissor(osg::Node* node = NULL):
        _coord_reference(GLOBAL),
        _node(node),
        _x(0),
        _y(0),
        _width(0),
        _height(0)
      {

      }

      /** Copy constructor using CopyOp to manage deep vs shallow copy. */
      RelativeScissor( const RelativeScissor& vp,
                       const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY ):
        StateAttribute(vp, copyop),
        _coord_reference(vp._coord_reference),
        _node(vp._node),
        _x(vp._x),
        _y(vp._y),
        _width(vp._width),
        _height(vp._height)
      {}

      META_StateAttribute(simgear, RelativeScissor, SCISSOR);

      /** Return -1 if *this < *rhs, 0 if *this==*rhs, 1 if *this>*rhs. */
      virtual int compare(const StateAttribute& sa) const
      {
        // check the types are equal and then create the rhs variable
        // used by the COMPARE_StateAttribute_Parameter macros below.
        COMPARE_StateAttribute_Types(RelativeScissor,sa)

        // compare each parameter in turn against the rhs.
        COMPARE_StateAttribute_Parameter(_x)
        COMPARE_StateAttribute_Parameter(_y)
        COMPARE_StateAttribute_Parameter(_width)
        COMPARE_StateAttribute_Parameter(_height)
        COMPARE_StateAttribute_Parameter(_coord_reference)
        COMPARE_StateAttribute_Parameter(_node)

        return 0; // passed all the above comparison macros, must be equal.
      }

      virtual bool getModeUsage(StateAttribute::ModeUsage& usage) const
      {
        usage.usesMode(GL_SCISSOR_TEST);
        return true;
      }

      inline float& x() { return _x; }
      inline float x() const { return _x; }

      inline float& y() { return _y; }
      inline float y() const { return _y; }

      inline float& width() { return _width; }
      inline float width() const { return _width; }

      inline float& height() { return _height; }
      inline float height() const { return _height; }

      virtual void apply(osg::State& state) const
      {
        if( _width <= 0 || _height <= 0 )
          return;

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
          osg::Node* ref_obj = _node.get();

          if( _coord_reference == PARENT )
          {
            if( _node->getNumParents() < 1 )
            {
              SG_LOG(SG_GL, SG_WARN, "RelativeScissor: missing parent.");
              return;
            }

            ref_obj = _node->getParent(0);
          }

          osg::MatrixList const& parent_matrices = ref_obj->getWorldMatrices();
          assert( !parent_matrices.empty() );
          model_view.preMult(parent_matrices.front());
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

    protected:
      float _x,
            _y,
            _width,
            _height;
  };

  //----------------------------------------------------------------------------
  Element::OSGUserData::OSGUserData(ElementPtr element):
    element(element)
  {

  }

  //----------------------------------------------------------------------------
  Element::~Element()
  {
  }

  //----------------------------------------------------------------------------
  void Element::onDestroy()
  {
    if( !_scene_group.valid() )
      return;

    // The transform node keeps a reference on this element, so ensure it is
    // deleted.
    for(osg::Group* parent: _scene_group->getParents())
    {
      parent->removeChild(_scene_group.get());
    }

    // Hide in case someone still holds a reference
    setVisible(false);
    removeListener();

    _parent = nullptr;
    _scene_group = nullptr;
  }

  //----------------------------------------------------------------------------
  ElementPtr Element::getParent() const
  {
    return _parent.lock();
  }

  //----------------------------------------------------------------------------
  CanvasWeakPtr Element::getCanvas() const
  {
    return _canvas;
  }

  //----------------------------------------------------------------------------
  void Element::update(double dt)
  {
    if( isVisible() )
      updateImpl(dt);
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
  void Element::setFocus()
  {
    if( auto canvas = _canvas.lock() )
      canvas->setFocusElement(this);
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
    ElementPtr parent = getParent();
    if( parent )
      return parent->accept(visitor);
    return true;
  }

  //----------------------------------------------------------------------------
  bool Element::traverse(EventVisitor& visitor)
  {
    return true;
  }

  //----------------------------------------------------------------------------
  size_t Element::numEventHandler(int type) const
  {
    ListenerMap::const_iterator listeners = _listener.find(type);
    if( listeners != _listener.end() )
      return listeners->second.size();
    return 0;
  }

  //----------------------------------------------------------------------------
  bool Element::handleEvent(const EventPtr& event)
  {
    ListenerMap::iterator listeners = _listener.find(event->getType());
    if( listeners == _listener.end() )
      return false;

    for(auto const& listener: listeners->second)
    {
      try
      {
        listener(event);
      }
      catch( std::exception const& ex )
      {
        SG_LOG(
          SG_GENERAL,
          SG_WARN,
          "canvas::Element: event handler error: '" << ex.what() << "'"
        );
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool Element::dispatchEvent(const EventPtr& event)
  {
    EventPropagationPath path;
    path.push_back( EventTarget(this) );

    for( ElementPtr parent = getParent();
                    parent.valid();
                    parent = parent->getParent() )
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
      return _drawable->
#if OSG_VERSION_LESS_THAN(3,3,2)
      getBound()
#else
      getBoundingBox()
#endif
      .contains(osg::Vec3f(local_pos, 0));
    else if( _scene_group.valid() )
      // ... for other elements, i.e. groups only a bounding sphere is available
      return _scene_group->getBound().contains(osg::Vec3f(parent_pos, 0));
    else
      return false;
  }


  //----------------------------------------------------------------------------
  void Element::setVisible(bool visible)
  {
    if( _scene_group.valid() )
      // TODO check if we need another nodemask
      _scene_group->setNodeMask(visible ? 0xffffffff : 0);
  }

  //----------------------------------------------------------------------------
  bool Element::isVisible() const
  {
    return _scene_group.valid() && _scene_group->getNodeMask() != 0;
  }

  //----------------------------------------------------------------------------
  osg::MatrixTransform* Element::getSceneGroup() const
  {
    return _scene_group.get();
  }

  //----------------------------------------------------------------------------
  osg::Vec2f Element::posToLocal(const osg::Vec2f& pos) const
  {
    if( !_scene_group )
      // TODO log warning?
      return pos;

    updateMatrix();
    const osg::Matrix& m = _scene_group->getInverseMatrix();
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
      if( strutils::starts_with(name, "data-") )
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
      else if( strutils::starts_with(name, "blend-") )
        return (void)(_attributes_dirty |= BLEND_FUNC);
    }
    else if(   parent
            && parent->getParent() == _node
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
    if( !_scene_group )
      // TODO warn?
      return;

    osg::StateSet* ss = getOrCreateStateSet();
    if( !ss )
      return;

    if( clip.empty() || clip == "auto" )
    {
      ss->removeAttribute(osg::StateAttribute::SCISSOR);
      _scissor = 0;
      return;
    }

    // TODO generalize CSS property parsing
    const std::string RECT("rect(");
    if(    !strutils::ends_with(clip, ")")
        || !strutils::starts_with(clip, RECT) )
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

    if( !_scissor )
      _scissor = new RelativeScissor(_scene_group.get());

    // <top>, <right>, <bottom>, <left>
    _scissor->x() = values[3];
    _scissor->y() = values[0];
    _scissor->width() = width;
    _scissor->height() = height;

    SGPropertyNode* clip_frame = _node->getChild("clip-frame", 0);
    if( clip_frame )
      valueChanged(clip_frame);
    else
      _scissor->_coord_reference = GLOBAL;

    ss->setAttributeAndModes(_scissor);
  }

  //----------------------------------------------------------------------------
  void Element::setClipFrame(ReferenceFrame rf)
  {
    if( _scissor )
      _scissor->_coord_reference = rf;
  }

  //----------------------------------------------------------------------------
  void Element::setRotation(unsigned int index, double r)
  {
    _node->getChild(NAME_TRANSFORM, index, true)->setDoubleValue("rot", r);
  }

  //----------------------------------------------------------------------------
  void Element::setTranslation(unsigned int index, double x, double y)
  {
    SGPropertyNode* tf = _node->getChild(NAME_TRANSFORM, index, true);
    tf->getChild("t", 0, true)->setDoubleValue(x);
    tf->getChild("t", 1, true)->setDoubleValue(y);
  }

  //----------------------------------------------------------------------------
  void Element::setTransformEnabled(unsigned int index, bool enabled)
  {
    SGPropertyNode* tf = _node->getChild(NAME_TRANSFORM, index, true);
    tf->setBoolValue("enabled", enabled);
  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Element::getBoundingBox() const
  {
    if( _drawable )
#if OSG_VERSION_LESS_THAN(3,3,2)
      return _drawable->getBound();
#else
      return _drawable->getBoundingBox();
#endif

    osg::BoundingBox bb;

    if( _scene_group.valid() )
      bb.expandBy( _scene_group->getBound() );

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
    const osg::BoundingBox& bb =
#if OSG_VERSION_LESS_THAN(3,3,2)
      _drawable->getBound();
#else
      _drawable->getBoundingBox();
#endif

    for(int i = 0; i < 4; ++i)
      transformed.expandBy( bb.corner(i) * m );

    return transformed;
  }

  //----------------------------------------------------------------------------
  osg::Matrix Element::getMatrix() const
  {
    if( !_scene_group )
      return osg::Matrix::identity();

    updateMatrix();
    return _scene_group->getMatrix();
  }

  //----------------------------------------------------------------------------
  Element::StyleSetters Element::_style_setters;

  //----------------------------------------------------------------------------
  Element::Element( const CanvasWeakPtr& canvas,
                    const SGPropertyNode_ptr& node,
                    const Style& parent_style,
                    ElementWeakPtr parent ):
    PropertyBasedElement(node),
    _canvas( canvas ),
    _parent( parent ),
    _scene_group( new osg::MatrixTransform ),
    _style( parent_style )
  {
    staticInit();

    SG_LOG
    (
      SG_GL,
      SG_DEBUG,
      "New canvas element " << node->getPath()
    );

    // Ensure elements are drawn in order they appear in the element tree
    _scene_group
      ->getOrCreateStateSet()
      ->setRenderBinDetails(
        0,
        "PreOrderBin",
        osg::StateSet::OVERRIDE_RENDERBIN_DETAILS
      );

    _scene_group->setUserData( new OSGUserData(this) );
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
    ElementPtr parent = getParent();
    if( parent )
    {
      Style::const_iterator style =
        parent->_style.find(child->getNameString());
      if( style != parent->_style.end() )
        return style->second;
    }

    // ...or reset to default if none is available
    return child; // TODO somehow get default value for each style?
  }

  //----------------------------------------------------------------------------
  void Element::setDrawable( osg::Drawable* drawable )
  {
    _drawable = drawable;

    if( !_drawable )
    {
      SG_LOG(SG_GL, SG_WARN, "canvas::Element::setDrawable: NULL drawable");
      return;
    }
    if( !_scene_group )
    {
      SG_LOG(SG_GL, SG_WARN, "canvas::Element::setDrawable: NULL scenegroup");
      return;
    }

    auto geode = new osg::Geode;
    geode->addDrawable(_drawable);
    _scene_group->addChild(geode);
  }

  //----------------------------------------------------------------------------
  osg::StateSet* Element::getOrCreateStateSet()
  {
    if( _drawable.valid() )
      return _drawable->getOrCreateStateSet();
    else if( _scene_group.valid() )
      return _scene_group->getOrCreateStateSet();
    else
      return nullptr;
  }

  //----------------------------------------------------------------------------
  void Element::setupStyle()
  {
    for(auto const& style: _style)
      setStyle(style.second);
  }

  //----------------------------------------------------------------------------
  void Element::updateMatrix() const
  {
    if( !(_attributes_dirty & TRANSFORM) || !_scene_group )
      return;

    osg::Matrix m;
    for( size_t i = 0; i < _transform_types.size(); ++i )
    {
      // Skip unused indizes...
      if( _transform_types[i] == TT_NONE )
        continue;

      SGPropertyNode* tf_node = _node->getChild("tf", i, true);
      if (!tf_node->getBoolValue("enabled", true)) {
        continue; // skip disabled transforms
      }

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
    _scene_group->setMatrix(m);
    _attributes_dirty &= ~TRANSFORM;
  }

  //----------------------------------------------------------------------------
  void Element::updateImpl(double dt)
  {
    updateMatrix();

    // Update bounding box on manual update (manual updates pass zero dt)
    if( dt == 0 && _drawable )
      _drawable->getBound();

    if( (_attributes_dirty & BLEND_FUNC) )
    {
      parseBlendFunc(
        _scene_group->getOrCreateStateSet(),
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

} // namespace canvas
} // namespace simgear
