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

#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/props/props.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/screen/extensions.hxx>
#include <simgear/scene/model/animation.hxx>
#include <simgear/scene/model/model.hxx>
#include SG_GLU_H

#include "shadowvolume.hxx"

/*
 geometry and edge list
 - traverse object graph until leaf to get geometry
 - what about transform and selection ?
	- use sub objects rather then objects
	- get local transform and selection
		- range anim : ssgRangeSelector ( min, max ) => ssgSelector ( true/false )
			=> selection [0..n], kids [0..n]
		- select/timed anim : ssgSelector ( true/false )
			=> isSelected( nkid )
		- spin/rotate/trans/scale/... : ssgTransform ( matrix )
			=> getNetTransform
 - on new object :
	- for one object branch
		- for each leaf
			- save geometry
			- save address in cache
 - when rendering object
	- for each leaf
		- getNetTransform + object global rotation (ac) => transform for light position
		- go up in tree and check isSelected( self )
 - generate connectivity each time the geometry change
	=> using local light so connectivity never changes
 - generate active edge list when :
	- light moves
	- subpart moves (animation code)
	- ac rotate
		=> cache rotation matrix and generate edge list only if something change
		=> even if it changes, no need to do that every frame

 - static objects have static edge list if light does not move

 shadowing a scene
 - render full scene as normal
 - render occluder in stencil buffer with their shadow volumes
 - apply stencil to framebuffer (darkens shadowed parts)

	shadows using the alpha buffer
	http://wwwvis.informatik.uni-stuttgart.de/~roettger/html/Pages/shadows.html
*/

// TODO
//	- shadow for objects
//		* aircraft
//		* tile objects (from .stg)
//		- ai objects
//		- random objects => tie shadow geometry to lib objects and reuse them
//	- zfail if camera inside shadow
//	- queue geometry work if lot of objects
//	* add a render property on/off (for aircraft, for scene objects, for ai)
//	* add a render property in rendering dialog
//	* filter : halo, light, shadow, disc, disk, flame, (exhaust), noshadow

static int statSilhouette=0;
static int statGeom=0;
static int statObj=0;

static SGShadowVolume *states;
static glBlendEquationProc glBlendEquation = NULL;
#define GL_MIN_EXT                          0x8007
#define GL_MAX_EXT                          0x8008


SGShadowVolume::ShadowCaster::ShadowCaster( int _num_tri, ssgBranch * _geometry_leaf ) : 
	geometry_leaf ( _geometry_leaf ),
	scenery_object ( 0 ),
	first_select ( 0 ),
	frameNumber ( 0 ),
	numTriangles ( 0 ),
	indices ( 0 ),
	vertices ( 0 ),
	lastSilhouetteIndicesCount ( 0 )
{
	int num_tri = _num_tri;
	numTriangles = num_tri;
	triangles = new triData[ num_tri ];
	indices = new int[1 + num_tri * 3];
	vertices = new sgVec4[1 + num_tri * 3];
	silhouetteEdgeIndices = new GLushort[(1+num_tri) * 3*3];
	indices [ num_tri * 3 ] = num_tri * 3;
	sgSetVec3(last_lightpos, 0.0, 0.0, 0.0);
	statGeom ++;

	ssgBranch *branch = (ssgBranch *) _geometry_leaf;
	while( branch && branch->getNumParents() > 0 ) {
		if( !first_select && branch->isA(ssgTypeSelector())) {
			first_select = branch;
			break;
		}
		branch = branch->getParent(0);
	}
}

void SGShadowVolume::ShadowCaster::addLeaf( int & tri_idx, int & ind_idx, ssgLeaf *geometry_leaf ) {
	int num_tri = geometry_leaf->getNumTriangles();
	for(int i = 0; i < num_tri ; i ++ ) {
		short v1, v2, v3;
		sgVec3 a, b, c;
		geometry_leaf->getTriangle( i, &v1, &v2, &v3 );
		sgCopyVec3(a, geometry_leaf->getVertex(v1));
		sgCopyVec3(b, geometry_leaf->getVertex(v2));
		sgCopyVec3(c, geometry_leaf->getVertex(v3));

		int p = tri_idx;
		sgMakePlane ( triangles[p].planeEquations, a, b, c );
		sgCopyVec3(vertices[ind_idx + v1], a);
		sgCopyVec3(vertices[ind_idx + v2], b);
		sgCopyVec3(vertices[ind_idx + v3], c);
		vertices[ind_idx + v1][SG_W] = 1.0f;
		vertices[ind_idx + v2][SG_W] = 1.0f;
		vertices[ind_idx + v3][SG_W] = 1.0f;
		indices[p*3] = ind_idx + v1;
		indices[p*3+1] = ind_idx + v2;
		indices[p*3+2] = ind_idx + v3;

		tri_idx++;
	}
	if( num_tri == 0 )
		return;
	int num_ind = geometry_leaf->getNumVertices();
	ind_idx += num_ind;
}

