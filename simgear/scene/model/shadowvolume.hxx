// Shadow volume class
//
// Written by Harald JOHNSEN, started June 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//

#ifndef _SHADOWVOLUME_HXX
#define _SHADOWVOLUME_HXX

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>

#include <vector>
#include <map>

using std::vector;
using std::map;

class ssgBranch;
class ssgLeaf;
class SGPropertyNode;

/**
 * A class wich add shadows in a postprocess stage.
 */
class SGShadowVolume {

public:
	SGShadowVolume( ssgBranch *root );
	~SGShadowVolume();

	typedef enum {
		occluderTypeAircraft,
		occluderTypeAI,
		occluderTypeTileObject
	} OccluderType;

	void init(SGPropertyNode *sim_rendering_options);
	void startOfFrame(void);
	void addOccluder(ssgBranch *occluder, OccluderType occluder_type, ssgBranch *tile=0);
	void deleteOccluder(ssgBranch *occluder);
	void deleteOccluderFromTile(ssgBranch *tile);
	void setupShadows(double lon, double lat,
		double gst, double SunRightAscension, double SunDeclination, double sunAngle );
	void endOfFrame(void);
	static int ACpostTravCB( ssgEntity *entity, int traversal_mask );

private:

	class ShadowCaster
	{
	public:
		typedef struct {
			sgVec4 planeEquations;
			int neighbourIndices[3];
			bool isSilhouetteEdge[3];
			bool isFacingLight;
		} triData;

		ssgSharedPtr<ssgBranch> geometry_leaf;
		ssgSharedPtr<ssgBranch> scenery_object;
		ssgSharedPtr<ssgBranch> lib_object;
		ssgSharedPtr<ssgBranch> first_select;
		sgVec3 last_lightpos;
		sgMat4 last_transform;
		int frameNumber;

		int *indices;
		int numTriangles;
		triData *triangles;

		sgVec4 * vertices;
		GLushort *silhouetteEdgeIndices;
		int lastSilhouetteIndicesCount;
		bool isTranslucent;

		ShadowCaster( int _num_tri, ssgBranch * _geometry_leaf );
		~ShadowCaster();
		void addLeaf( int & tri_idx, int & ind_idx, ssgLeaf *_geometry_leaf );
		void SetConnectivity();
		void CalculateSilhouetteEdges(sgVec3 lightPosition);
		void DrawInfiniteShadowVolume(sgVec3 lightPosition, bool drawCaps);
		void computeShadows(sgMat4 rotation, sgMat4 rotation_translation, OccluderType occluder_type);
		void getNetTransform ( ssgBranch * branch, sgMat4 xform );
		bool isSelected (  ssgBranch * branch, float dist);

		bool sameVertex(int edge1, int edge2);
	};
	typedef vector<ShadowCaster *> ShadowCaster_list;

	class SceneryObject {
	public:
		SceneryObject(ssgBranch *_scenery_object, OccluderType _occluder_type);
		~SceneryObject();
		void computeShadows(void);
		void traverseTree(ssgBranch *branch);
		void find_trans(void);
		ssgSharedPtr<ssgBranch> scenery_object;
		ssgSharedPtr<ssgBranch> lib_object;
		ssgSharedPtr<ssgBranch> pending_object;
		ssgSharedPtr<ssgBranch> tile;
		ShadowCaster_list parts;
		OccluderType occluder_type;
	};
	typedef map<ssgSharedPtr<ssgBranch>, SceneryObject *> SceneryObject_map;


private:
	void update_light_view(void);
	void computeShadows(void);
	void cull ( ssgBranch *b, sgFrustum *f, sgMat4 m, int test_needed );

	bool	shadows_enabled;
	bool	shadowsAC_enabled, shadowsAI_enabled, shadowsTO_enabled, shadowsDebug_enabled;
	bool	shadowsAC_transp_enabled;
	bool	use_alpha;
	bool	canDoAlpha, canDoStencil;
	SGPropertyNode_ptr sim_rendering;

	sgVec3 sunPos;
	int frameNumber;
	int lastTraverseTreeFrame;
	sgMat4 CameraViewM;
	double	sun_angle;
	SceneryObject_map sceneryObjects;
	ssgSharedPtr<ssgBranch> ssg_root;
	bool shadows_rendered;
};

#endif // _SHADOWVOLUME_HXX
