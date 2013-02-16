// Copyright (C) 2008 - 2009  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifndef BVHSubTreeCollector_hxx
#define BVHSubTreeCollector_hxx

#include <simgear/math/SGGeometry.hxx>

#include "BVHVisitor.hxx"
#include "BVHNode.hxx"
#include "BVHGroup.hxx"
#include "BVHStaticNode.hxx"

namespace simgear {

/// Visitor to subcollect parts of an existing bounding volume tree.
/// Given a sphere, it takes those sub parts of the tree that intersect the
/// sphere. Transform nodes of any kind are preserved to be able to ask for
/// intersections a different times. The subcollected tree is kept as small as
/// possible as it does avoid having groups with single childs.
/// Also the BVHStaticGeometry parts are seached for subtrees that are outside
/// the sphere.

class BVHSubTreeCollector : public BVHVisitor {
public:
    typedef std::vector<SGSharedPtr<BVHNode> > NodeList;
    
    BVHSubTreeCollector(const SGSphered& sphere = SGSphered());
    virtual ~BVHSubTreeCollector();
    
    virtual void apply(BVHGroup&);
    virtual void apply(BVHPageNode&);
    virtual void apply(BVHTransform&);
    virtual void apply(BVHMotionTransform&);
    virtual void apply(BVHLineGeometry&);
    virtual void apply(BVHStaticGeometry&);
    
    virtual void apply(const BVHStaticBinary&, const BVHStaticData&);
    virtual void apply(const BVHStaticTriangle&, const BVHStaticData&);
    
    void setSphere(const SGSphered& sphere)
    { _sphere = sphere; }
    const SGSphered& getSphere() const
    { return _sphere; }

    bool haveChildren() const
    { return !_nodeList.empty(); }
    
    void addNode(BVHNode* node);
    
    void pushNodeList(NodeList& parentNodeList)
    { _nodeList.swap(parentNodeList); }
    void popNodeList(NodeList& parentNodeList, BVHGroup* transform);
    void popNodeList(NodeList& parentNodeList);

    SGSharedPtr<BVHNode> getNode() const;
    
protected:
    NodeList _nodeList;
    SGSharedPtr<const BVHStaticNode> _staticNode;
    
private:
    SGSphered _sphere;
};

}

#endif