SGShadowVolume::ShadowCaster::~ShadowCaster() {
	delete [] indices ;
	delete [] vertices ;
	delete [] triangles;
	delete [] silhouetteEdgeIndices;
}



bool SGShadowVolume::ShadowCaster::sameVertex(int edge1, int edge2) {
	if( edge1 == edge2)
		return true;
	sgVec3 delta_v;
	sgSubVec3( delta_v, vertices[edge1], vertices[edge2]);
	if( delta_v[SG_X] != 0.0)	return false;
	if( delta_v[SG_Y] != 0.0)	return false;
	if( delta_v[SG_Z] != 0.0)	return false;
	return true;
}


//Calculate neighbour faces for each edge
// caution, this is O(n2)
void SGShadowVolume::ShadowCaster::SetConnectivity(void)
{
	int edgeCount = 0;

	//set the neighbour indices to be -1
	for(int ii=0; ii<numTriangles; ++ii)
		triangles[ii].neighbourIndices[0] = 
		triangles[ii].neighbourIndices[1] = 
		triangles[ii].neighbourIndices[2] = -1;

	//loop through triangles
	for(int i=0; i<numTriangles-1; ++i)
	{
		//loop through edges on the first triangle
		for(int edgeI=0; edgeI<3; ++edgeI)
		{
			//continue if this edge already has a neighbour set
			if(triangles[i].neighbourIndices[ edgeI ]!=-1)
				continue;

			//get the vertex indices on each edge
			int edgeI1=indices[i*3+edgeI];
			int edgeI2=indices[i*3+(edgeI == 2 ? 0 : edgeI+1)];

			//loop through triangles with greater indices than this one
			for(int j=i+1; j<numTriangles; ++j)
			{
				//loop through edges on triangle j
				for(int edgeJ=0; edgeJ<3; ++edgeJ)
				{
					//continue if this edge already has a neighbour set
					if(triangles[j].neighbourIndices[ edgeJ ]!=-1) {
						continue;
					}
					//get the vertex indices on each edge
					int edgeJ1=indices[j*3+edgeJ];
					int edgeJ2=indices[j*3+(edgeJ == 2 ? 0 : edgeJ+1)];

					//if these are the same (possibly reversed order), these faces are neighbours
#if 0
					//no, we only use reverse order because same order means that
					//the triangle is wrongly oriented and that will cause shadow artifact
					if(	sameVertex(edgeI1, edgeJ1) && sameVertex(edgeI2, edgeJ2) ) {
							// can happens with 'bad' geometry
//							printf("flipped triangle detected...check your model\n");
						continue;
					}
#endif
					if(	sameVertex(edgeI1, edgeJ2) && sameVertex(edgeI2, edgeJ1) )
					{
						int edgeI3=indices[i*3+(edgeI == 0 ? 2 : edgeI-1)];
						int edgeJ3=indices[j*3+(edgeJ == 0 ? 2 : edgeJ-1)];
						if(	sameVertex(edgeI3, edgeJ3) ) {
							// can happens with 'bad' geometry
//							printf("duplicated tri...check your model\n");
							// exit loop
							break;
						}
						triangles[i].neighbourIndices[edgeI]=j;
						triangles[j].neighbourIndices[edgeJ]=i;
						edgeCount ++;
						// exit loop
						j = numTriangles;
						break;
					}
				}
			}
		}
	}
//	printf("submodel has %d triangles and %d shared edges\n", numTriangles, edgeCount);
}

