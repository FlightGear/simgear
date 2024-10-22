// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

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
