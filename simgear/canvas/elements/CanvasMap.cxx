///@file
/// A group of 2D Canvas elements which get automatically transformed according
/// to the map parameters.
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

#include "CanvasMap.hxx"
#include "map/geo_node_pair.hxx"
#include "map/projection.hxx"

#include <simgear/misc/strutils.hxx>

#include <cmath>

#define LOG_GEO_RET(msg) \
  {\
    SG_LOG\
    (\
      SG_GENERAL,\
      SG_WARN,\
      msg << " (" << child->getStringValue()\
                  << ", " << child->getPath() << ")"\
    );\
    return;\
  }

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  const std::string GEO = "-geo";
  const std::string HDG = "hdg";
  const std::string Map::TYPE_NAME = "map";
  const std::string WEB_MERCATOR = "webmercator";
  const std::string REF_LAT = "ref-lat";
  const std::string REF_LON = "ref-lon";
  const std::string SCREEN_RANGE = "screen-range";
  const std::string RANGE = "range";
  const std::string PROJECTION = "projection";

  //----------------------------------------------------------------------------
  void Map::staticInit()
  {
    Group::staticInit();

    if( isInit<Map>() )
      return;

    // Do some initialization if needed...
  }

  //----------------------------------------------------------------------------
  Map::Map( const CanvasWeakPtr& canvas,
            const SGPropertyNode_ptr& node,
            const Style& parent_style,
            ElementWeakPtr parent ):
    Group(canvas, node, parent_style, parent)
  {
    staticInit();

    projectionNodeChanged(node->getChild(PROJECTION));
  }

  //----------------------------------------------------------------------------
  Map::~Map()
  {

  }

  //----------------------------------------------------------------------------
  void Map::updateImpl(double dt)
  {
    Group::updateImpl(dt);

    for(auto& it: _geo_nodes)
    {
      GeoNodePair* geo_node = it.second.get();
      if(    !geo_node->isComplete()
          || (!geo_node->isDirty() && !_projection_dirty) )
        continue;

      double latD = -9999.0, lonD = -9999.0;
      if (geo_node->isDirty()) {
        GeoCoord lat = parseGeoCoord(geo_node->getLat());
        if( lat.type != GeoCoord::LATITUDE )
          continue;
        
        GeoCoord lon = parseGeoCoord(geo_node->getLon());
        if( lon.type != GeoCoord::LONGITUDE )
          continue;
        
        // save the parsed values so we can re-use them if only projection
        // is changed (very common case for moving vehicle)
        latD = lat.value;
        lonD = lon.value;
        geo_node->setCachedLatLon(std::make_pair(latD, lonD));
      } else {
        std::tie(latD, lonD) = geo_node->getCachedLatLon();
      }
      
      Projection::ScreenPosition pos = _projection->worldToScreen(latD, lonD);
      geo_node->setScreenPos(pos.x, pos.y);

//      geo_node->print();
      geo_node->setDirty(false);
    }
    _projection_dirty = false;
  }

  //----------------------------------------------------------------------------
  void Map::childAdded(SGPropertyNode* parent, SGPropertyNode* child)
  {
    if( strutils::ends_with(child->getNameString(), GEO) )
      _geo_nodes[child].reset(new GeoNodePair());
    else if( parent != _node && child->getNameString() == HDG )
      _hdg_nodes.insert(child);
    else
      return Element::childAdded(parent, child);
  }

  //----------------------------------------------------------------------------
  void Map::childRemoved(SGPropertyNode* parent, SGPropertyNode* child)
  {
    if( strutils::ends_with(child->getNameString(), GEO) )
      // TODO remove from other node
      _geo_nodes.erase(child);
    else if( parent != _node && child->getName() == HDG )
    {
      _hdg_nodes.erase(child);

      // Remove rotation matrix (tf[0]) and return to element always being
      // oriented upwards (or any orientation inside other matrices).
      child->getParent()->removeChild("tf", 0);
    }
    else
      return Element::childRemoved(parent, child);
  }

  //----------------------------------------------------------------------------
  void Map::valueChanged(SGPropertyNode* child)
  {
    if( child->getParent() != _node )
    {
      const std::string& name = child->getNameString();

      if( strutils::ends_with(name, GEO) )
        return geoNodeChanged(child);
      else if( name == HDG )
        return hdgNodeChanged(child);
    }

    return Group::valueChanged(child);
  }

  //----------------------------------------------------------------------------
  void Map::childChanged(SGPropertyNode* child)
  {
    if( child->getParent() != _node )
      return Group::childChanged(child);

    if(    child->getNameString() == REF_LAT
        || child->getNameString() == REF_LON )
      _projection->setWorldPosition( _node->getDoubleValue(REF_LAT),
                                     _node->getDoubleValue(REF_LON) );
    else if( child->getNameString() == HDG )
    {
      _projection->setOrientation(child->getFloatValue());
      for( NodeSet::iterator it = _hdg_nodes.begin();
                             it != _hdg_nodes.end();
                           ++it )
        hdgNodeChanged(*it);
    }
    else if( child->getNameString() == RANGE )
      _projection->setRange(child->getDoubleValue());
    else if( child->getNameString() == SCREEN_RANGE )
      _projection->setScreenRange(child->getDoubleValue());
    else if( child->getNameString() == PROJECTION )
      projectionNodeChanged(child);
    else
      return Group::childChanged(child);

    _projection_dirty = true;
  }

  //----------------------------------------------------------------------------
  void Map::projectionNodeChanged(SGPropertyNode* child)
  {
    if(child && child->getStringValue() == WEB_MERCATOR)
      _projection = std::make_shared<WebMercatorProjection>();
    else
      _projection = std::make_shared<SansonFlamsteedProjection>();

    _projection->setWorldPosition(_node->getDoubleValue(REF_LAT),
                                  _node->getDoubleValue(REF_LON));

    // Only set existing properties to prevent using 0 instead of default values

    if( auto heading = _node->getChild(HDG) )
      _projection->setOrientation(heading->getFloatValue());

    if( auto screen_range = _node->getChild(SCREEN_RANGE) )
      _projection->setScreenRange(screen_range->getDoubleValue());

    if( auto range = _node->getChild(RANGE) )
      _projection->setRange(range->getDoubleValue());

    _projection_dirty = true;
  }

  //----------------------------------------------------------------------------
  void Map::geoNodeChanged(SGPropertyNode* child)
  {
    GeoNodes::iterator it_geo_node = _geo_nodes.find(child);
    if( it_geo_node == _geo_nodes.end() )
      LOG_GEO_RET("GeoNode not found!")
    GeoNodePair* geo_node = it_geo_node->second.get();

    geo_node->setDirty();

    if( !(geo_node->getStatus() & GeoNodePair::INCOMPLETE) )
      return;

    // Detect lat, lon tuples...
    GeoCoord coord = parseGeoCoord(child->getStringValue());
    int index_other = -1;

    switch( coord.type )
    {
      case GeoCoord::LATITUDE:
        index_other = child->getIndex() + 1;
        geo_node->setNodeLat(child);
        break;
      case GeoCoord::LONGITUDE:
        index_other = child->getIndex() - 1;
        geo_node->setNodeLon(child);
        break;
      default:
        LOG_GEO_RET("Invalid geo coord")
    }

    const std::string& name = child->getNameString();
    SGPropertyNode *other = child->getParent()->getChild(name, index_other);
    if( !other )
      return;

    GeoCoord coord_other = parseGeoCoord(other->getStringValue());
    if(    coord_other.type == GeoCoord::INVALID
        || coord_other.type == coord.type )
      return;

    GeoNodes::iterator it_geo_node_other = _geo_nodes.find(other);
    if( it_geo_node_other == _geo_nodes.end() )
      LOG_GEO_RET("other geo node not found!")
    GeoNodePair* geo_node_other = it_geo_node_other->second.get();

    // Let use both nodes use the same GeoNodePair instance
    if( geo_node_other != geo_node )
      it_geo_node_other->second = it_geo_node->second;

    if( coord_other.type == GeoCoord::LATITUDE )
      geo_node->setNodeLat(other);
    else
      geo_node->setNodeLon(other);

    // Set name for resulting screen coordinate nodes
    geo_node->setTargetName( name.substr(0, name.length() - GEO.length()) );
  }

  //----------------------------------------------------------------------------
  void Map::hdgNodeChanged(SGPropertyNode* child)
  {
    child->getParent()->setFloatValue(
      "tf[0]/rot",
      SGMiscf::deg2rad(child->getFloatValue() - _projection->orientation())
    );
  }

  //----------------------------------------------------------------------------
  Map::GeoCoord Map::parseGeoCoord(const std::string& val) const
  {
    GeoCoord coord;
    if( val.length() < 2 )
      return coord;

    if( val[0] == 'N' || val[0] == 'S' )
      coord.type = GeoCoord::LATITUDE;
    else if( val[0] == 'E' || val[0] == 'W' )
      coord.type = GeoCoord::LONGITUDE;
    else
      return coord;

    char* end;
    coord.value = strtod(&val[1], &end);

    if( end != &val[val.length()] )
    {
      coord.type = GeoCoord::INVALID;
      return coord;
    }

    if( val[0] == 'S' || val[0] == 'W' )
      coord.value *= -1;

    return coord;
  }

} // namespace canvas
} // namespace simgear
