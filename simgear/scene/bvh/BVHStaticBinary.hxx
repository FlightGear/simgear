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

#ifndef BVHStaticBinary_hxx
#define BVHStaticBinary_hxx

#include <simgear/math/SGGeometry.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include "BVHStaticNode.hxx"

namespace simgear {

class BVHStaticBinary : public BVHStaticNode {
public:
  BVHStaticBinary(unsigned splitAxis, const BVHStaticNode* leftChild,
                   const BVHStaticNode* rightChild, const SGBoxf& box);
  virtual ~BVHStaticBinary();
  virtual void accept(BVHVisitor& visitor, const BVHStaticData& data) const;

  void traverse(BVHVisitor& visitor, const BVHStaticData& data) const
  {
    _leftChild->accept(visitor, data);
    _rightChild->accept(visitor, data);
  }

  // Traverse call that first enters the child node that is potentially closer
  // to the given point than the other.
  template<typename T>
  void traverse(BVHVisitor& visitor, const BVHStaticData& data,
                const SGVec3<T>& pt) const
  {
    float center = 0.5f*(_boundingBox.getMin()[_splitAxis]
                         + _boundingBox.getMax()[_splitAxis]);
    if (pt[_splitAxis] < center) {
      _leftChild->accept(visitor, data);
      _rightChild->accept(visitor, data);
    } else {
      _rightChild->accept(visitor, data);
      _leftChild->accept(visitor, data);
    }
  }

  unsigned getSplitAxis() const
  { return _splitAxis; }

  const BVHStaticNode* getLeftChild() const
  { return _leftChild; }
  const BVHStaticNode* getRightChild() const
  { return _rightChild; }

  const SGBoxf& getBoundingBox() const
  { return _boundingBox; }

private:
  // Note the order of the members, this is to avoid padding
  unsigned _splitAxis;
  SGSharedPtr<const BVHStaticNode> _leftChild;
  SGSharedPtr<const BVHStaticNode> _rightChild;
  SGBoxf _boundingBox;
};

}

#endif