//calculate silhouette edges
void SGShadowVolume::ShadowCaster::CalculateSilhouetteEdges(sgVec3 lightPosition)
{
	//Calculate which faces face the light
	for(int i=0; i<numTriangles; ++i)
	{
		if( sgDistToPlaneVec3 ( triangles[i].planeEquations, lightPosition ) > 0.0 )
			triangles[i].isFacingLight=true;
		else
			triangles[i].isFacingLight=false;
	}

	//loop through faces
	int iEdgeIndices = 0;
	sgVec4 farCap = {-lightPosition[SG_X], -lightPosition[SG_Y], -lightPosition[SG_Z], 1.0f};
	sgCopyVec4( vertices[ numTriangles*3 ], farCap );

	for(int t=0; t < numTriangles; t++) {
		int v = t * 3;
		//if this face is not facing the light, not a silhouette edge
		if(!triangles[t].isFacingLight)
		{
			triangles[t].isSilhouetteEdge[0]=false;
			triangles[t].isSilhouetteEdge[1]=false;
			triangles[t].isSilhouetteEdge[2]=false;
			continue;
		}
		//loop through edges
		for(int j = 0 ; j < 3 ; j++) {
			//this face is facing the light
			//if the neighbouring face is not facing the light, or there is no neighbouring face,
			//then this is a silhouette edge
			if(triangles[t].neighbourIndices[j]==-1 || 
				!triangles[triangles[t].neighbourIndices[j]].isFacingLight ) {
				triangles[t].isSilhouetteEdge[j]=true;
				silhouetteEdgeIndices[ iEdgeIndices++ ] = indices[v+(j == 2 ? 0 : j+1)];
				silhouetteEdgeIndices[ iEdgeIndices++ ] = indices[v+j];
				silhouetteEdgeIndices[ iEdgeIndices++ ] = numTriangles * 3;
			}
			else 
				triangles[t].isSilhouetteEdge[j]=false;
		}
	}
	lastSilhouetteIndicesCount = iEdgeIndices;
}

void SGShadowVolume::ShadowCaster::DrawInfiniteShadowVolume(sgVec3 lightPosition, bool drawCaps)
{
	glEnableClientState ( GL_VERTEX_ARRAY ) ;
	glVertexPointer ( 4, GL_FLOAT, 0, vertices ) ;
	glDrawElements ( GL_TRIANGLES, lastSilhouetteIndicesCount, GL_UNSIGNED_SHORT, silhouetteEdgeIndices ) ;

	//Draw caps if required
	if(drawCaps)
	{
		glBegin(GL_TRIANGLES);
		{
			for(int i=0; i<numTriangles; ++i)
			{
				if(triangles[i].isFacingLight) {
					int v = i*3;
					glVertex3fv( vertices[indices[v+0]] );
					glVertex3fv( vertices[indices[v+1]] );
					glVertex3fv( vertices[indices[v+2]] );
				}
			}
		}
		glEnd();
	}
}


void SGShadowVolume::ShadowCaster::getNetTransform ( ssgBranch * branch, sgMat4 xform )
{
	// one less matmult...
	bool first = true;
	while( branch && branch != lib_object ) {
		if( branch->isA(ssgTypeTransform()) ) {
			sgMat4 transform;
			if( first ) {
				((ssgTransform *) branch)->getTransform( xform );
				first = false;
			} else {
				((ssgTransform *) branch)->getTransform(transform);
				sgPostMultMat4 ( xform, transform ) ;
			}
		}
		branch = branch->getParent( 0 );
	}
	if( first )
		sgMakeIdentMat4 ( xform ) ;
}

