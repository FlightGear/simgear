// Glue for GUI layout items implemented in Nasal space
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

#include "NasalWidget.hxx"

#include <simgear/canvas/Canvas.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  NasalWidget::NasalWidget(naRef impl):
    Object(impl)
  {

  }

  //----------------------------------------------------------------------------
  NasalWidget::~NasalWidget()
  {
    onRemove();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::invalidate()
  {
    LayoutItem::invalidate();
    _flags |= LAYOUT_DIRTY;
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setGeometry(const SGRect<int>& geom)
  {
    if( _geometry != geom )
      _geometry = geom;
    else if( !(_flags & LAYOUT_DIRTY) || !_set_geometry )
      return;

    naContext c = naNewContext();
    try
    {
      _set_geometry(nasal::to_nasal(c, this), geom);
      _flags &= ~LAYOUT_DIRTY;
    }
    catch( std::exception const& ex )
    {
      SG_LOG(
        SG_GUI,
        SG_WARN,
        "NasalWidget::setGeometry: callback error: '" << ex.what() << "'"
      );
    }
    naFreeContext(c);
  }

  //----------------------------------------------------------------------------
  void NasalWidget::onRemove()
  {
    if( !_nasal_impl.valid() )
      return;

    typedef boost::function<void(nasal::Me)> Deleter;

    naContext c = naNewContext();
    try
    {
      Deleter del =
        nasal::get_member<Deleter>(c, _nasal_impl.get_naRef(), "onRemove");
      if( del )
        del(_nasal_impl.get_naRef());
    }
    catch( std::exception const& ex )
    {
      SG_LOG(
        SG_GUI,
        SG_WARN,
        "NasalWidget::onRemove: callback error: '" << ex.what() << "'"
      );
    }
    naFreeContext(c);
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setSetGeometryFunc(const SetGeometryFunc& func)
  {
    _set_geometry = func;
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setHeightForWidthFunc(const HeightForWidthFunc& func)
  {
    _height_for_width = func;
    invalidate();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setMinimumHeightForWidthFunc(const HeightForWidthFunc& func)
  {
    _min_height_for_width = func;
    invalidate();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setSizeHint(const SGVec2i& s)
  {
    if( _size_hint == s )
      return;

    _size_hint = s;

    // TODO just invalidate size_hint? Probably not a performance issue...
    invalidate();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setMinimumSize(const SGVec2i& s)
  {
    if( _min_size == s )
      return;

    _min_size = s;
    invalidate();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setMaximumSize(const SGVec2i& s)
  {
    if( _max_size == s )
      return;

    _max_size = s;
    invalidate();
  }

  //----------------------------------------------------------------------------
  bool NasalWidget::hasHeightForWidth() const
  {
    return !_height_for_width.empty() || !_min_height_for_width.empty();
  }

  //----------------------------------------------------------------------------
  int NasalWidget::heightForWidth(int w) const
  {
    return callHeightForWidthFunc( _height_for_width.empty()
                                 ? _min_height_for_width
                                 : _height_for_width, w );
  }

  //----------------------------------------------------------------------------
  int NasalWidget::minimumHeightForWidth(int w) const
  {
    return callHeightForWidthFunc( _min_height_for_width.empty()
                                 ? _height_for_width
                                 : _min_height_for_width, w );
  }

  //----------------------------------------------------------------------------
  static naRef f_makeNasalWidget(const nasal::CallContext& ctx)
  {
    return ctx.to_nasal(NasalWidgetRef(
      new NasalWidget( ctx.requireArg<naRef>(0) )
    ));
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setupGhost(nasal::Hash& ns)
  {
    nasal::Ghost<NasalWidgetRef>::init("canvas.Widget")
      .bases<LayoutItemRef>()
      .bases<nasal::ObjectRef>()
      .method("setSetGeometryFunc", &NasalWidget::setSetGeometryFunc)
      .method("setMinimumHeightForWidthFunc",
                                    &NasalWidget::setMinimumHeightForWidthFunc)
      .method("setHeightForWidthFunc", &NasalWidget::setHeightForWidthFunc)
      .method("setSizeHint", &NasalWidget::setSizeHint)
      .method("setMinimumSize", &NasalWidget::setMinimumSize)
      .method("setMaximumSize", &NasalWidget::setMaximumSize);

    nasal::Hash widget_hash = ns.createHash("Widget");
    widget_hash.set("new", &f_makeNasalWidget);
  }

  //----------------------------------------------------------------------------
  int NasalWidget::callHeightForWidthFunc( const HeightForWidthFunc& hfw,
                                           int w ) const
  {
    if( hfw.empty() )
      return -1;

    naContext c = naNewContext();
    try
    {
      return hfw(nasal::to_nasal(c, const_cast<NasalWidget*>(this)), w);
    }
    catch( std::exception const& ex )
    {
      SG_LOG(
        SG_GUI,
        SG_WARN,
        "NasalWidget.heightForWidth: callback error: '" << ex.what() << "'"
      );
    }
    naFreeContext(c);

    return -1;
  }

  //----------------------------------------------------------------------------
  SGVec2i NasalWidget::sizeHintImpl() const
  {
    return _size_hint;
  }

  //----------------------------------------------------------------------------
  SGVec2i NasalWidget::minimumSizeImpl() const
  {
    return _min_size;
  }

  //----------------------------------------------------------------------------
  SGVec2i NasalWidget::maximumSizeImpl() const
  {
    return _max_size;
  }

} // namespace canvas
} // namespace simgear
