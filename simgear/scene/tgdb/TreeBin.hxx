/* -*-c++-*-
 *
 * Copyright (C) 2008 Stuart Buchanan
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

#ifndef TREE_BIN_HXX
#define TREE_BIN_HXX

#include <vector>
#include <string>

#include <osg/Geometry>
#include <osg/Group>
#include <osg/Matrix>

#include <simgear/scene/util/OsgMath.hxx>

namespace simgear
{
class TreeBin {
public:
  struct Tree {
    Tree(const SGVec3f& p) :
      position(p)
    { }
    SGVec3f position;
  };

    typedef std::vector<Tree> TreeList;

    int texture_varieties;
    double range;
    float height;
    float width;
    std::string texture;
    
    void insert(const Tree& t)
    { _trees.push_back(t); }
    void insert(const SGVec3f& p, int t, float s)
    { insert(Tree(p)); }

    unsigned getNumTrees() const
    { return _trees.size(); }
    const Tree& getTree(unsigned i) const
    { return _trees[i]; }
    TreeList _trees;
    
    ~TreeBin() {
      _trees.clear();
    }    
};

typedef std::list<TreeBin*> SGTreeBinList;

osg::Group* createForest(SGTreeBinList& forestList, const osg::Matrix& transform,
                         const SGReaderWriterOptions* options);
}
#endif
