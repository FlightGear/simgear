//------------------------------------------------------------------------------
// File : SkyBVTree.hpp
//------------------------------------------------------------------------------
// Empyrean / SkyWorks : Copyright 2002 Mark J. Harris
//------------------------------------------------------------------------------
/**
* @file SkyBVTree.hpp
* 
* This source was written by Wesley Hunt.  Many thanks to him for providing it.
*/
//-----------------------------------------------------------------------------
// SkyBVTree.hpp
//
// Author:	Wesley Hunt (hunt@cs.unc.edu)
// Date:	2000/12/19
//-----------------------------------------------------------------------------
//	Overview
//	--------
//		declare an SkyBVTree with whatever object type and bounding volume type you
//		want. The hard part is to pass in a NodeSplitter class that determines how
//		to split a node up. See below for a description of its requirements.
//		Finally, use that tree like so:
//			* BeginTree()
//			*   AddObject(Object, ObjectBV)
//			*   ...
//			* EndTree()
//			< Do whatever you want with GetRoot() >
//			< Use GetLeftChild() and GetRightChild() to traverse the nodes >
//
//		The class hierarchy is designed for a flexible, simple public interface.
//		This pushes some extra complexity into the class hierarchy, but there are two
//		main advantages to the final approach:
//			1. There are no interface requirements for the Object and BoundingVolume
//			   template parameters. This means you don't have to modify your existing
//			   class interfaces to use them with the tree. 
//			2. All the dependent logic for dealing with the bounding volumes is pushed 
//			   into the NodeSplitter class. So there is one centralized location for 
//			   adapting your bounding volume classes to work with the tree. See the 
//			   description of the NodeSplitter template requirements for more details.
//
//	Class Descriptions
//	------------------
//
//		SkyBaseBVTree
//		-------------
//		declares the node class that the tree holds. It exposes
//		the public functions of the nodes and defines protected accessors that
//		the derived tree class can use to manipulate nodes and build the tree.
//
//			Node
//			----
//			This is the class that gives access to the tree. You can access each
//			object and bounding volume owned by a node, along with that node's
//			children.
//
//			NodeObject
//			----------------
//			An aggregation of an object and its associated bounding volume.
//			Each node in the tree essentially owns an array of NodeObjects. This
//			array is passed to the NodeSplitter class to allow it to examine a 
//			node's contained objects before making a decision on how to split the 
//			node up.
//
//		SkyBVTree
//		---------
//		The main tree class. To use it, supply and object type, a bounding volume
//		type, and a NodeSplitter class that is used to split the nodes up during
//		tree construction.
//
//	Template class requirements
//	---------------------------
//
//		Object
//		------
//		None
//
//		BoundingVolume
//		--------------
//		None
//
//		NodeSplitter
//		------------
//		This is the user-supplied class that decides how to split a node during
//		tree construction. It is given an array of NodeObjects that the node owns and 
//		is responsible for determining the node's bounding volume as well as how to 
//		split the node up into left and right children.
//		The required API is as follows:
//		
//			* a constructor that takes an array of NodeObject and an unsigned int giving 
//			  the size of the array. These are the objects owned by the node to be split.
//			* a method GetNodeBV() that returns the BoundingVolume of the  node. 
//			  Typically this is the union of the bounding volumes of the objects owned 
//			  by the node.
//			* A unary function operator used to partition the objects in the node. The
//			  operator must take a single NodeObject as a parameter and return as
//			  a bool whether to place the NodeObject in the left or right child. 
//			  Basically, it should be compatible with std::partition using NodeObjects.
//			* A binary function operator used to sort the objects. If a partition fails, 
//			  the nodes are then sorted based on this function and half are sent to each child.
//			  This operator must define a total ordering of the NodeObjects.
//			  Basically, it should be compatible with std::sort using NodeObjects.
//			  
//		Example:
//			struct NodeSplitter
//			{
//				NodeSplitter(const NodeObject* objs, unsigned int numObjs);
//				BoundingVolume& GetNodeBV() const;
//				// Partition predicate
//				bool operator()(const NodeObject& obj) const;
//				// Sort predicate
//				bool operator()(const NodeObject& obj1, const NodeObject& obj2) const;
//			};
//
//-----------------------------------------------------------------------------
#ifndef __SKYBVTREE_HPP__
#define __SKYBVTREE_HPP__