// check the value of <select> and <range> animation
// wich have been computed during the rendering
bool SGShadowVolume::ShadowCaster::isSelected (  ssgBranch * branch ) {
	while( branch && branch != lib_object) {
		if( branch->isA(ssgTypeSelector()) )
			if( !((ssgSelector *) branch)->isSelected(0) )
				return false;
		branch = branch->getParent(0);
	}
	return true;
}
/*
	trans ----- personality ---- shared object
	perso1 *-----
	             \
	              *--------* shared object
	             /
	perso2 *-----
*/
void SGShadowVolume::ShadowCaster::computeShadows(sgMat4 rotation, sgMat4 rotation_translation) {
  
	// check the select and range ssgSelector node
	// object can't cast shadow if it is not visible

  	if( first_select && ! isSelected( first_select) )
		return;

		sgMat4 transform ;
		sgMat4 invTransform;
		// get the transformations : this comes from animation code for example
		// or static transf
  		getNetTransform( (ssgBranch *) geometry_leaf, transform );
		sgMat4 transf;
		sgCopyMat4( transf, transform );
		sgPostMultMat4( transf, rotation_translation );
		sgPostMultMat4( transform, rotation );
		sgTransposeNegateMat4 ( invTransform, transform ); 

  		glLoadMatrixf ( (float *) states->CameraViewM ) ;
   		glMultMatrixf( (float *) transf );

 		sgVec3 lightPos;
  		sgCopyVec3( lightPos, states->sunPos );
 		sgXformPnt3( lightPos, invTransform );

		sgVec3 lightPosNorm;
		sgNormaliseVec3( lightPosNorm, lightPos );
		float deltaPos = sgAbs(lightPosNorm[0] - last_lightpos[0]) + 
				sgAbs(lightPosNorm[1] - last_lightpos[1]) +
				sgAbs(lightPosNorm[2] - last_lightpos[2]);
		// if the geometry has rotated/moved enought then
		// we need to recompute the silhouette
		// but this computation does not need to be done each frame
   		if( (deltaPos > 0.0) && ( states->frameNumber - frameNumber > 4)) {
			CalculateSilhouetteEdges( lightPos );
			sgCopyVec3( last_lightpos, lightPosNorm );
			frameNumber = states->frameNumber ;
			statSilhouette ++;
		}

	if( states->shadowsDebug_enabled )
	{
   		glStencilFunc(GL_ALWAYS, 0, ~0);
	  	glDisable( GL_CULL_FACE  );
		glDisable( GL_DEPTH_TEST );
		glDisable(GL_STENCIL_TEST);
		glColorMask(1, 1, 1, 1);
  		glColor4f(0.0, 0.0, 1.0, 1.0);
	  	glBegin(GL_LINES);
		for(int i=0; i<numTriangles; ++i)
		{
			if(!triangles[i].isFacingLight)
				continue;
			int v = i*3;
			//Loop through edges on this face
			sgVec3 vertex1, vertex2, vertex3;
			sgCopyVec3(vertex1, vertices[indices[v+0]]);
			sgCopyVec3(vertex2, vertices[indices[v+1]]);
			sgCopyVec3(vertex3, vertices[indices[v+2]]);

			if(triangles[i].isSilhouetteEdge[0]) {
				glVertex3fv(vertex2);
 				glVertex3fv(vertex1);
			}
			if(triangles[i].isSilhouetteEdge[1]) {
				glVertex3fv(vertex2);
 				glVertex3fv(vertex3);
			}
			if(triangles[i].isSilhouetteEdge[2]) {
				glVertex3fv(vertex3);
 				glVertex3fv(vertex1);
			}
		}
		glEnd();
		glColorMask(0, 0, 0, 0);
 	  	glEnable( GL_CULL_FACE  );
		glEnable( GL_DEPTH_TEST );
		glEnable(GL_STENCIL_TEST);
	}


		// TODO:compute intersection with near clip plane...
 		bool needZFail=false;

		// GL_INCR_WRAP_EXT, GL_DECR_WRAP_EXT

 		if(needZFail)
		{
			//Increment stencil buffer for back face depth fail
			glStencilFunc(GL_ALWAYS, 0, ~0);
			glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
			glCullFace(GL_FRONT);

			DrawInfiniteShadowVolume( lightPos, true);

			//Decrement stencil buffer for front face depth fail
			glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
			glCullFace(GL_BACK);

			DrawInfiniteShadowVolume( lightPos, true);
		}
		else	//using zpass
		{
			//Increment stencil buffer for front face depth pass
			if( states->use_alpha ) {
				glBlendEquation( GL_FUNC_ADD );
				glBlendFunc( GL_ONE, GL_ONE );
 			  	glColor4ub(1, 1, 1, 16);
			} else {
			  	glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
				glStencilFunc(GL_ALWAYS, 0, ~0);
				glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
			}
			glCullFace(GL_BACK);

			DrawInfiniteShadowVolume( lightPos, false);

			//Decrement stencil buffer for back face depth pass
			if( states->use_alpha ) {
				glBlendEquation( GL_FUNC_REVERSE_SUBTRACT );
				glBlendFunc( GL_ONE, GL_ONE );
			  	glColor4ub(1, 1, 1, 16);
			} else {
				glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
			}
			glCullFace(GL_FRONT);

			DrawInfiniteShadowVolume( lightPos, false);
		}
}


