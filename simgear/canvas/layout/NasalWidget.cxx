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

#include <simgear_config.h>

#include "NasalWidget.hxx"

#include <simgear/canvas/Canvas.hxx>
#include <simgear/nasal/cppbind/NasalContext.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  NasalWidget::NasalWidget(naRef impl):
    Object(impl),
    _layout_size_hint(32, 32),
    _layout_min_size(16, 16),
    _layout_max_size(MAX_SIZE),
    _user_size_hint(0, 0),
    _user_min_size(0, 0),
    _user_max_size(MAX_SIZE)
  {

  }

  //----------------------------------------------------------------------------
  NasalWidget::~NasalWidget()
  {

  }

  //----------------------------------------------------------------------------
  void NasalWidget::onRemove()
  {
    try
    {
      callMethod<void>("onRemove");
    }
    catch( std::exception const& ex )
    {
      SG_LOG(
        SG_GUI,
        SG_WARN,
        "NasalWidget::onRemove: callback error: '" << ex.what() << "'"
      );
    }
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
    if( _user_size_hint == s )
      return;

    _user_size_hint = s;

    // TODO just invalidate size_hint? Probably not a performance issue...
    invalidate();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setMinimumSize(const SGVec2i& s)
  {
    if( _user_min_size == s )
      return;

    _user_min_size = s;
    invalidate();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setMaximumSize(const SGVec2i& s)
  {
    if( _user_max_size == s )
      return;

    _user_max_size = s;
    invalidate();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setLayoutSizeHint(const SGVec2i& s)
  {
    if( _layout_size_hint == s )
      return;

    _layout_size_hint = s;
    invalidate();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setLayoutMinimumSize(const SGVec2i& s)
  {
    if( _layout_min_size == s )
      return;

    _layout_min_size = s;
    invalidate();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setLayoutMaximumSize(const SGVec2i& s)
  {
    if( _layout_max_size == s )
      return;

    _layout_max_size = s;
    invalidate();
  }

  //----------------------------------------------------------------------------
  bool NasalWidget::hasHeightForWidth() const
  {
    return _height_for_width || _min_height_for_width;
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
      .method("setMaximumSize", &NasalWidget::setMaximumSize)
      .method("setLayoutSizeHint", &NasalWidget::setLayoutSizeHint)
      .method("setLayoutMinimumSize", &NasalWidget::setLayoutMinimumSize)
      .method("setLayoutMaximumSize", &NasalWidget::setLayoutMaximumSize);

    nasal::Hash widget_hash = ns.createHash("Widget");
    widget_hash.set("new", &f_makeNasalWidget);
  }

  //----------------------------------------------------------------------------
  int NasalWidget::callHeightForWidthFunc( const HeightForWidthFunc& hfw,
                                           int w ) const
  {
    if( !hfw )
      return -1;

    try
    {
      nasal::Context ctx;
      return hfw(ctx.to_me(const_cast<NasalWidget*>(this)), w);
    }
    catch( std::exception const& ex )
    {
      SG_LOG(
        SG_GUI,
        SG_WARN,
        "NasalWidget.heightForWidth: callback error: '" << ex.what() << "'"
      );
    }

    return -1;
  }

  //----------------------------------------------------------------------------
  SGVec2i NasalWidget::sizeHintImpl() const
  {
    return SGVec2i(
      _user_size_hint.x() > 0 ? _user_size_hint.x() : _layout_size_hint.x(),
      _user_size_hint.y() > 0 ? _user_size_hint.y() : _layout_size_hint.y()
    );
  }

  //----------------------------------------------------------------------------
  SGVec2i NasalWidget::minimumSizeImpl() const
  {
    return SGVec2i(
      _user_min_size.x() > 0 ? _user_min_size.x() : _layout_min_size.x(),
      _user_min_size.y() > 0 ? _user_min_size.y() : _layout_min_size.y()
    );
  }

  //----------------------------------------------------------------------------
  SGVec2i NasalWidget::maximumSizeImpl() const
  {
    return SGVec2i(
      _user_max_size.x() < MAX_SIZE.x() ? _user_max_size.x()
                                        : _layout_max_size.x(),
      _user_max_size.y() < MAX_SIZE.y() ? _user_max_size.y()
                                        : _layout_max_size.y()
    );
  }


  //----------------------------------------------------------------------------
  int NasalWidget::heightForWidthImpl(int w) const
  {
    return callHeightForWidthFunc( _height_for_width
                                 ? _height_for_width
                                 : _min_height_for_width, w );
  }

  //----------------------------------------------------------------------------
  int NasalWidget::minimumHeightForWidthImpl(int w) const
  {
    return callHeightForWidthFunc( _min_height_for_width
                                 ? _min_height_for_width
                                 : _height_for_width, w );
  }


  //----------------------------------------------------------------------------
  void NasalWidget::contentsRectChanged(const SGRect<int>& rect)
  {
    if( !_set_geometry )
      return;

    try
    {
      nasal::Context ctx;
      _set_geometry(ctx.to_me(this), rect);
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
  }

  //----------------------------------------------------------------------------
  void NasalWidget::visibilityChanged(bool visible)
  {
    try
    {
      callMethod<void>("visibilityChanged", visible);
    }
    catch( std::exception const& ex )
    {
      SG_LOG(
        SG_GUI,
        SG_WARN,
        "NasalWidget::visibilityChanged: callback error: '" << ex.what() << "'"
      );
    }
  }

} // namespace canvas
} // namespace simgear
