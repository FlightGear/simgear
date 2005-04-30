// a layer of 3d clouds
//
// Written by Harald JOHNSEN, started April 2005.
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/math/sg_random.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/polar3d.hxx>

#include STL_ALGORITHM
#include <vector>

SG_USING_STD(vector);

#include <simgear/environment/visual_enviro.hxx>
#include "newcloud.hxx"
#include "cloudfield.hxx"

static list_of_culledCloud inViewClouds;

// visibility distance for clouds in meters
float SGCloudField::CloudVis = 25000.0f;
bool SGCloudField::enable3D = true;
// fieldSize must be > CloudVis or we can destroy the impostor cache
// a cloud must only be seen once or the impostor will be generated for each of his positions
double SGCloudField::fieldSize = 27000.0;
float SGCloudField::density = 100.0;
static int last_cache_size = 1*1024;
static int cacheResolution = 64;

int SGCloudField::get_CacheResolution(void) {
	return cacheResolution;
}

void SGCloudField::set_CacheResolution(int resolutionPixels) {
	if(cacheResolution == resolutionPixels)
		return;
	cacheResolution = resolutionPixels;
	if(enable3D) {
		int count = last_cache_size * 1024 / (cacheResolution * cacheResolution * 4);
		if(count == 0)
			count = 1;
		SGNewCloud::cldCache->setCacheSize(count, cacheResolution);
	}
}

int SGCloudField::get_CacheSize(void) { 
	return SGNewCloud::cldCache->queryCacheSize(); 
}

void SGCloudField::set_CacheSize(int sizeKb) {
	// apply in rendering option dialog
	if(last_cache_size == sizeKb)
		return;
	if(sizeKb == 0)
		return;
	if(sizeKb)
		last_cache_size = sizeKb;
	if(enable3D) {
		int count = last_cache_size * 1024 / (cacheResolution * cacheResolution * 4);
		if(count == 0)
			count = 1;
//		SGNewCloud::cldCache->setCacheSize(sizeKb);
		SGNewCloud::cldCache->setCacheSize(count, cacheResolution);
	}
}
void SGCloudField::set_CloudVis(float distance) {
	if( distance <= fieldSize )
		SGCloudField::CloudVis = distance;
}
void SGCloudField::set_density(float density) {
	SGCloudField::density = density;
}
void SGCloudField::set_enable3dClouds(bool enable) {
	if(enable3D == enable)
		return;
	enable3D = enable;
	if(enable) {
		SGNewCloud::cldCache->setCacheSize(last_cache_size);
	} else {
		SGNewCloud::cldCache->setCacheSize(0);
	}
}

// reposition the cloud layer at the specified origin and orientation
void SGCloudField::reposition( sgVec3 p, sgVec3 up, double lon, double lat, double alt, double dt) {
    sgMat4 T1, LON, LAT;
    sgVec3 axis;

    sgMakeTransMat4( T1, p );

    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( LON, lon * SGD_RADIANS_TO_DEGREES, axis );

    sgSetVec3( axis, 0.0, 1.0, 0.0 );
    sgMakeRotMat4( LAT, 90.0 - lat * SGD_RADIANS_TO_DEGREES, axis );

    sgMat4 TRANSFORM;

    sgCopyMat4( TRANSFORM, T1 );
    sgPreMultMat4( TRANSFORM, LON );
    sgPreMultMat4( TRANSFORM, LAT );

    sgCoord layerpos;
    sgSetCoord( &layerpos, TRANSFORM );

	sgMakeCoordMat4( transform, &layerpos );


	this->alt = alt;

	// simulate clouds movement from wind
	double speed = 10.0f;
	double direction = 45.0;
    double sp_dist = speed*dt;
    if (sp_dist > 0) {
        double bx = cos((180.0-direction) * SGD_DEGREES_TO_RADIANS) * sp_dist;
        double by = sin((180.0-direction) * SGD_DEGREES_TO_RADIANS) * sp_dist;
		relative_position[SG_X] += bx;
		relative_position[SG_Y] += by;
    }

    if ( lon != last_lon || lat != last_lat || sp_dist != 0 ) {
        Point3D start( last_lon, last_lat, 0.0 );
        Point3D dest( lon, lat, 0.0 );
        double course = 0.0, dist = 0.0;

        calc_gc_course_dist( dest, start, &course, &dist );
        // if start and dest are too close together,
        // calc_gc_course_dist() can return a course of "nan".  If
        // this happens, lets just use the last known good course.
        // This is a hack, and it would probably be better to make
        // calc_gc_course_dist() more robust.
        if ( isnan(course) ) {
            course = last_course;
        } else {
            last_course = course;
        }

        // calculate cloud movement due to external forces
        double ax = 0.0, ay = 0.0;

        if (dist > 0.0) {
            ax = cos(course) * dist;
            ay = sin(course) * dist;
        }

		deltax += ax;
		deltay += ay;

        last_lon = lon;
        last_lat = lat;
    }


	// correct the frustum with the right far plane
	ssgContext *context = ssgGetCurrentContext();
	frustum = *context->getFrustum();
	frustum.setFOV(55.0,0);
	frustum.setNearFar(1.0, CloudVis);
}

