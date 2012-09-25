// Copyright (C) 2008 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef BVHPageNodeOSG_hxx
#define BVHPageNodeOSG_hxx

#include "../../bvh/BVHPageNode.hxx"

#include <osg/ref_ptr>
#include <osg/Referenced>

namespace simgear {

class BVHPageNodeOSG : public BVHPageNode {
public:
    BVHPageNodeOSG(const std::string& name, const SGSphered& boundingSphere,
                   const osg::ref_ptr<const osg::Referenced>& options);
    BVHPageNodeOSG(const std::vector<std::string>& nameList,
                   const SGSphered& boundingSphere,
                   const osg::ref_ptr<const osg::Referenced>& options);
    virtual ~BVHPageNodeOSG();
    
    virtual BVHPageRequest* newRequest();

    void setBoundingSphere(const SGSphered& sphere);

    static SGSharedPtr<BVHNode>
    load(const std::string& name, const osg::ref_ptr<const osg::Referenced>& options);

protected:
    virtual SGSphered computeBoundingSphere() const;
    virtual void invalidateBound();

private:
    class _NodeVisitor;
    class _Request;

    /// The submodels apropriate for intersection tests.
    std::vector<std::string> _modelList;
    /// The bounding sphere as given by the lod node.
    SGSphered _boundingSphere;
    /// The osg loader options that are active for this subtree
    osg::ref_ptr<const osg::Referenced> _options;
};

}

#endif
