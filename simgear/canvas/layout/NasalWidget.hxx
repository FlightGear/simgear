///@file
/// Glue for GUI widgets implemented in Nasal space.
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

#ifndef SG_CANVAS_NASAL_WIDGET_HXX_
#define SG_CANVAS_NASAL_WIDGET_HXX_

#include "LayoutItem.hxx"

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/NasalObject.hxx>

namespace simgear
{
namespace canvas
{

  /**
   * Base class/ghost to implement gui widgets in Nasal space.
   */
  class NasalWidget:
    public LayoutItem,
    public nasal::Object
  {
    public:

      typedef boost::function<void (nasal::Me, const SGRecti&)> SetGeometryFunc;
      typedef boost::function<int (nasal::Me, int)> HeightForWidthFunc;

      /**
       *
       * @param impl    Initial implementation hash (nasal part of
       *                implementation)
       */
      NasalWidget(naRef impl);

      ~NasalWidget();

      virtual void onRemove();

      void setSetGeometryFunc(const SetGeometryFunc& func);
      void setHeightForWidthFunc(const HeightForWidthFunc& func);
      void setMinimumHeightForWidthFunc(const HeightForWidthFunc& func);

      /** Set size hint.
       *
       * Overrides default size hint. Set to (0, 0) to fall back to default size
       * hint.
       */
      void setSizeHint(const SGVec2i& s);

      /** Set minimum size.
       *
       * Overrides default minimum size. Set to (0, 0) to fall back to default
       * minimum size.
       */
      void setMinimumSize(const SGVec2i& s);

      /** Set maximum size.
       *
       * Overrides default maximum size hint. Set to LayoutItem::MAX_SIZE to
       * fall back to default maximum size.
       */
      void setMaximumSize(const SGVec2i& s);

      void setLayoutSizeHint(const SGVec2i& s);
      void setLayoutMinimumSize(const SGVec2i& s);
      void setLayoutMaximumSize(const SGVec2i& s);

      virtual bool hasHeightForWidth() const;

      /**
       * @param ns  Namespace to register the class interface
       */
      static void setupGhost(nasal::Hash& ns);

    protected:
      enum WidgetFlags
      {
        LAYOUT_DIRTY = LayoutItem::LAST_FLAG << 1,
        LAST_FLAG = LAYOUT_DIRTY
      };

      SetGeometryFunc       _set_geometry;
      HeightForWidthFunc    _height_for_width,
                            _min_height_for_width;

      SGVec2i _layout_size_hint,
              _layout_min_size,
              _layout_max_size,
              _user_size_hint,
              _user_min_size,
              _user_max_size;

      int callHeightForWidthFunc( const HeightForWidthFunc& hfw,
                                  int w ) const;

      virtual SGVec2i sizeHintImpl() const;
      virtual SGVec2i minimumSizeImpl() const;
      virtual SGVec2i maximumSizeImpl() const;

      virtual int heightForWidthImpl(int w) const;
      virtual int minimumHeightForWidthImpl(int w) const;

      virtual void contentsRectChanged(const SGRecti& rect);

      virtual void visibilityChanged(bool visible);

  };

  typedef SGSharedPtr<NasalWidget> NasalWidgetRef;

} // namespace canvas
} // namespace simgear


#endif /* SG_CANVAS_NASAL_WIDGET_HXX_ */
