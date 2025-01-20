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
#include <osg/LOD>

#include <simgear/scene/util/OsgMath.hxx>

namespace simgear
{
class TreeBin final {
public:
    TreeBin() = default;
    TreeBin(const SGMaterial *mat);
    TreeBin(const SGPath& absoluteFileName, const SGMaterial *mat);

    ~TreeBin() = default;   // non-virtual intentional

    int texture_varieties;
    double range;
    float height;
    float width;
    std::string texture;
    std::string normal_map;
    std::string teffect;
    
    void insert(osg::Vec3d t)
    { _trees.push_back(t); }

    void insert(const SGVec3f& p)
    { _trees.push_back(toOsg(p)); }

    unsigned getNumTrees() const
    { return _trees.size(); }

    const osg::Vec3d getTree(unsigned i) const
    {
        assert(i < _trees.size());
        return _trees.at(i);
    }

    std::vector<osg::Vec3d> _trees;
};

typedef std::list<TreeBin*> SGTreeBinList;

osg::Group* createForest(SGTreeBinList& forestList, osg::ref_ptr<simgear::SGReaderWriterOptions> options);
}
#endif
