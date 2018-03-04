///@file
/// Geographic projections for Canvas map element
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

#ifndef CANVAS_MAP_PROJECTION_HXX_
#define CANVAS_MAP_PROJECTION_HXX_

#include <simgear/math/SGMisc.hxx>

namespace simgear
{
namespace canvas
{

  /**
   * Base class for all projections
   */
  class Projection
  {
    public:
      struct ScreenPosition
      {
        ScreenPosition():
          x(0),
          y(0)
        {}

        ScreenPosition(double x, double y):
          x(x),
          y(y)
        {}

        double x, y;
      };

      virtual ~Projection() {}

      void setScreenRange(double range)
      {
        _screen_range = range;
      }

      virtual ScreenPosition worldToScreen(double x, double y) = 0;

    protected:

      double _screen_range;
  };

  /**
   * Base class for horizontal projections
   */
  class HorizontalProjection:
    public Projection
  {
    public:

      HorizontalProjection():
        _ref_lat(0),
        _ref_lon(0),
        _angle(0),
        _cos_angle(1),
        _sin_angle(0),
        _range(5)
      {
        setScreenRange(200);
      }

      /**
       * Set world position of center point used for the projection
       */
      void setWorldPosition(double lat, double lon)
      {
        _ref_lat = SGMiscd::deg2rad(lat);
        _ref_lon = SGMiscd::deg2rad(lon);
      }

      /**
       * Set up heading
       */
      void setOrientation(float hdg)
      {
        _angle = hdg;

        hdg = SGMiscf::deg2rad(hdg);
        _sin_angle = sin(hdg);
        _cos_angle = cos(hdg);
      }

      /**
       * Get orientation/heading of the projection (in degree)
       */
      float orientation() const
      {
        return _angle;
      }

      void setRange(double range)
      {
        _range = range;
      }

      /**
       * Transform given world position to screen position
       *
       * @param lat   Latitude in degrees
       * @param lon   Longitude in degrees
       */
      ScreenPosition worldToScreen(double lat, double lon)
      {
        lat = SGMiscd::deg2rad(lat);
        lon = SGMiscd::deg2rad(lon);
        ScreenPosition pos = project(lat, lon);
        double scale = _screen_range / _range;
        pos.x *= scale;
        pos.y *= scale;
        return ScreenPosition
        (
          _cos_angle * pos.x - _sin_angle * pos.y,
         -_sin_angle * pos.x - _cos_angle * pos.y
        );
      }

    protected:

      /**
       * Project given geographic world position to screen space
       *
       * @param lat   Latitude in radians
       * @param lon   Longitude in radians
       */
      virtual ScreenPosition project(double lat, double lon) const = 0;

      double  _ref_lat,   ///<! Reference latitude (radian)
              _ref_lon,   ///<! Reference latitude (radian)
              _angle,     ///<! Map rotation angle (degree)
              _cos_angle,
              _sin_angle,
              _range;
  };

  /**
   * Sanson-Flamsteed projection, relative to the projection center
   */
  class SansonFlamsteedProjection:
    public HorizontalProjection
  {
    protected:

      virtual ScreenPosition project(double lat, double lon) const
      {
        double d_lat = lat - _ref_lat,
               d_lon = lon - _ref_lon;
        double r = getEarthRadius(lat);

        ScreenPosition pos;

        pos.x = r * cos(lat) * d_lon;
        pos.y = r * d_lat;

        return pos;
      }

      /**
       * Returns Earth radius at a given latitude (Ellipsoide equation with two
       * equal axis)
       */
      float getEarthRadius(float lat) const
      {
        const float rec  = 6378137.f / 1852;      // earth radius, equator (?)
        const float rpol = 6356752.314f / 1852;   // earth radius, polar   (?)

        double a = cos(lat) / rec;
        double b = sin(lat) / rpol;
        return 1.0f / sqrt( a * a + b * b );
      }
  };

  /**
   * WebMercator projection, relative to the projection center.
   * Required for Slippy Maps - i.e. openstreetmap
   */
  class WebMercatorProjection:
    public HorizontalProjection
  {
    protected:

      virtual ScreenPosition project(double lat, double lon) const
      {
        double d_lat = lat - _ref_lat,
               d_lon = lon - _ref_lon;
        double r = 6378137.f / 1852; // Equatorial radius divided by ?

        ScreenPosition pos;

        pos.x = r * d_lon;
        pos.y = r * (log(tan(d_lat) + 1.0 / cos(d_lat)));
        //pos.x = lon;
        //pos.y = log(tan(lat) + 1.0 / cos(lat));
        return pos;
      }
  };


} // namespace canvas
} // namespace simgear

#endif /* CANVAS_MAP_PROJECTION_HXX_ */
