//============================================================================
// File          : SkyAABBTree.hpp
// 
// Author        : Wesley Hunt
// 
// Content       : axis-aligned bounding box tree
// 
//============================================================================
#ifndef __SKYAABBTREE_HPP__
#define __SKYAABBTREE_HPP__

#include "SkyBVTree.hpp"
#include "SkyBVTreeSplitter.hpp"

template <class object>
class SkyAABBTree : public SkyBVTree<object, SkyMinMaxBox, SkyAABBTreeSplitter<object> > 
{};

#endif //__SKYAABBTREE_HPP__