void SGShadowVolume::SceneryObject::computeShadows(void) {

 	bool intersect = true;
	// compute intersection with view frustum
	// use pending_object (pointer on obj transform) & tile transform
	// to get position
	sgMat4 position, CamXpos;
	sgFrustum *f = ssgGetFrustum();
	pending_object->getParent(0)->getNetTransform( position );
	sgCopyMat4 ( CamXpos, states->CameraViewM ) ;
	sgPreMultMat4 ( CamXpos, position ) ;

   	sgSphere tmp = *(pending_object->getBSphere()) ;
	if ( tmp.isEmpty () )
		intersect = false;
	else {
		// 7000
		float max_dist = 5000.0f;
		tmp . orthoXform ( CamXpos ) ;
		// cull if too far
		if ( -tmp.center[2] - tmp.radius > max_dist )
			intersect = false;
		else if( tmp.center[2] == 0.0 )
			intersect = true;
		// cull if too small on screen
		else if ( tmp.radius / sgAbs(tmp.center[2]) < 1.0 / 40.0 )
			intersect = false;
		else
			intersect = SSG_OUTSIDE != (ssgCullResult) f -> contains ( &tmp );
	}

 	if( intersect ) {
		if( !scenery_object ) {
			if( states->frameNumber - states->lastTraverseTreeFrame > 5 ) {
				find_trans();
				if( scenery_object )
					traverseTree( pending_object );
					states->lastTraverseTreeFrame = states->frameNumber;
			}
			return;
		}
		sgMat4 rotation, rotation_translation;
		scenery_object->getNetTransform ( rotation_translation );
		// split placement offset into rotation and offset
		// rotation from model inside world
		// we are not interested in translation since the light is very far away
		// without translation we reduce the frequency of updates of the silouhette
		sgCopyMat4( rotation, rotation_translation );
   		sgSetVec4( rotation[3], 0, 0, 0, 1);

		ShadowCaster_list::iterator iShadowCaster;
		for(iShadowCaster = parts.begin() ; iShadowCaster != parts.end() ; iShadowCaster ++ ) {
			(*iShadowCaster)->computeShadows(rotation, rotation_translation);
		}
	}
}

/*
	1) starting at the root of a scene object we follow all branch to find vertex leaf.
	each vertex leaf will create a shadow sub object so that the shadow casted by this
	geometry can be correctly rendered according to select or transform nodes.
	2) models are not optimized for shadows because the geometry is cut by
		- select / transform nodes
		- material differences
	since we are not interested in the geometry material we try to merge same level
	leafs. this can divide the number of shadow sub models by a lot (reducing the cpu
	overhead for matrix computation) and at the end this will reduce the number of
	silouhette edge by a lot too.
*/
static bool filterName(const char *leaf_name) {
	if( ! leaf_name )
		return true;
	char lname[20];
	int l = 0;
	char *buff;
	for( buff = lname; *leaf_name && l < (sizeof( lname )-1); ++buff, l++ )
	     *buff = tolower(*leaf_name++);
	*buff = 0;
	if( !strncmp(lname, "noshadow", 8) )
		return false;
	return true;
}
void SGShadowVolume::SceneryObject::traverseTree(ssgBranch *branch) {
	int num_tri = 0;
	int num_leaf = 0;

	if( sgCheckAnimationBranch( (ssgEntity *) branch ) ) {
		if( ((SGAnimation *) branch->getUserData())->get_animation_type() == 1)
			return;
	}

	for(int i = 0 ; i < branch->getNumKids() ; i++) {
		ssgEntity *this_kid = branch->getKid( i );
		if( this_kid->isAKindOf(ssgTypeLeaf()) ) {
			if( filterName( this_kid->getName()) ) {
				num_tri += ((ssgLeaf *) this_kid)->getNumTriangles();
				num_leaf ++;
			}
		} else
			traverseTree( (ssgBranch *) this_kid );
	}
	if( num_tri > 0) {
		int tri_idx = 0;
		int ind_idx = 0;
		ShadowCaster *new_part = new ShadowCaster( num_tri, branch);
		new_part->scenery_object = scenery_object;
		new_part->lib_object = lib_object;
		for(int i = 0 ; i < branch->getNumKids() ; i++) {
			ssgEntity *this_kid = branch->getKid( i );
			if( this_kid->isAKindOf(ssgTypeLeaf()) ) {
				if( filterName( this_kid->getName()) )
					new_part->addLeaf( tri_idx, ind_idx, (ssgLeaf *) this_kid );
			}
		}
		new_part->SetConnectivity();
		parts.push_back( new_part );
	}
}

void SGShadowVolume::SceneryObject::find_trans(void) {
	ssgBranch *branch = pending_object;
	ssgBranch *prev_branch = pending_object;
	// check the existence of the root node
	while( branch && branch->getNumParents() > 0 ) {
		prev_branch = branch;
		branch = branch->getParent(0);
	}
	// check if we are connected to the scene graph
	if( !branch->isA(ssgTypeRoot() ) )
		return;
	scenery_object = pending_object;
}

