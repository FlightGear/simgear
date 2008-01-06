/* -*-c++-*-
 *
 * Copyright (C) 2007 Stuart Buchanan
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

#ifndef SG_MAT_MODEL_BIN_HXX
#define SG_MAT_MODEL_BIN_HXX

#include <math.h>

class SGMatModelBin {
public:
  struct MatModel {
    MatModel(const SGVec3f& p, SGMatModel *m) :
      position(p), model(m)
    { }
    SGVec3f position;
    SGMatModel *model;
  };
  typedef std::vector<MatModel> MatModelList;

  void insert(const MatModel& model)
  { 
    float x = model.position.x();
    float y = model.position.y();
    float z = model.position.z();
  
    if (_models.size() == 0) 
    {
      min_x = x;
      max_x = x;
      
      min_y = y;
      max_y = y;
      
      min_z = z;
      max_z = z;
    }
    else
    {      
      min_x = SGMisc<float>::min(min_x, x);
      max_x = SGMisc<float>::max(max_x, x);

      min_y = SGMisc<float>::min(min_y, y);
      max_y = SGMisc<float>::max(max_y, y);

      min_z = SGMisc<float>::min(min_z, z);
      max_z = SGMisc<float>::max(max_z, z);
    }
    
    _models.push_back(model);   
  }
  
  void insert(const SGVec3f& p, SGMatModel *m)
  { insert(MatModel(p, m)); }

  unsigned getNumModels() const
  { return _models.size(); }
  const MatModel& getMatModel(unsigned i) const
  { return _models[i]; }
  
private:
  MatModelList _models;
  float min_x;
  float max_x;
  float min_y;
  float max_y;
  float min_z;
  float max_z;
};

#endif
