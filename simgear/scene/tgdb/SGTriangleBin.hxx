/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich 
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

#ifndef SG_TRIANGLE_BIN_HXX
#define SG_TRIANGLE_BIN_HXX

#include <vector>
#include <map>
#include "SGVertexArrayBin.hxx"

template<typename T>
class SGTriangleBin : public SGVertexArrayBin<T> {
public:
#define BUILD_EDGE_MAP
  typedef typename SGVertexArrayBin<T>::value_type value_type;
  typedef typename SGVertexArrayBin<T>::index_type index_type;
  typedef SGVec2<index_type> edge_ref;
  typedef SGVec3<index_type> triangle_ref;
  typedef std::vector<triangle_ref> TriangleVector;
  typedef std::vector<index_type> TriangleList;
  typedef std::map<edge_ref,TriangleList> EdgeMap;

  void insert(const value_type& v0, const value_type& v1, const value_type& v2)
  {
    index_type i0 = SGVertexArrayBin<T>::insert(v0);
    index_type i1 = SGVertexArrayBin<T>::insert(v1);
    index_type i2 = SGVertexArrayBin<T>::insert(v2);
    index_type triangleIndex = _triangleVector.size();
    _triangleVector.push_back(triangle_ref(i0, i1, i2));
#ifdef BUILD_EDGE_MAP
    _edgeMap[edge_ref(i0, i1)].push_back(triangleIndex);
    _edgeMap[edge_ref(i1, i2)].push_back(triangleIndex);
    _edgeMap[edge_ref(i2, i0)].push_back(triangleIndex);
#endif
  }

  unsigned getNumTriangles() const
  { return _triangleVector.size(); }
  const triangle_ref& getTriangleRef(index_type i) const
  { return _triangleVector[i]; }
  const TriangleVector& getTriangles() const
  { return _triangleVector; }

#ifdef BUILD_EDGE_MAP
// protected: //FIXME
  void getConnectedSets(std::list<TriangleVector>& connectSets) const
  {
    std::vector<bool> processedTriangles(getNumTriangles(), false);
    for (index_type i = 0; i < getNumTriangles(); ++i) {
      if (processedTriangles[i])
        continue;

      TriangleVector currentSet;
      std::vector<edge_ref> edgeStack;

      {
        triangle_ref triangleRef = getTriangleRef(i);
        edgeStack.push_back(edge_ref(triangleRef[0], triangleRef[1]));
        edgeStack.push_back(edge_ref(triangleRef[1], triangleRef[2]));
        edgeStack.push_back(edge_ref(triangleRef[2], triangleRef[0]));
        currentSet.push_back(triangleRef);
        processedTriangles[i] = true;
      }

      while (!edgeStack.empty()) {
        edge_ref edge = edgeStack.back();
        edgeStack.pop_back();
        
        typename EdgeMap::const_iterator emiList[2] = {
          _edgeMap.find(edge),
          _edgeMap.find(edge_ref(edge[1], edge[0]))
        };
        for (unsigned ei = 0; ei < 2; ++ei) {
          typename EdgeMap::const_iterator emi = emiList[ei];
          if (emi == _edgeMap.end())
            continue;

          for (unsigned ti = 0; ti < emi->second.size(); ++ti) {
            index_type triangleIndex = emi->second[ti];
            if (processedTriangles[triangleIndex])
              continue;

            triangle_ref triangleRef = getTriangleRef(triangleIndex);
            edgeStack.push_back(edge_ref(triangleRef[0], triangleRef[1]));
            edgeStack.push_back(edge_ref(triangleRef[1], triangleRef[2]));
            edgeStack.push_back(edge_ref(triangleRef[2], triangleRef[0]));
            currentSet.push_back(triangleRef);
            processedTriangles[triangleIndex] = true;
          }
        }
      }

      connectSets.push_back(currentSet);
    }
  }
#endif

private:
  TriangleVector _triangleVector;
#ifdef BUILD_EDGE_MAP
  EdgeMap _edgeMap;
#endif
};

#endif