SGShadowVolume::SceneryObject::SceneryObject(ssgBranch *_scenery_object, OccluderType _occluder_type) :
	pending_object ( _scenery_object ),
	occluder_type ( _occluder_type ),
	scenery_object ( 0 )
{
	// queue objects, don't do all the work in the first frames because of
	// massive cpu power needed
	statObj++;
	if( occluder_type == SGShadowVolume::occluderTypeAircraft )
		lib_object = _scenery_object;
	else
		lib_object = (ssgBranch *) ((ssgBranch *)_scenery_object->getKid(0))->getKid(0);
}

SGShadowVolume::SceneryObject::~SceneryObject()
{
	ShadowCaster_list::iterator iParts;
	for(iParts = parts.begin() ; iParts != parts.end(); iParts++ ) {
		delete *iParts;
	}
	parts.clear();
}

void SGShadowVolume::computeShadows(void) {

	// intensity = ambient + lights
	// lights = sun_const * (sun_angle . normal)
	double dot_light = cos(sun_angle);
	// do nothing if sun is under horizon
	if( dot_light < 0.2 )
		return;

	//Draw shadow volumes
	glPushAttrib(GL_ALL_ATTRIB_BITS);
 	glPushClientAttrib ( GL_CLIENT_VERTEX_ARRAY_BIT ) ;
	glDisableClientState ( GL_COLOR_ARRAY         ) ;
	glDisableClientState ( GL_NORMAL_ARRAY        ) ;
	glDisableClientState ( GL_TEXTURE_COORD_ARRAY ) ;

	if( use_alpha ) {
		glColorMask(0, 0, 0, 1);
 		glClearColor(0.0, 0.0, 0.0, 0.0 );
		glClear(GL_COLOR_BUFFER_BIT);
 		glDisable(GL_ALPHA);
		glEnable(GL_BLEND);
	} else {
		glClearStencil( 0 );
		glClear(GL_STENCIL_BUFFER_BIT);
		glColorMask(0, 0, 0, 0);
		glEnable(GL_STENCIL_TEST);
		glDisable(GL_ALPHA);
		glDisable(GL_BLEND);
	}
	glDisable( GL_LIGHTING );
	glDisable( GL_FOG );
  	glEnable( GL_CULL_FACE  );
//	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//    glPolygonOffset(0.0,5.0);
    glPolygonOffset(0.0,30.0);
    glEnable(GL_POLYGON_OFFSET_FILL);

 	glShadeModel(GL_FLAT);

	glDepthMask( false );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc(GL_LESS);

	SceneryObject_map::iterator iSceneryObject;
	// compute shadows for each objects
	for(iSceneryObject = sceneryObjects.begin() ; iSceneryObject != sceneryObjects.end(); iSceneryObject++ ) {
		SceneryObject *an_occluder = iSceneryObject->second;
		if( shadowsTO_enabled && (an_occluder->occluder_type == occluderTypeTileObject) ||
			shadowsAI_enabled && (an_occluder->occluder_type == occluderTypeAI ) ||
			shadowsAC_enabled && (an_occluder->occluder_type == occluderTypeAircraft ) )
				an_occluder->computeShadows();
	}


	glMatrixMode   ( GL_PROJECTION ) ;
	glPushMatrix   () ;
	glLoadIdentity () ;
	glOrtho        ( -100, 100, -100, 100, -1, 1 ) ;
	glMatrixMode   ( GL_MODELVIEW ) ;
	glPushMatrix   () ;
	glLoadIdentity () ;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
//	glBindTexture(GL_TEXTURE_2D, 0);
	glPolygonMode(GL_FRONT, GL_FILL);
	if( use_alpha ) {
		// there is a flaw in the Roettger paper, this does not work for some geometry
		// where we increment (multiply) the buffer a few times then we
		// decrement (divide) a few times. Decrementing more then once will give a
		// non shadowed parts with mask value < 0.25 because the incrementation was
		// clamped
		// Solution : don't use a start value as high as 0.25 but the smallest value
		// posible ie 1/256. This is not a general solution, a stencil buffer will
		// support 255 incrementation before clamping, the alpha mask only 7.
		// => that still does not work with our geometry so use subtractive blend

		// clamp mask = {0,16,32,64,...} => {0,16,16,16,...}
		glBlendEquation( GL_MIN_EXT );
   		glBlendFunc( GL_DST_COLOR, GL_ONE );
 		glColor4ub(1, 1, 1, 16);
 		glRectf(-100,-100,100,100);
		// scale mask = {0,16} => {0,64}
		glBlendEquation( GL_FUNC_ADD );
   		glBlendFunc( GL_DST_COLOR, GL_ONE );
 		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
 		glRectf(-100,-100,100,100);
		glRectf(-100,-100,100,100);
		// negate mask => {0,64} => {255, 191}
		glBlendFunc( GL_ONE_MINUS_DST_COLOR, GL_ZERO );
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
 		glRectf(-100,-100,100,100);
		// apply mask
		glColorMask(1, 1, 1, 1);
   		glBlendFunc( GL_ZERO, GL_DST_ALPHA );
   		glColor4f(1.0f, 0.5f, 0.2f, 1.0f);
		glRectf(-100,-100,100,100);
	} else {
		// now the stencil contains 0 for scenery in light and != 0 for parts in shadow
		// draw a quad covering the screen, the stencil will be the mask
		// we darken the shadowed parts
		glColorMask(1, 1, 1, 1);
		glStencilFunc(GL_NOTEQUAL, 0, ~0);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glEnable(GL_STENCIL_TEST);
		glEnable(GL_ALPHA);
		glAlphaFunc(GL_GREATER, 0.0f);
		glEnable(GL_BLEND);
		glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
  		glColor4f(0.0, 0.0, 0.0, sgLerp(0.1, 0.3, dot_light) );
		// fixed value is better, the previous line is surely wrong
  		glColor4f(0.0, 0.0, 0.0, 0.3 );
		glRectf(-100,-100,100,100);
	}
	glMatrixMode   ( GL_PROJECTION ) ;
	glPopMatrix    () ;
	glMatrixMode   ( GL_MODELVIEW ) ;
	glPopMatrix    () ;

	glDisable(GL_STENCIL_TEST);
	glPopClientAttrib ( ) ;
	glPopAttrib();
}

