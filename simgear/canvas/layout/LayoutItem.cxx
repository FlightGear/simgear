// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Basic element used in layouts of Canvas elements.
 */

#include <simgear_config.h>
#include "LayoutItem.hxx"
#include <simgear/canvas/Canvas.hxx>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  Margins::Margins(int m):
    l(m), t(m), r(m), b(m)
  {

  }

  //----------------------------------------------------------------------------
  Margins::Margins(int h, int v):
    l(h), t(v),
    r(h), b(v)
  {

  }

  //----------------------------------------------------------------------------
  Margins::Margins(int l, int t, int r, int b):
    l(l), t(t), r(r), b(b)
  {

  }

  //----------------------------------------------------------------------------
  int Margins::horiz() const
  {
    return l + r;
  }

  //----------------------------------------------------------------------------
  int Margins::vert() const
  {
    return t + b;
  }

  //----------------------------------------------------------------------------
  SGVec2i Margins::size() const
  {
    return SGVec2i(horiz(), vert());
  }

  //----------------------------------------------------------------------------
  bool Margins::isNull() const
  {
    return l == 0 && t == 0 && r == 0 && b == 0;
  }

  //----------------------------------------------------------------------------
  const SGVec2i LayoutItem::MAX_SIZE( SGLimits<int>::max(),
                                      SGLimits<int>::max() );

  //----------------------------------------------------------------------------
  LayoutItem::LayoutItem():
    _alignment(AlignFill),
    _flags(VISIBLE),
    _size_hint(0, 0),
    _min_size(0, 0),
    _max_size(MAX_SIZE)
  {
    invalidate();
  }

  //----------------------------------------------------------------------------
  void LayoutItem::setContentsMargins(const Margins& margins)
  {
    _margins = margins;
  }

  //----------------------------------------------------------------------------
  void LayoutItem::setContentsMargins(int left, int top, int right, int bottom)
  {
    _margins.l = left;
    _margins.t = top;
    _margins.r = right;
    _margins.b = bottom;
  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::gridLocation() const
  {
      return _gridLocation;
  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::gridSpan() const
  {
      return _span;
  }

  SGVec2i LayoutItem::gridEnd() const
  {
      return _gridLocation + _span + SGVec2i{-1, -1};
  }


  //----------------------------------------------------------------------------
  void LayoutItem::setContentsMargin(int margin)
  {
    setContentsMargins(margin, margin, margin, margin);
  }

  //----------------------------------------------------------------------------
  Margins LayoutItem::getContentsMargins() const
  {
    return _margins;
  }

  //----------------------------------------------------------------------------
  SGRecti LayoutItem::contentsRect() const
  {
    return SGRecti(
      _geometry.x() + _margins.l,
      _geometry.y() + _margins.t,
      std::max(0, _geometry.width() - _margins.horiz()),
      std::max(0, _geometry.height() - _margins.vert())
    );
  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::sizeHint() const
  {
    if( _flags & SIZE_HINT_DIRTY )
    {
      _size_hint = sizeHintImpl();
      _flags &= ~SIZE_HINT_DIRTY;
    }

    return addClipOverflow(_size_hint, _margins.size());
  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::minimumSize() const
  {
    if( _flags & MINIMUM_SIZE_DIRTY )
    {
      _min_size = minimumSizeImpl();
      _flags &= ~MINIMUM_SIZE_DIRTY;
    }

    return addClipOverflow(_min_size, _margins.size());
  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::maximumSize() const
  {
    if( _flags & MAXIMUM_SIZE_DIRTY )
    {
      _max_size = maximumSizeImpl();
      _flags &= ~MAXIMUM_SIZE_DIRTY;
    }

    return addClipOverflow(_max_size, _margins.size());
  }

  //----------------------------------------------------------------------------
  bool LayoutItem::hasHeightForWidth() const
  {
    return false;
  }

  //----------------------------------------------------------------------------
  int LayoutItem::heightForWidth(int w) const
  {
    int h = heightForWidthImpl(w - _margins.horiz());
    return h < 0 ? -1 : SGMisc<int>::addClipOverflow(h, _margins.vert());
  }

  //------------------------------------------------------------------------------
  int LayoutItem::minimumHeightForWidth(int w) const
  {
    int h = minimumHeightForWidthImpl(w - _margins.horiz());
    return h < 0 ? -1 : SGMisc<int>::addClipOverflow(h, _margins.vert());
  }

  //----------------------------------------------------------------------------
  void LayoutItem::setAlignment(uint8_t align)
  {
    if( align == _alignment )
      return;

    _alignment = align;
    invalidateParent();
  }

  //----------------------------------------------------------------------------
  uint8_t LayoutItem::alignment() const
  {
    return _alignment;
  }

  //----------------------------------------------------------------------------
  void LayoutItem::setVisible(bool visible)
  {
    if( visible )
      _flags &= ~EXPLICITLY_HIDDEN;
    else
      _flags |= EXPLICITLY_HIDDEN;

    setVisibleInternal(visible);
  }

  //----------------------------------------------------------------------------
  bool LayoutItem::isVisible() const
  {
    return _flags & VISIBLE;
  }

  //----------------------------------------------------------------------------
  bool LayoutItem::isExplicitlyHidden() const
  {
    return _flags & EXPLICITLY_HIDDEN;
  }

  //----------------------------------------------------------------------------
  void LayoutItem::invalidate()
  {
    _flags |= SIZE_INFO_DIRTY | LAYOUT_DIRTY;
    invalidateParent();
  }

  //----------------------------------------------------------------------------
  void LayoutItem::invalidateParent()
  {
    LayoutItemRef parent = _parent.lock();
    if( parent )
      parent->invalidate();
  }

  //----------------------------------------------------------------------------
  void LayoutItem::update()
  {
    if( (_flags & LAYOUT_DIRTY) && isVisible() )
      contentsRectChanged( contentsRect() );
  }

  //----------------------------------------------------------------------------
  void LayoutItem::setGeometry(const SGRecti& geom)
  {
    SGRecti ar = alignmentRect(geom);
    if( ar != _geometry )
    {
      _geometry = ar;
      _flags |= LAYOUT_DIRTY;
    }

    update();
  }

  //----------------------------------------------------------------------------
  SGRecti LayoutItem::geometry() const
  {
    return _geometry;
  }

  //----------------------------------------------------------------------------
  SGRecti LayoutItem::alignmentRect(const SGRecti& geom) const
  {
    uint8_t halign = alignment() & AlignHorizontal_Mask,
            valign = alignment() & AlignVertical_Mask;

    // Size
    SGVec2i size = sizeHint();

    if( halign == AlignFill )
      size.x() = maximumSize().x();
    size.x() = std::min(size.x(), geom.width());

    if( valign == AlignFill )
      size.y() = maximumSize().y();
    else if( hasHeightForWidth() )
      size.y() = heightForWidth(size.x());
    size.y() = std::min(size.y(), geom.height());

    // Position
    SGVec2i pos = geom.pos();

    if( halign & AlignRight )
      pos.x() += geom.width() - size.x();
    else if( !(halign & AlignLeft) )
      pos.x() += (geom.width() - size.x()) / 2;

    if( valign & AlignBottom )
      pos.y() += geom.height() - size.y();
    else if( !(valign & AlignTop) )
      pos.y() += (geom.height() - size.y()) / 2;

    return SGRecti(pos, pos + size);
  }

  //----------------------------------------------------------------------------
  void LayoutItem::setGridLocation(const SGVec2i& loc)
  {
      _gridLocation = loc;
  }

  //----------------------------------------------------------------------------
  void LayoutItem::setGridSpan(const SGVec2i& span)
  {
      _span = span;
  }

  //----------------------------------------------------------------------------
  void LayoutItem::setCanvas(const CanvasWeakPtr& canvas)
  {
    _canvas = canvas;
  }

  //----------------------------------------------------------------------------
  CanvasPtr LayoutItem::getCanvas() const
  {
    return _canvas.lock();
  }

  //----------------------------------------------------------------------------
  void LayoutItem::setParent(const LayoutItemWeakRef& parent)
  {
    _parent = parent;
    LayoutItemRef parent_ref = parent.lock();

    if( parent_ref )
      // Only change the canvas if there is a new parent. If the item is removed
      // keep the old canvas, as it may be used for example during the call to
      // onRemove.
      setCanvas(parent_ref->_canvas);

    setVisibleInternal(!parent_ref || parent_ref->isVisible());
  }

  //----------------------------------------------------------------------------
  LayoutItemRef LayoutItem::getParent() const
  {
    return _parent.lock();
  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::sizeHintImpl() const
  {
    return _size_hint;
  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::minimumSizeImpl() const
  {
    return _min_size;
  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::maximumSizeImpl() const
  {
    return _max_size;
  }

  //----------------------------------------------------------------------------
  int LayoutItem::heightForWidthImpl(int w) const
  {
    return -1;
  }

  //------------------------------------------------------------------------------
  int LayoutItem::minimumHeightForWidthImpl(int w) const
  {
    return heightForWidth(w);
  }

  //----------------------------------------------------------------------------
  void LayoutItem::setVisibleInternal(bool visible)
  {
    LayoutItemRef parent = getParent();
    if( isExplicitlyHidden() || (parent && !parent->isVisible()) )
      visible = false;

    if( isVisible() == visible )
      return;

    invalidateParent();

    if( visible )
      _flags |= VISIBLE;
    else
      _flags &= ~VISIBLE;

    visibilityChanged(visible);
  }

  //----------------------------------------------------------------------------
  void LayoutItem::callSetVisibleInternal(LayoutItem* item, bool visible)
  {
    item->setVisibleInternal(visible);
  }

} // namespace canvas
} // namespace simgear
