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

#include <vector>
#include <simgear/math/SGMath.hxx>

// forward decls  
class SGMatModel;
   
class SGMatModelBin {
public:
  struct MatModel {
    MatModel(const SGVec3f& p, SGMatModel *m, int l, float rot) :
      position(p), model(m), lod(l), rotation(rot)
    { }
    SGVec3f position;
    SGMatModel *model;
    int lod;
    float rotation;
  };
  typedef std::vector<MatModel> MatModelList;

  void insert(const MatModel& model)
  { 
    _models.push_back(model);   
  }
  
  void insert(const SGVec3f& p, SGMatModel *m, int l, float rot)
  { insert(MatModel(p, m, l, rot)); }

  unsigned getNumModels() const
  { return _models.size(); }
  const MatModel& getMatModel(unsigned i) const
  { return _models[i]; }
  
private:
  MatModelList _models;
};

#endif
