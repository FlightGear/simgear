///@file Glue for GUI widgets implemented in Nasal space
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
   * Baseclass/ghost to create widgets with Nasal.
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

      virtual void setGeometry(const SGRecti& geom);

      void setSetGeometryFunc(const SetGeometryFunc& func);
      void setHeightForWidthFunc(const HeightForWidthFunc& func);
      void setMinimumHeightForWidthFunc(const HeightForWidthFunc& func);

      void setSizeHint(const SGVec2i& s);
      void setMinimumSize(const SGVec2i& s);
      void setMaximumSize(const SGVec2i& s);

      virtual bool hasHeightForWidth() const;
      virtual int heightForWidth(int w) const;
      virtual int minimumHeightForWidth(int w) const;

      /**
       * @param ns  Namespace to register the class interface
       */
      static void setupGhost(nasal::Hash& ns);

    protected:
      SetGeometryFunc       _set_geometry;
      HeightForWidthFunc    _height_for_width,
                            _min_height_for_width;

      int callHeightForWidthFunc( const HeightForWidthFunc& hfw,
                                  int w ) const;

      virtual SGVec2i sizeHintImpl() const;
      virtual SGVec2i minimumSizeImpl() const;
      virtual SGVec2i maximumSizeImpl() const;

  };

  typedef SGSharedPtr<NasalWidget> NasalWidgetRef;

} // namespace canvas
} // namespace simgear


#endif /* SG_CANVAS_NASAL_WIDGET_HXX_ */
