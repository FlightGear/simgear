//============================================================================
// File          : SkyBVTreeSplitter.hpp
// 
// Author        : Wesley Hunt
// 
// Content       : NodeSplitter classes for SkyBVTrees using SkyBoundingBox or
//                 SkyBoundingSphere (not implemented).
// 
//============================================================================
#ifndef __SKYBVTREESPLITTER_HPP__
#define __SKYBVTREESPLITTER_HPP__

//----------------------------------------------------------------------------
//-- Includes ----------------------------------------------------------------
//----------------------------------------------------------------------------
#include "SkyBVTree.hpp"
#include "SkyMinMaxBox.hpp"
//#include <Mlx/MlxBoundingSphere.hpp>
//#if _MSC_VER == 1200
//#include <Auxlib/AuxCompileTimeChecker.hpp>
//#endif
//----------------------------------------------------------------------------
//-- Forward Declarations ----------------------------------------------------
//----------------------------------------------------------------------------
// A strategy for splitting nodes compatible with bounding boxes and spheres.
template<class Object> class SkyBoundingBoxSplitter;
// SkyBVTree compatible node splitters implemented using the above strategy.
template<class Object> class SkyAABBTreeSplitter;
//template<class Object> class SkySphereTreeSplitter;

//----------------------------------------------------------------------------
//-- Defines, Constants, Enumerated Types ------------------------------------
//----------------------------------------------------------------------------
const float rLongObjectPercentageTolerance = 0.75f;

//----------------------------------------------------------------------------
//	SkyBoundingBoxSplitter
//----------------------------------------------------------------------------
//	This class defines a NodeSplitter strategy that has the functionality 
//	required by SkyBVTree's NodeSplitter template class. Can be used with
//	SkyMinMaxBox and SkyBoundingSphere.
//
//	It defines a two-tiered split strategy:
//
//	* First it tries to separate large objects from any smaller objects. 
//	* If there are no large objects, it splits along the midpoint of the longest
//	  axis defined by the objects.
//	* Finally, if all else fails, it defines a total ordering along the longest
//	  axis based on the center of each node.
//----------------------------------------------------------------------------
template<class Object>
class SkyBoundingBoxSplitter
{
public:
	typedef SkyBaseBVTree<Object, SkyMinMaxBox>::NodeObject NodeObjectBox;
	//typedef SkyBaseBVTree<Object, SkyBoundingSphere>::NodeObject NodeObjectSphere;

#if _MSC_VER == 1200
	// !!! WRH HACK MSVC++6 SP5 Workaround.
	// VC6 can't disambiguate this constructor because it doesn't consider the two
	// NodeObject templates to be different classes for the purposes of
	// overloading. It won't recognize that the second template parameters are
	// different. Forcing them to be explicit template specializations fixes
	// the problem.
	template<class BV>
	SkyBoundingBoxSplitter(const SkyBaseBVTree<Object, BV>::NodeObject*, unsigned int)
	{
		//AUX_STATIC_CHECK(false, VisualC_6_WorkAround); ???
	}
	template<>
#endif
	SkyBoundingBoxSplitter(const NodeObjectBox* pObjs, unsigned int iNumObjs)
	{
		for (unsigned int i = 0; i < iNumObjs; ++i)
		{
			_nodeBBox.Union(pObjs[i].GetBV());
		}
		Init(pObjs, iNumObjs);
	}
/*#if _MSC_VER == 1200
	template<>
#endif
SkyBoundingBoxSplitter(const NodeObjectSphere* objs, 
#ifdef _PLATFORM_XBOX
	Int32
#else
	UInt32 
#endif
	numObjs)
	{
		for (int i=0; i<numObjs; ++i)
		{
			SkyMinMaxBox box;
			box.AddPoint(objs[i].GetBV().GetCenter());
			box.Bloat(objs[i].GetBV().GetRadius());
			_nodeBBox.Union(box);
		}
		Init(objs, numObjs);
	}*/

	template<class nodeObj>
	bool SplitLeft(const nodeObj& obj) const
	{
		if (_bIsolateLongObjects) 
			return GetSplitAxisLength(obj.GetBV()) < _rMaxObjectLength;
		else
			return GetSplitAxisCenter(obj.GetBV()) < _rSplitValue;
	}

	template<class nodeObj>
	bool LessThan(const nodeObj& obj1, const nodeObj& obj2) const
	{
		return GetSplitAxisCenter(obj1.GetBV()) < GetSplitAxisCenter(obj2.GetBV());
	}