#include <algorithm>
#include <vector>

//-----------------------------------------------------------------------------
// SkyBaseBVTree<Object, BoundingVolume>
//-----------------------------------------------------------------------------
// See header description for details.
//-----------------------------------------------------------------------------
template <class Object, class BoundingVolume>
class SkyBaseBVTree
{
public:
  typedef BoundingVolume BV;
  class NodeObject;
  
public:
  class Node
  {
    friend class SkyBaseBVTree<Object, BoundingVolume>;
  public:
    Node() : _pObjs(NULL), _iNumObjs(0) {}
    Node(NodeObject* pObjs, unsigned int iNumObjs) : _pObjs(pObjs), _iNumObjs(iNumObjs)  {}
  
    // Public interface
    const Object& GetObj(unsigned int index) const { assert(_iNumObjs != 0 && _pObjs != NULL && index < _iNumObjs); return _pObjs[index].GetObj(); }
    const BV&     GetBV(unsigned int index) const  { assert(_iNumObjs != 0 && _pObjs != NULL && index < _iNumObjs); return _pObjs[index].GetBV(); }
    unsigned int  GetNumObjs() const      { assert(_iNumObjs != 0 && _pObjs != NULL); return _iNumObjs; }
    const BV&     GetNodeBV() const       { assert(_iNumObjs != 0 && _pObjs != NULL); return _volume; }
    const Node*   GetLeftChild() const    { assert(_iNumObjs != 0 && _pObjs != NULL); return this+1; }
    const Node*   GetRightChild() const   { assert(_iNumObjs != 0 && _pObjs != NULL); return this+(GetLeftChild()->GetNumObjs()<<1); }
    bool          IsLeaf() const          { assert(_iNumObjs != 0 && _pObjs != NULL); return _iNumObjs == 1; }
  
  private:
    // List of Objects owned by the node
    NodeObject    *_pObjs;
    unsigned int  _iNumObjs;
    BV            _volume;
  };

public:
  class NodeObject
  {
  public:
    NodeObject(const Object& o, const BV& v) : _obj(o), _volume(v) {}
  
    const Object& GetObj() const { return _obj;    }
    const BV&     GetBV() const  { return _volume; }
  private:
    Object _obj;
    BV     _volume;
  };
      
protected:
  // Give non-const access to the node for descendant classes to build the tree
  BV& GetNodeBV(Node* pNode)          { assert(pNode->_iNumObjs != 0 && pNode->_pObjs != NULL); return pNode->_volume; }
  Node* GetLeftChild(Node* pNode)		  { assert(pNode->_iNumObjs != 0 && pNode->_pObjs != NULL); return pNode+1; }
  Node* GetRightChild(Node* pNode)		{ assert(pNode->_iNumObjs != 0 && pNode->_pObjs != NULL); return pNode+(GetLeftChild(pNode)->GetNumObjs()<<1); }
  NodeObject* GetObjs(Node* pNode)    { assert(pNode->_iNumObjs != 0 && pNode->_pObjs != NULL); return pNode->_pObjs; }
  
  // Links a node's children by assigning the given number of objects to each child
  // assumes the node has already had it's objects partitioned
  void LinkNodeChildren(Node* pNode, unsigned int iLeftNumObjs)
  {
    assert(pNode->_iNumObjs != 0 && pNode->_pObjs != NULL); 
    GetLeftChild(pNode)->_pObjs     = pNode->_pObjs;
    GetLeftChild(pNode)->_iNumObjs  = iLeftNumObjs;
    GetRightChild(pNode)->_pObjs    = pNode->_pObjs + iLeftNumObjs;
    GetRightChild(pNode)->_iNumObjs = pNode->_iNumObjs - iLeftNumObjs;
  }        
};


