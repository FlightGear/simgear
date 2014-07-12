// Basic element for layouting canvas elements
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

#include "LayoutItem.hxx"
#include <simgear/canvas/Canvas.hxx>

namespace simgear
{
namespace canvas
{
  const SGVec2i LayoutItem::MAX_SIZE( SGLimits<int>::max(),
                                      SGLimits<int>::max() );

  //----------------------------------------------------------------------------
  LayoutItem::LayoutItem():
    _flags(0),
    _size_hint(0, 0),
    _min_size(0, 0),
    _max_size(MAX_SIZE)
  {
    invalidate();
  }

  //----------------------------------------------------------------------------
  LayoutItem::~LayoutItem()
  {

  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::sizeHint() const
  {
    if( _flags & SIZE_HINT_DIRTY )
    {
      _size_hint = sizeHintImpl();
      _flags &= ~SIZE_HINT_DIRTY;
    }

    return _size_hint;
  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::minimumSize() const
  {
    if( _flags & MINIMUM_SIZE_DIRTY )
    {
      _min_size = minimumSizeImpl();
      _flags &= ~MINIMUM_SIZE_DIRTY;
    }

    return _min_size;
  }

  //----------------------------------------------------------------------------
  SGVec2i LayoutItem::maximumSize() const
  {
    if( _flags & MAXIMUM_SIZE_DIRTY )
    {
      _max_size = maximumSizeImpl();
      _flags &= ~MAXIMUM_SIZE_DIRTY;
    }

    return _max_size;
  }

  //----------------------------------------------------------------------------
  bool LayoutItem::hasHeightForWidth() const
  {
    return false;
  }

  //----------------------------------------------------------------------------
  int LayoutItem::heightForWidth(int w) const
  {
    return -1;
  }

  //------------------------------------------------------------------------------
  int LayoutItem::minimumHeightForWidth(int w) const
  {
    return heightForWidth(w);
  }

  //----------------------------------------------------------------------------
  void LayoutItem::invalidate()
  {
    _flags |= SIZE_INFO_DIRTY;
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
  void LayoutItem::setGeometry(const SGRecti& geom)
  {
    _geometry = geom;
  }

  //----------------------------------------------------------------------------
  SGRecti LayoutItem::geometry() const
  {
    return _geometry;
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
    setCanvas(parent_ref ? parent_ref->_canvas : CanvasWeakPtr());
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

} // namespace canvas
} // namespace simgear
