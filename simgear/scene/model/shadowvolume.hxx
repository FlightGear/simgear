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
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#ifndef _SHADOWVOLUME_HXX
#define _SHADOWVOLUME_HXX


#include <plib/sg.h>
#include <vector>
#include <map>

SG_USING_STD(vector);
SG_USING_STD(map);

class ssgBranch;
class ssgLeaf;
class SGPropertyNode;

/**
 * A class wich add shadows in a postprocess stage.
 */
class SGShadowVolume {

public:
	SGShadowVolume();
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

		ssgBranch *geometry_leaf;
		ssgBranch *scenery_object;
		ssgBranch *lib_object;
		ssgBranch *first_select;
		sgVec3 last_lightpos;
		int frameNumber;

		int *indices;
		int numTriangles;
		triData *triangles;

		sgVec4 * vertices;
		GLushort *silhouetteEdgeIndices;
		int lastSilhouetteIndicesCount;


		ShadowCaster( int _num_tri, ssgBranch * _geometry_leaf );
		~ShadowCaster();
		void addLeaf( int & tri_idx, int & ind_idx, ssgLeaf *_geometry_leaf );
		void SetConnectivity();
		void CalculateSilhouetteEdges(sgVec3 lightPosition);
		void DrawInfiniteShadowVolume(sgVec3 lightPosition, bool drawCaps);
		void computeShadows(sgMat4 rotation, sgMat4 rotation_translation);
		void getNetTransform ( ssgBranch * branch, sgMat4 xform );
		bool isSelected (  ssgBranch * branch );

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
		ssgBranch *scenery_object;
		ssgBranch *lib_object;
		ssgBranch *pending_object;
		ssgBranch *tile;
		ShadowCaster_list parts;
		OccluderType occluder_type;
	};
	typedef map<ssgBranch *, SceneryObject *> SceneryObject_map;


private:
	void update_light_view(void);
	void computeShadows(void);

	bool	init_done;
	bool	shadows_enabled;
	bool	shadowsAC_enabled, shadowsAI_enabled, shadowsTO_enabled, shadowsDebug_enabled;
	bool	use_alpha;
	bool	canDoAlpha, canDoStencil;
	SGPropertyNode *sim_rendering;

	sgVec3 sunPos;
	int frameNumber;
	int lastTraverseTreeFrame;
	sgMat4 CameraViewM;
	sgMat4 invViewAngle;
	double	sun_angle;
	SceneryObject_map sceneryObjects;
	/** this sphere contains the visible scene and is used to cull shadow casters */
	sgSphere frustumSphere;
	/** this sphere contains the near clip plane and is used to check the need of a zfail */
	sgSphere nearClipSphere;

};

#endif // _SHADOWVOLUME_HXX