//------------------------------------------------------------------------------
// Function     	  : ClearVector
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn ClearVector(std::vector<T>& vec)
 * @brief This utility function uses the std::vector::swap trick to free the memory of a vector
 *
 * This is necessary since clear() doesn't actually free anything.
 */ 
template<class T>
void ClearVector(std::vector<T>& vec)
{
  std::vector<T>().swap(vec);
}


//-----------------------------------------------------------------------------
// SkyBVTree<Object, BoundingVolume, NodeSplitter>
//-----------------------------------------------------------------------------
// See header description for details.
//-----------------------------------------------------------------------------
template <class Object, class BoundingVolume, class NodeSplitter>
class SkyBVTree : public SkyBaseBVTree<Object, BoundingVolume>
{
public:
  typedef SkyBaseBVTree<Object, BoundingVolume> BaseTree;
  typedef BaseTree::BV BV;
  typedef BaseTree::NodeObject NodeObject;
  typedef BaseTree::Node Node;
  
  void Clear()
  {
    BeginTree();
  }
  
  void BeginTree(unsigned int iNumObjs = 0)
  {
    ClearVector(_objList);
    ClearVector(_nodes);
    if (iNumObjs > 0) _objList.reserve(iNumObjs);
  }
  
  void AddObject(const Object &obj, const BV& volume)
  {
    _objList.push_back(NodeObject(obj, volume));
  }
  
  void EndTree()
  {
    if (_objList.size() == 0) return;
    // Initialize the root node with all the objects
    _nodes.push_back(Node(&_objList[0], _objList.size()));
    // create room for the other nodes. They are initialized in BuildTree().
    _nodes.reserve(_objList.size()*2-1);
    _nodes.resize(_objList.size()*2-1);
    BuildTree(&_nodes[0]);
  }
  
  const Node *GetRoot() const { return _nodes.empty() ? NULL : &_nodes[0]; }
  
  // Memory usage info
  unsigned int CalcMemUsage() const
  {
    unsigned int usage = 0;
    usage += sizeof(*this);
    usage += _objList.capacity() * sizeof(_objList[0]);
    usage += _nodes.capacity() * sizeof(_nodes[0]);
    return usage;
  }
  
public:
private:
  // Does the real work
  void BuildTree(Node *pCurNode)
  {
    int iLeftNumObjs;
    {
      // Initialize the node splitter with the current node
      NodeSplitter splitter(GetObjs(pCurNode), pCurNode->GetNumObjs());
      
      // set the node's bounding volume using the splitter
      GetNodeBV(pCurNode) = splitter.GetNodeBV();
      
      // When a node has one object we can stop
      if (pCurNode->GetNumObjs() == 1) return;
      
      // Try and partition the objects
      iLeftNumObjs = std::partition(GetObjs(pCurNode), &GetObjs(pCurNode)[pCurNode->GetNumObjs()], splitter) - GetObjs(pCurNode);
      
      if ((iLeftNumObjs == 0) || (iLeftNumObjs == pCurNode->GetNumObjs()))
      {
        // Partition failed. Sort and split again to force a complete tree
        std::sort(GetObjs(pCurNode), &GetObjs(pCurNode)[pCurNode->GetNumObjs()], splitter);
        iLeftNumObjs = pCurNode->GetNumObjs() / 2;
      }
    }
    
    LinkNodeChildren(pCurNode, iLeftNumObjs);
    BuildTree(GetLeftChild(pCurNode));
    BuildTree(GetRightChild(pCurNode));
  }
  
  std::vector<Node>       _nodes;
  std::vector<NodeObject> _objList;
};

#endif //__SKYBVTREE_HPP__