	const SkyMinMaxBox& GetNodeBBox() const { return _nodeBBox; }

private:
	template<class nodeObj>
	void Init(const nodeObj* pObjs, unsigned int iNumObjs)
	{
		_iSplitAxis       = FindSplitAxis(_nodeBBox);
		_rSplitValue      = FindSplitValue(_nodeBBox, _iSplitAxis);
		_rMaxObjectLength = GetSplitAxisLength(_nodeBBox) * rLongObjectPercentageTolerance;

		_bIsolateLongObjects = false;
		for (unsigned int i = 0; i < iNumObjs; ++i)
		{
			if (GetSplitAxisLength(pObjs[i].GetBV()) > _rMaxObjectLength)
			{
				_bIsolateLongObjects = true;
				break;
			}
		}
	}

	int FindSplitAxis(const SkyMinMaxBox& bbox)
	{
		int iAxis = 0, i;
		Vec3f vecExt = bbox.GetMax() - bbox.GetMin();
		for (i = 1; i < 3; ++i)  if (vecExt[i] > vecExt[iAxis]) iAxis = i;
		return iAxis;
	}
	float FindSplitValue(const SkyMinMaxBox& bbox, int iSplitAxis)
	{
		return (bbox.GetMin()[iSplitAxis] + bbox.GetMax()[iSplitAxis])*0.5f;
	}

	/*float GetSplitAxisLength(const SkyBoundingSphere& sphere) const
	{
		return 2.f*sphere.GetRadius();
	}*/
	float GetSplitAxisLength(const SkyMinMaxBox& bbox) const
	{
		return bbox.GetMax()[_iSplitAxis] - bbox.GetMin()[_iSplitAxis];
	}

	float GetSplitAxisCenter(const SkyMinMaxBox& bbox) const
	{
		return (bbox.GetMin()[_iSplitAxis] + bbox.GetMax()[_iSplitAxis]) * 0.5f;
	}
	/*float GetSplitAxisCenter(const SkyBoundingSphere& sphere) const
	{
		return sphere.GetCenter()[SplitAxis];
	}*/

	int   _iSplitAxis;
	float _rSplitValue;
	bool  _bIsolateLongObjects;
	float _rMaxObjectLength;

	SkyMinMaxBox _nodeBBox;
};

//----------------------------------------------------------------------------
//	SkyAABBTreeSplitter
//----------------------------------------------------------------------------
//	A NodeSplitter that is compatible with SkyBVTree for SkyMinMaxBox. 
//	Implemented using the SkyBoundingBoxSplitter strategy.
//----------------------------------------------------------------------------
template<class Object>
class SkyAABBTreeSplitter
{
public:
	typedef SkyMinMaxBox BV;
	typedef SkyBaseBVTree<Object, BV>::NodeObject NodeObject;

	SkyAABBTreeSplitter(const NodeObject* pObjs, unsigned int iNumObjs) : _splitter(pObjs, iNumObjs) {}

	const BV& GetNodeBV() const				{ return _splitter.GetNodeBBox(); }

	bool operator()(const NodeObject& obj) const
	{
		return _splitter.SplitLeft(obj);
	}

	bool operator()(const NodeObject& obj1, const NodeObject& obj2) const
	{
		return _splitter.LessThan(obj1, obj2);
	}
	
private:
	SkyBoundingBoxSplitter<Object> _splitter;
};

//----------------------------------------------------------------------------
//	SkySphereTreeSplitter
//----------------------------------------------------------------------------
//	A NodeSplitter that is compatible with SkyBVTree for SkyBoundingSphere. 
//	Implemented using the SkyBoundingBoxSplitter strategy.
//----------------------------------------------------------------------------
/*template<class Object>
class SkySphereTreeSplitter
{
public:
	typedef SkyBoundingSphere BV;
	typedef SkyBaseBVTree<Object, BV>::NodeObject NodeObject;

	MlxSphereTreeSplitter(const NodeObject* pObjs, unsigned int iNumObjs) : _splitter(pObjs, iNumObjs) 
	{
		_nodeBV = pObjs[0].GetBV();
		for (unsigned int i = 1; i < iNumObjs; ++i) _nodeBV.Union(pObjs[i].GetBV());
	}

	const BV& GetNodeBV() const				{ return _nodeBV; }

	bool operator()(const NodeObject& obj) const
	{
		return _splitter.SplitLeft(obj);
	}

	bool operator()(const NodeObject& obj1, const NodeObject& obj2) const
	{
		return _splitter.LessThan(obj1, obj2);
	}

private:
	BV _nodeBV;
	SkyBoundingBoxSplitter<Object> _splitter;
};*/

#endif //__SKYBVTREESPLITTER_HPP__