SGCloudField::SGCloudField() :
	last_density(0.0),
	deltax(0.0),
	deltay(0.0),
	last_course(0.0)
{
	sgSetVec3( relative_position, 0,0,0);
	theField.reserve(200);
	inViewClouds.reserve(200);
        sg_srandom_time_10();
}

SGCloudField::~SGCloudField() {
	list_of_Cloud::iterator iCloud;
	for( iCloud = theField.begin() ; iCloud != theField.end() ; iCloud++ ) {
		delete iCloud->aCloud;
	}
	theField.clear();
}


// use a table or else we see poping when moving the slider...
static int densTable[][10] = {
	{0,0,0,0,0,0,0,0,0,0},
	{1,0,0,0,0,0,0,0,0,0},
	{1,0,0,0,1,0,0,0,0,0},
	{1,0,0,0,1,0,0,1,0,0}, // 30%
	{1,0,1,0,1,0,0,1,0,0},
	{1,0,1,0,1,0,1,1,0,0}, // 50%
	{1,0,1,0,1,0,1,1,0,1},
	{1,0,1,1,1,0,1,1,0,1}, // 70%
	{1,1,1,1,1,0,1,1,0,1},
	{1,1,1,1,1,0,1,1,1,1}, // 90%
	{1,1,1,1,1,1,1,1,1,1}
};

// set the visible flag depending on density
void SGCloudField::applyDensity(void) {
	int row = (int) (density / 10.0);
	int col = 0;
	list_of_Cloud::iterator iCloud;
	for( iCloud = theField.begin() ; iCloud != theField.end() ; iCloud++ ) {
		if(++col > 9)
			col = 0;
		if( densTable[row][col] ) {
			iCloud->visible = true;
		} else
			iCloud->visible = false;
	}
	last_density = density;
}

// add one cloud, data is not copied, ownership given
void SGCloudField::addCloud( sgVec3 pos, SGNewCloud *cloud) {
	Cloud cl;
	sgCopyVec3( cl.pos, pos );
	cl.aCloud = cloud;
	cl.visible = true;
	cloud->SetPos( pos );
	theField.push_back( cl );
}


static float Rnd(float n) {
	return n * (-0.5f + sg_random());
}

// for debug only
// build a field of cloud of size 25x25 km, its a grid of 11x11 clouds
void SGCloudField::buildTestLayer(void) {

	const float s = 2250.0f;

	for( int z = -5 ; z <= 5 ; z++) {
		for( int x = -5 ; x <= 5 ; x++ ) {
            SGNewCloud *cloud = new SGNewCloud;
			cloud->new_cu();
			sgVec3 pos = {(x+Rnd(0.7)) * s, 750.0f, (z+Rnd(0.7)) * s};
            addCloud(pos, cloud);
		}
	}
	applyDensity();
}