SGShadowVolume::SGShadowVolume() : 
	init_done( false ),
	shadows_enabled( false ),
	frameNumber( 0 ),
	lastTraverseTreeFrame ( 0 )
{
	states = this;
}

SGShadowVolume::~SGShadowVolume() {
	SceneryObject_map::iterator iSceneryObject;
	for(iSceneryObject = sceneryObjects.begin() ; iSceneryObject != sceneryObjects.end();  ) {
		delete iSceneryObject->second;
		iSceneryObject = sceneryObjects.erase( iSceneryObject );
	}
}

void SGShadowVolume::init(SGPropertyNode *sim_rendering_options) {
	init_done = true;
	shadows_enabled = true;
	sim_rendering = sim_rendering_options;
	int stencilBits = 0, alphaBits = 0;
	glGetIntegerv( GL_STENCIL_BITS, &stencilBits );
	glGetIntegerv( GL_ALPHA_BITS, &alphaBits );
	bool hasSubtractiveBlend = SGIsOpenGLExtensionSupported("GL_EXT_blend_subtract");
	bool hasMinMaxBlend = SGIsOpenGLExtensionSupported("GL_EXT_blend_minmax");
	if( hasSubtractiveBlend )
		glBlendEquation = (glBlendEquationProc ) SGLookupFunction("glBlendEquationEXT");
	canDoAlpha = (alphaBits >= 8) && hasSubtractiveBlend && hasMinMaxBlend;
	canDoStencil = (stencilBits >= 3);
	if( !canDoStencil )
		if( canDoAlpha )
			SG_LOG(SG_ALL, SG_WARN, "SGShadowVolume:no stencil buffer, using alpha buffer");
		else
			SG_LOG(SG_ALL, SG_WARN, "SGShadowVolume:no stencil buffer and no alpha buffer");
}

void SGShadowVolume::startOfFrame(void) {
}
void SGShadowVolume::deleteOccluderFromTile(ssgBranch *tile) {
	SceneryObject_map::iterator iSceneryObject;
	for(iSceneryObject = sceneryObjects.begin() ; iSceneryObject != sceneryObjects.end();  ) {
		if( iSceneryObject->second->tile == tile ) {
			delete iSceneryObject->second;
			iSceneryObject = sceneryObjects.erase( iSceneryObject );
		}
		else 
			iSceneryObject++;
	}
}

