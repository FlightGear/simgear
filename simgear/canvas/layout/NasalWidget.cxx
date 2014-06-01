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
  void NasalWidget::setGeometry(const SGRect<int>& geom)
  {
    if( _geometry == geom )
      return;
    _geometry = geom;

    if( !_set_geometry )
      return;

    naContext c = naNewContext();
    try
    {
      _set_geometry(nasal::to_nasal(c, this), geom);
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
  void NasalWidget::setSetGeometryFunc(const SetGeometryFunc& func)
  {
    _set_geometry = func;
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setSizeHint(const SGVec2i& s)
  {
    if( _size_hint == s )
      return;

    _size_hint = s;

    // TODO just invalidate size_hint? Probably not a performance issue...
    invalidateParent();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setMinimumSize(const SGVec2i& s)
  {
    if( _min_size == s )
      return;

    _min_size = s;
    invalidateParent();
  }

  //----------------------------------------------------------------------------
  void NasalWidget::setMaximumSize(const SGVec2i& s)
  {
    if( _max_size == s )
      return;

    _max_size = s;
    invalidateParent();
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
      .method("setSizeHint", &NasalWidget::setSizeHint)
      .method("setMinimumSize", &NasalWidget::setMinimumSize)
      .method("setMaximumSize", &NasalWidget::setMaximumSize);

    nasal::Hash widget_hash = ns.createHash("Widget");
    widget_hash.set("new", &f_makeNasalWidget);
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