// cull all clouds of a tiled field
void SGCloudField::cullClouds(sgVec3 eyePos, sgMat4 mat) {
	list_of_Cloud::iterator iCloud;

	// TODO:cull the field before culling the clouds in the field (should eliminate 3 fields)
	for( iCloud = theField.begin() ; iCloud != theField.end() ; iCloud++ ) {
		sgVec3 dist;
		sgSphere sphere;
		if( ! iCloud->visible )
			continue;
		sgSubVec3( dist, iCloud->pos, eyePos );
		sphere.setCenter(dist[0], dist[2], dist[1]);
		float radius = iCloud->aCloud->getRadius();
		sphere.setRadius(radius);
		sphere.orthoXform(mat);
		if( frustum.contains( & sphere ) != SG_OUTSIDE ) {
			float squareDist = dist[0]*dist[0] + dist[1]*dist[1] + dist[2]*dist[2];
			culledCloud tmp;
			tmp.aCloud = iCloud->aCloud;
			sgCopyVec3( tmp.eyePos, eyePos ); 
			// save distance for later sort, opposite distance because we want back to front
			tmp.dist   =  - squareDist;
			inViewClouds.push_back(tmp);
			if( squareDist - radius*radius < 100*100 )
				sgEnviro.set_view_in_cloud(true);
		}
	}

}


// Render a cloud field
// because no field can have an infinite size (and we don't want to reach his border)
// we draw this field and adjacent fields.
// adjacent fields are not real, its the same field displaced by some offset
void SGCloudField::Render(void) {
    sgVec3 eyePos;
	double relx, rely;

	if( ! enable3D )
		return;

	if( last_density != density ) {
		last_density = density;
		applyDensity();
	}

	// ask the impostor cache to do some cleanup
	// TODO:don't do that for every field
	SGNewCloud::cldCache->startNewFrame();

	inViewClouds.clear();

 
	glPushMatrix();
 
	sgMat4 modelview, tmp, invtrans;

	// try to find the sun position
	sgTransposeNegateMat4( invtrans, transform );
    sgVec3 lightVec;
    ssgGetLight( 0 )->getPosition( lightVec );
    sgXformVec3( lightVec, invtrans );
	sgCopyVec3( SGNewCloud::modelSunDir, lightVec );
	sgSetVec3(  SGNewCloud::modelSunDir, lightVec[0], lightVec[2], lightVec[1]);
	// try to find the lighting data (buggy)
	sgVec4 diffuse, ambient;
	ssgGetLight( 0 )->getColour( GL_DIFFUSE, diffuse );
	ssgGetLight( 0 )->getColour( GL_AMBIENT, ambient );
	sgScaleVec3 ( SGNewCloud::sunlight, diffuse , 0.70f);
	sgScaleVec3 ( SGNewCloud::ambLight, ambient , 0.60f);

	// voodoo things on the matrix stack
    ssgGetModelviewMatrix( modelview );
	sgCopyMat4( tmp, transform );
    sgPostMultMat4( tmp, modelview );

	// cloud fields are tiled on the flat earth
	// compute the position in the tile
	relx = -fmod( deltax + relative_position[SG_X] + tmp[3][0], fieldSize );
	rely = -fmod( deltay + relative_position[SG_Y] + tmp[3][1], fieldSize );

	sgSetVec3( eyePos, -relx, alt, -rely);

	tmp[3][2] = 0;
	tmp[3][0] = 0;
	tmp[3][1] = 0;
    ssgLoadModelviewMatrix( tmp );
 
/* flat earth

	^
	|
	|    FFF
	|    FoF
	|    FFF
	|
	O----------->
		o = we are here
		F = adjacent fields
*/

	for(int x = -1 ; x <= 1 ; x++)
		for(int y = -1 ; y <= 1 ; y++ ) {
			sgVec3 fieldPos;
			// pretend we are not where we are
			sgSetVec3(fieldPos, eyePos[0] + x*fieldSize, eyePos[1], eyePos[2] + y*fieldSize);
			cullClouds(fieldPos, tmp);
		}
	// sort all visible clouds back to front (because of transparency)
	std::sort( inViewClouds.begin(), inViewClouds.end() );
 
	// TODO:push states
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_SMOOTH);
    glEnable(GL_BLEND);
	glBlendFunc( GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_FOG );
    glDisable(GL_LIGHTING);

	// test data: field = 11x11 cloud, 9 fields
	// depending on position and view direction, perhaps 40 to 60 clouds not culled
	list_of_culledCloud::iterator iCloud;
	for( iCloud = inViewClouds.begin() ; iCloud != inViewClouds.end() ; iCloud++ ) {
//		iCloud->aCloud->drawContainers();
		iCloud->aCloud->Render(iCloud->eyePos);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
	glEnable( GL_FOG );
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	ssgLoadModelviewMatrix( modelview );

	glPopMatrix();

}

