// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Groups together two nodes representing a geographic position (lat + lon)
 */

#ifndef CANVAS_GEO_NODE_PAIR_HXX_
#define CANVAS_GEO_NODE_PAIR_HXX_

namespace simgear
{
namespace canvas
{
  class GeoNodePair
  {
    public:
      enum StatusFlags
      {
        LAT_MISSING = 1,
        LON_MISSING = LAT_MISSING << 1,
        INCOMPLETE = LAT_MISSING | LON_MISSING,
        DIRTY = LON_MISSING << 1
      };

      GeoNodePair():
        _status(INCOMPLETE),
        _node_lat(0),
        _node_lon(0)
      {}

      uint8_t getStatus() const
      {
        return _status;
      }

      void setDirty(bool flag = true)
      {
        if( flag )
          _status |= DIRTY;
        else
          _status &= ~DIRTY;
      }

      bool isDirty() const
      {
        return (_status & DIRTY)!=0;
      }

      bool isComplete() const
      {
        return !(_status & INCOMPLETE);
      }

      void setNodeLat(SGPropertyNode* node)
      {
        _node_lat = node;
        _status &= ~LAT_MISSING;
        _xNode.reset();
          
        if( node == _node_lon )
        {
          _node_lon = 0;
          _status |= LON_MISSING;
        }
      }

      void setNodeLon(SGPropertyNode* node)
      {
        _node_lon = node;
        _status &= ~LON_MISSING;
        _yNode.reset();
          
        if( node == _node_lat )
        {
          _node_lat = 0;
          _status |= LAT_MISSING;
        }
      }

      std::string getLat() const
      {
        return _node_lat ? _node_lat->getStringValue() : "";
      }

      std::string getLon() const
      {
        return _node_lon ? _node_lon->getStringValue() : "";
      }

      void setCachedLatLon(const std::pair<double, double>& latLon)
      { _cachedLatLon = latLon; }
      
      std::pair<double, double> getCachedLatLon()
      { return _cachedLatLon; }
      
      void setTargetName(const std::string& name)
      {
        _target_name = name;
        _xNode.reset();
        _yNode.reset();
      }

      void setScreenPos(float x, float y)
      {
        assert( isComplete() );
        if (!_xNode) {
          SGPropertyNode *parent = _node_lat->getParent();
          _xNode = parent->getChild(_target_name, _node_lat->getIndex(), true);
        }
          
        if (!_yNode) {
          SGPropertyNode *parent = _node_lat->getParent();
          _yNode = parent->getChild(_target_name, _node_lon->getIndex(), true);
        }
          
        _xNode->setDoubleValue(x);
        _yNode->setDoubleValue(y);
      }

      void print()
      {
        std::cout << "lat=" << (_node_lat ? _node_lat->getPath() : "")
                  << ", lon=" << (_node_lon ? _node_lon->getPath() : "")
                  << std::endl;
      }

    private:

      uint8_t _status;
      SGPropertyNode *_node_lat,
                     *_node_lon;
      std::string   _target_name;
      SGPropertyNode_ptr _xNode,
          _yNode;
      std::pair<double, double> _cachedLatLon;

  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_GEO_NODE_PAIR_HXX_ */
