// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief A group of 2D Canvas elements which get automatically transformed according to the map parameters.
 */

#ifndef CANVAS_MAP_HXX_
#define CANVAS_MAP_HXX_

#include "CanvasGroup.hxx"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace simgear
{
namespace canvas
{
  class GeoNodePair;
  class HorizontalProjection;
  class Map:
    public Group
  {
    public:
      static const std::string TYPE_NAME;
      static void staticInit();

      Map( const CanvasWeakPtr& canvas,
           const SGPropertyNode_ptr& node,
           const Style& parent_style,
           ElementWeakPtr parent = 0 );
      virtual ~Map();

    protected:
      void updateImpl(double dt) override;

      void updateProjection(SGPropertyNode* type_node);

      void childAdded(SGPropertyNode* parent, SGPropertyNode* child) override;
      void childRemoved(SGPropertyNode* parent, SGPropertyNode* child) override;
      void valueChanged(SGPropertyNode* child) override;
      void childChanged(SGPropertyNode* child) override;

      using GeoNodes =
        std::unordered_map<SGPropertyNode*, std::shared_ptr<GeoNodePair>>;
      using NodeSet = std::unordered_set<SGPropertyNode*>;

      GeoNodes _geo_nodes;
      NodeSet  _hdg_nodes;
      std::shared_ptr<HorizontalProjection> _projection;
      bool _projection_dirty = false;

      struct GeoCoord
      {
        enum
        {
          INVALID,
          LATITUDE,
          LONGITUDE
        } type = INVALID;
        double value = 0;
      };

      void projectionNodeChanged(SGPropertyNode* child);
      void geoNodeChanged(SGPropertyNode* child);
      void hdgNodeChanged(SGPropertyNode* child);

      GeoCoord parseGeoCoord(const std::string& val) const;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_MAP_HXX_ */
