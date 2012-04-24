/* -*-c++-*-
 *
 * Copyright (C) 2011 Stuart Buchanan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef SG_BUILDING_BIN_HXX
#define SG_BUILDING_BIN_HXX

#include <math.h>

#include <vector>
#include <string>

#include <osg/Geometry>
#include <osg/Group>
#include <osg/Matrix>

#include <simgear/scene/util/OsgMath.hxx>

namespace simgear
{
class SGBuildingBin {
public:

  enum BuildingType {
    SMALL = 0,
    MEDIUM,
    LARGE };      

  struct Building {
    Building(BuildingType t, const SGVec3f& p, float w, float d, float h, int f, float rot, bool pitch) :
      type(t), 
      position(p), 
      width(w), 
      depth(d), 
      height(h), 
      floors(f),
      rotation(rot), 
      pitched(pitch), 
      radius(std::max(d, 0.5f*w))
    { }
    Building(const SGVec3f& p, Building b) :
      type(b.type), 
      position(p), 
      width(b.width), 
      depth(b.depth), 
      height(b.height),
      floors(b.floors),
      rotation(b.rotation), 
      pitched(b.pitched),
      radius(std::max(b.depth, 0.5f*b.width))
    { }  
    
    BuildingType type;
    SGVec3f position;
    float width;
    float depth;
    float height;
    int floors;
    float rotation;
    bool pitched;
    float radius;
    
    float getFootprint() {
      return radius;
    }
  };
  
  typedef std::vector<Building> BuildingList;
  BuildingList buildings;
  
  std::string texture;

  void insert(const Building& model)
  { 
    buildings.push_back(model);   
  }
  
  void insert(BuildingType t, const SGVec3f& p, float w, float d, float h, int f, float rot, bool pitch)
  { insert(Building(t, p, w, d, h, f, rot, pitch)); }

  unsigned getNumBuildings() const
  { return buildings.size(); }
  const Building& getBuilding(unsigned i) const
  { return buildings[i]; }  
};

// List of buildings
typedef std::list<SGBuildingBin*> SGBuildingBinList;

osg::Group* createRandomBuildings(SGBuildingBinList buildinglist, const osg::Matrix& transform,
                         const SGReaderWriterOptions* options);
}
#endif