void SGShadowVolume::deleteOccluder(ssgBranch *occluder) {
	ssgBranch *branch = occluder;
	// skip first node and go to first transform (placement)
	while( occluder && !occluder->isA(ssgTypeTransform()))
		occluder = (ssgBranch *) occluder->getKid(0);

	// check if we allready know this object
	SceneryObject_map::iterator iSceneryObject = sceneryObjects.find( occluder );
	if( iSceneryObject != sceneryObjects.end() ) {
		delete iSceneryObject->second;
		sceneryObjects.erase( occluder );
	}
}

void SGShadowVolume::addOccluder(ssgBranch *occluder, OccluderType occluder_type, ssgBranch *tile) {

	ssgBranch *branch = occluder;

	// skip first node and go to first transform (placement)
	while( occluder && !occluder->isA(ssgTypeTransform()))
		occluder = (ssgBranch *) occluder->getKid(0);

	// check if we allready know this object
	SceneryObject_map::iterator iSceneryObject = sceneryObjects.find( occluder );
	if( iSceneryObject == sceneryObjects.end() ) {
//		printf("adding model %x %x\n", occluder, tile);
		SceneryObject *entry = new SceneryObject( occluder, occluder_type );
		entry->tile = tile;
		sceneryObjects[ occluder ] = entry;
	}

}

void SGShadowVolume::setupShadows( double lon, double lat,
		double gst, double SunRightAscension, double SunDeclination, double sunAngle) {

	shadowsAC_enabled = sim_rendering->getBoolValue("shadows-ac", false);
	shadowsAI_enabled = sim_rendering->getBoolValue("shadows-ai", false);
	shadowsTO_enabled = sim_rendering->getBoolValue("shadows-to", false);
	shadowsDebug_enabled = sim_rendering->getBoolValue("shadows-debug", false);
//	shadows_enabled   = sim_rendering->getBoolValue("shadows", false);
	shadows_enabled = shadowsAC_enabled || shadowsAI_enabled || shadowsTO_enabled;
	shadows_enabled &= canDoAlpha || canDoStencil;
	use_alpha = ((!canDoStencil) || sim_rendering->getBoolValue("shadows-alpha", false)) &&
		canDoAlpha;

	if( ! shadows_enabled )
		return;

	sgMat4 view_angle;
	sun_angle = sunAngle;
	{
		sgMat4 LON, LAT;
		sgVec3 axis;

		sgSetVec3( axis, 0.0, 0.0, 1.0 );
		sgMakeRotMat4( LON, lon, axis );

		sgSetVec3( axis, 0.0, 1.0, 0.0 );
		sgMakeRotMat4( LAT, 90.0 - lat, axis );

		sgMat4 TRANSFORM;

		sgMakeIdentMat4 ( TRANSFORM );
		sgPreMultMat4( TRANSFORM, LON );
		sgPreMultMat4( TRANSFORM, LAT );

		sgCoord pos;
		sgSetCoord( &pos, TRANSFORM );

 		sgMakeCoordMat4( view_angle, &pos );
	}
	{
	sgMat4 GST, RA, DEC;
	sgVec3 axis;


	sgSetVec3( axis, 0.0, 0.0, -1.0 );
	sgMakeRotMat4( GST, (gst) * 15.0, axis );

	sgSetVec3( axis, 0.0, 0.0, 1.0 );
	sgMakeRotMat4( RA, (SunRightAscension * SGD_RADIANS_TO_DEGREES) - 90.0, axis );

	sgSetVec3( axis, 1.0, 0.0, 0.0 );
	sgMakeRotMat4( DEC, SunDeclination * SGD_RADIANS_TO_DEGREES, axis );

	sgInvertMat4( invViewAngle, view_angle); 

	sgMat4 TRANSFORM;
	sgMakeIdentMat4( TRANSFORM );
	sgPreMultMat4( TRANSFORM, GST );
	sgPreMultMat4( TRANSFORM, RA );
	sgPreMultMat4( TRANSFORM, DEC );
   	sgSetVec3( sunPos, 0.0, 9900000.0, 0.0);
	sgXformPnt3( sunPos, TRANSFORM );
	}

	ssgGetModelviewMatrix( CameraViewM );

}

void SGShadowVolume::endOfFrame(void) {
	if( ! shadows_enabled )
		return;
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_1D, 0);

	glMatrixMode(GL_MODELVIEW);

	computeShadows();

	frameNumber ++;
}

