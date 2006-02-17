// Visual environment helper class
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

#include <plib/sg.h>
#include <simgear/constants.h>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/scene/sky/cloudfield.hxx>
#include <simgear/scene/sky/newcloud.hxx>
#include "visual_enviro.hxx"

#include <vector>

SG_USING_STD(vector);


typedef struct {
	Point3D		pt;
	int			depth;
	int			prev;
} lt_tree_seg;

#define MAX_RAIN_SLICE	200
static float rainpos[MAX_RAIN_SLICE];
#define MAX_LT_TREE_SEG	400

/**
 * A class to render lightnings.
 */
class SGLightning {
public:
    /**
     * Build a new lightning.
     * The lightning has a limited life time. It will also play a thunder sounder once.
     * @param lon lon longitude in degree
     * @param lat lat latitude in degree
     * @param alt asl of top of lightning
     */
	SGLightning(double lon, double lat, double alt);
	~SGLightning();
	void lt_Render(void);
	void lt_build(void);
	void lt_build_tree_branch(int tree_nr, Point3D &start, float energy, int nbseg, float segsize);

	// contains all the segments of the lightning
	lt_tree_seg lt_tree[MAX_LT_TREE_SEG];
	// segment count
	int		nb_tree;
	// position of lightning
	double	lon, lat, alt;
	int		sequence_count;
	// time to live
	double	age;
};

typedef vector<SGLightning *> list_of_lightning;
static list_of_lightning lightnings;

SGEnviro sgEnviro;

SGEnviro::SGEnviro(void) :
	view_in_cloud(false),
	precipitation_enable_state(true),
	precipitation_density(100.0),
	precipitation_max_alt(0.0),
	turbulence_enable_state(false),
	last_cloud_turbulence(0.0),
	cloud_turbulence(0.0),
	lightning_enable_state(false),
	elapsed_time(0.0),
	dt(0.0),
	soundMgr(NULL),
	snd_active(false),
	snd_dist(0.0),
	min_time_before_lt(0.0),
	fov_width(55.0),
	fov_height(55.0)

{
	for(int i = 0; i < MAX_RAIN_SLICE ; i++)
		rainpos[i] = sg_random();
	radarEcho.reserve(100);
}

SGEnviro::~SGEnviro(void) {
	list_of_lightning::iterator iLightning;
	for( iLightning = lightnings.begin() ; iLightning != lightnings.end() ; iLightning++ ) {
		delete (*iLightning);
	}
	lightnings.clear();
}

void SGEnviro::startOfFrame( sgVec3 p, sgVec3 up, double lon, double lat, double alt, double delta_time) {
	view_in_cloud = false;
	// ask the impostor cache to do some cleanup
	if(SGNewCloud::cldCache)
		SGNewCloud::cldCache->startNewFrame();
	last_cloud_turbulence = cloud_turbulence;
	cloud_turbulence = 0.0;
	elapsed_time += delta_time;
	min_time_before_lt -= delta_time;
	dt = delta_time;

	sgMat4 T1, LON, LAT;
    sgVec3 axis;

    sgMakeTransMat4( T1, p );

    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( LON, lon, axis );

    sgSetVec3( axis, 0.0, 1.0, 0.0 );
    sgMakeRotMat4( LAT, 90.0 - lat, axis );

    sgMat4 TRANSFORM;

    sgCopyMat4( TRANSFORM, T1 );
    sgPreMultMat4( TRANSFORM, LON );
    sgPreMultMat4( TRANSFORM, LAT );

    sgCoord pos;
    sgSetCoord( &pos, TRANSFORM );

	sgMakeCoordMat4( transform, &pos );
    last_lon = lon;
    last_lat = lat;
	last_alt = alt;

	radarEcho.clear();
	precipitation_max_alt = 400.0;
}

void SGEnviro::endOfFrame(void) {
}

double SGEnviro::get_cloud_turbulence(void) const {
	return last_cloud_turbulence;
}

// this can be queried to add some turbulence for example
bool SGEnviro::is_view_in_cloud(void) const {
	return view_in_cloud;
}
void SGEnviro::set_view_in_cloud(bool incloud) {
	view_in_cloud = incloud;
}

int SGEnviro::get_CacheResolution(void) const {
	return SGCloudField::get_CacheResolution();
}

int SGEnviro::get_clouds_CacheSize(void) const {
	return SGCloudField::get_CacheSize();
}
float SGEnviro::get_clouds_visibility(void) const {
	return SGCloudField::get_CloudVis();
}
float SGEnviro::get_clouds_density(void) const {
	return SGCloudField::get_density();
}
bool SGEnviro::get_clouds_enable_state(void) const {
	return SGCloudField::get_enable3dClouds();
}

bool SGEnviro::get_turbulence_enable_state(void) const {
	return turbulence_enable_state;
}

void SGEnviro::set_CacheResolution(int resolutionPixels) {
	SGCloudField::set_CacheResolution(resolutionPixels);
}

void SGEnviro::set_clouds_CacheSize(int sizeKb) {
	SGCloudField::set_CacheSize(sizeKb);
}
void SGEnviro::set_clouds_visibility(float distance) {
	SGCloudField::set_CloudVis(distance);
}
void SGEnviro::set_clouds_density(float density) {
	SGCloudField::set_density(density);
}
void SGEnviro::set_clouds_enable_state(bool enable) {
	SGCloudField::set_enable3dClouds(enable);
}
void SGEnviro::set_turbulence_enable_state(bool enable) {
	turbulence_enable_state = enable;
}
// rain/snow
float SGEnviro::get_precipitation_density(void) const {
	return precipitation_density;
}
bool SGEnviro::get_precipitation_enable_state(void) const {
	return precipitation_enable_state;
}

void SGEnviro::set_precipitation_density(float density) {
	precipitation_density = density;
}
void SGEnviro::set_precipitation_enable_state(bool enable) {
	precipitation_enable_state = enable;
}

// others
bool SGEnviro::get_lightning_enable_state(void) const {
	return lightning_enable_state;
}

void SGEnviro::set_lightning_enable_state(bool enable) {
	lightning_enable_state = enable;
	if( ! enable ) {
		// TODO:cleanup
	}
}

void SGEnviro::setLight(sgVec4 adj_fog_color) {
	sgCopyVec4( fog_color, adj_fog_color );
	if( false ) {
	//    ssgGetLight( 0 ) -> setColour( GL_DIFFUSE, l->scene_diffuse() );
	}
}

void SGEnviro::callback_cloud(float heading, float alt, float radius, int familly, float dist, int cloudId) {
	// send data to wx radar
	// compute turbulence
	// draw precipitation
	// draw lightning
	// compute illumination

	// http://www.pilotfriend.com/flight_training/weather/THUNDERSTORM%20HAZARDS1.htm
	double turbulence = 0.0;
	if( dist < radius * radius * 2.25f ) {
		switch(familly) {
			case SGNewCloud::CLFamilly_st:
				turbulence = 0.2;
				break;
			case SGNewCloud::CLFamilly_ci:
			case SGNewCloud::CLFamilly_cs:
			case SGNewCloud::CLFamilly_cc:
			case SGNewCloud::CLFamilly_ac:
			case SGNewCloud::CLFamilly_as:
				turbulence = 0.1;
				break;
			case SGNewCloud::CLFamilly_sc:
				turbulence = 0.3;
				break;
			case SGNewCloud::CLFamilly_ns:
				turbulence = 0.4;
				break;
			case SGNewCloud::CLFamilly_cu:
				turbulence = 0.5;
				break;
			case SGNewCloud::CLFamilly_cb:
				turbulence = 0.6;
				break;
		}
		// full turbulence inside cloud, half in the vicinity
		if( dist > radius * radius )
			turbulence *= 0.5;
		if( turbulence > cloud_turbulence )
			cloud_turbulence = turbulence;
		// we can do 'local' precipitations too
	}

	// convert to LWC for radar (experimental)
	// http://www-das.uwyo.edu/~geerts/cwx/notes/chap08/moist_cloud.html
	double LWC = 0.0;
	switch(familly) {
		case SGNewCloud::CLFamilly_st:
			LWC = 0.29;
			break;
		case SGNewCloud::CLFamilly_cu:
			LWC = 0.27;
			break;
		case SGNewCloud::CLFamilly_cb:
			LWC = 2.0;
			break;
		case SGNewCloud::CLFamilly_sc:
			LWC = 0.44;
			break;
		case SGNewCloud::CLFamilly_ci:
			LWC = 0.03;
			break;
		// no data
		case SGNewCloud::CLFamilly_cs:
		case SGNewCloud::CLFamilly_cc:
		case SGNewCloud::CLFamilly_ac:
		case SGNewCloud::CLFamilly_as:
			LWC = 0.03;
			break;
		case SGNewCloud::CLFamilly_ns:
			LWC = 0.29*2.0;
			break;
	}
	// add to the list for the wxRadar instrument
	if( LWC > 0.0 )
		radarEcho.push_back( SGWxRadarEcho ( heading, alt, radius, dist, LWC, false, cloudId ) );

	// NB:data valid only from cockpit view

	// spawn a new lightning
	if(lightning_enable_state && min_time_before_lt <= 0.0 && (familly == SGNewCloud::CLFamilly_cb) &&
		dist < 15000.0 * 15000.0 && sg_random() > 0.9f) {
		double lat, lon;
		Point3D orig, dest;
		orig.setlat(last_lat * SG_DEGREES_TO_RADIANS );
		orig.setlon(last_lon * SG_DEGREES_TO_RADIANS );
		orig.setelev(0.0);
		dist = sgSqrt(dist);
		dest = calc_gc_lon_lat(orig, heading, dist);
		lon = dest.lon() * SG_RADIANS_TO_DEGREES;
		lat = dest.lat() * SG_RADIANS_TO_DEGREES;
		addLightning( lon, lat, alt );

		// reset timer
		min_time_before_lt = 5.0 + sg_random() * 30;
		// DEBUG only
//		min_time_before_lt = 5.0;
	}
	if( (alt - radius * 0.1) > precipitation_max_alt )
		switch(familly) {
			case SGNewCloud::CLFamilly_st:
			case SGNewCloud::CLFamilly_cu:
			case SGNewCloud::CLFamilly_cb:
			case SGNewCloud::CLFamilly_ns:
			case SGNewCloud::CLFamilly_sc:
				precipitation_max_alt = alt - radius * 0.1;
				break;
		}
}

list_of_SGWxRadarEcho *SGEnviro::get_radar_echo(void) {
	return &radarEcho;
}

// precipitation rendering code
void SGEnviro::DrawCone2(float baseRadius, float height, int slices, bool down, double rain_norm, double speed) {

	sgVec3 light;
	sgVec3 min_light = {0.35, 0.35, 0.35};
	sgAddVec3( light, fog_color, min_light );
	float da = SG_PI * 2.0f / (float) slices;
	// low number = faster
	float speedf = 2.5f - speed / 200.0;
	if( speedf < 1.0f )
		speedf = 1.0f;
	float lenf = 0.03f + speed / 2000.0;
	if( lenf > 0.10f )
		lenf = 0.10f;
    float t = fmod((float) elapsed_time, speedf) / speedf;
//	t = 0.1f;
	if( !down )
		t = 1.0f - t;
	float angle = 0.0f;
	glColor4f(1.0f, 0.7f, 0.7f, 0.9f);
	glBegin(GL_LINES);
	int rainpos_indice = 0;
	for( int i = 0 ; i < slices ; i++ ) {
		float x = cos(angle) * baseRadius;
		float y = sin(angle) * baseRadius;
		angle += da;
		sgVec3 dir = {x, -height, y};

		// rain drops at 2 different speed to simulate depth
		float t1 = (i & 1 ? t : t + t) + rainpos[rainpos_indice];
		if(t1 > 1.0f)	t1 -= 1.0f;
		if(t1 > 1.0f)	t1 -= 1.0f;

		// distant raindrops are more transparent
		float c = (i & 1 ? t1 * 0.5f : t1 * 0.9f);
		glColor4f(c * light[0], c * light[1], c * light[2], c);
		sgVec3 p1, p2;
		sgScaleVec3(p1, dir, t1);
		// distant raindrops are shorter
		float t2 = t1 + (i & 1 ? lenf : lenf+lenf);
		sgScaleVec3(p2, dir, t2);

		glVertex3f(p1[0], p1[1] + height, p1[2]);
		glVertex3f(p2[0], p2[1] + height, p2[2]);
		if( ++rainpos_indice >= MAX_RAIN_SLICE )
			rainpos_indice = 0;
	}
	glEnd();
}

void SGEnviro::drawRain(double pitch, double roll, double heading, double speed, double rain_norm) {

	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_FOG );
	glDisable(GL_LIGHTING);

	int slice_count = static_cast<int>(
				(40.0 + rain_norm*150.0)* precipitation_density / 100.0);

	// www.wonderquest.com/falling-raindrops.htm says that
	// Raindrop terminal velocity is 5 to 20mph
	// Rather than model it accurately (temp, pressure, diameter), and make it
	// smaller than terminal when closer to the precipitation cloud base,
	// we interpolate in the 5-20mph range according to rain_norm.
	double raindrop_speed_kts 
		= (5.0 + rain_norm*15.0) * SG_MPH_TO_MPS * SG_MPS_TO_KT;

	float angle = atanf(speed / raindrop_speed_kts) * SG_RADIANS_TO_DEGREES;
	// We assume that the speed is HORIZONTAL airspeed here!!! XXX
	// DOCUMENT THE CHANGE IN THE PARAMETER speed MEANING BY RENAMING IT!!!
	glPushMatrix();
		// the cone rotate with speed
		angle = -pitch - angle;
		glRotatef(heading, 0.0, 1.0, 0.0);
		glRotatef(roll, 0.0, 0.0, 1.0);
		glRotatef(angle, 1.0, 0.0, 0.0);

		// up cone
		DrawCone2(15.0, 30.0, slice_count, true, rain_norm, speed);
		// down cone (usually not visible)
		if(angle > 0.0 || heading != 0.0)
			DrawCone2(15.0, -30.0, slice_count, false, rain_norm, speed);

	glPopMatrix();

	glEnable(GL_LIGHTING);
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
	glEnable( GL_FOG );
	glEnable(GL_DEPTH_TEST);

}

void SGEnviro::set_soundMgr(SGSoundMgr *mgr) {
	soundMgr = mgr;
}

void SGEnviro::drawPrecipitation(double rain_norm, double snow_norm, double hail_norm, double pitch, double roll, double heading, double speed) {
	if( precipitation_enable_state && rain_norm > 0.0)
	  if( precipitation_max_alt >= last_alt )
		drawRain(pitch, roll, heading, speed, rain_norm);
}


SGLightning::SGLightning(double _lon, double _lat, double _alt) :
	nb_tree(0),
	lon(_lon),
	lat(_lat),
	alt(_alt),
	age(1.0 + sg_random() * 4.0)
{
//	sequence_count = 1 + sg_random() * 5.0;
	lt_build();
}

SGLightning::~SGLightning() {
}

// lightning rendering code
void SGLightning::lt_build_tree_branch(int tree_nr, Point3D &start, float energy, int nbseg, float segsize) {

	sgVec3 dir, newdir;
	int nseg = 0;
	Point3D pt = start;
	if( nbseg == 50 )
		sgSetVec3( dir, 0.0, -1.0, 0.0 );
	else {
		sgSetVec3( dir, sg_random() - 0.5f, sg_random() - 0.5f, sg_random() - 0.5f);
		sgNormaliseVec3(dir);
	}
	if( nb_tree >= MAX_LT_TREE_SEG )
		return;

	lt_tree[nb_tree].depth = tree_nr;
	nseg = 0;
	lt_tree[nb_tree].pt = pt;
	lt_tree[nb_tree].prev = -1;
	nb_tree ++;

	// TODO:check agl
	while(nseg < nbseg && pt.y() > 0.0) {
        int prev = nb_tree - 1;
        nseg++;
		// add a branch
        if( energy * sg_random() > 0.8f )
			lt_build_tree_branch(tree_nr + 1, pt, energy * 0.9f, nbseg == 50 ? 10 : static_cast<int>(nbseg * 0.4f), segsize * 0.7f);

		if( nb_tree >= MAX_LT_TREE_SEG )
			return;
		sgSetVec3(newdir, (sg_random() - 0.5f), (sg_random() - 0.5f) - (nbseg == 50 ? 0.5f : 0.0), (sg_random() - 0.5f));
		sgNormaliseVec3(newdir);
		sgAddVec3( dir, newdir);
		sgNormaliseVec3(dir);
		sgVec3 scaleDir;
		sgScaleVec3( scaleDir, dir, segsize * energy * 0.5f );
		pt[PX] += scaleDir[0];
		pt[PY] += scaleDir[1];
		pt[PZ] += scaleDir[2];

		lt_tree[nb_tree].depth = tree_nr;
		lt_tree[nb_tree].pt = pt;
		lt_tree[nb_tree].prev = prev;
		nb_tree ++;
	}
}

void SGLightning::lt_build(void) {
    Point3D top;
    nb_tree = 0;
    top[PX] = 0 ;
    top[PY] = alt;
    top[PZ] = 0;
    lt_build_tree_branch(0, top, 1.0, 50, top[PY] / 8.0);
	if( ! sgEnviro.soundMgr )
		return;
	Point3D start( sgEnviro.last_lon*SG_DEGREES_TO_RADIANS, sgEnviro.last_lat*SG_DEGREES_TO_RADIANS, 0.0 );
	Point3D dest( lon*SG_DEGREES_TO_RADIANS, lat*SG_DEGREES_TO_RADIANS, 0.0 );
	double course = 0.0, dist = 0.0;
	calc_gc_course_dist( dest, start, &course, &dist );
	if( dist < 10000.0 && ! sgEnviro.snd_playing && (dist < sgEnviro.snd_dist || ! sgEnviro.snd_active) ) {
		sgEnviro.snd_timer = 0.0;
		sgEnviro.snd_wait  = dist / 340;
		sgEnviro.snd_dist  = dist;
		sgEnviro.snd_pos_lat = lat;
		sgEnviro.snd_pos_lon = lon;
		sgEnviro.snd_active = true;
		sgEnviro.snd_playing = false;
	}
}


void SGLightning::lt_Render(void) {
	float flash = 0.5;
	if( fmod(sgEnviro.elapsed_time*100.0, 100.0) > 50.0 )
		flash = sg_random() * 0.75f + 0.25f;
    float h = lt_tree[0].pt[PY];
	sgVec4 col={0.62f, 0.83f, 1.0f, 1.0f};
	sgVec4 c;

#define DRAW_SEG() \
			{glColorMaterial(GL_FRONT, GL_EMISSION);  \
			glDisable(GL_LINE_SMOOTH); glBegin(GL_LINES); \
				glColor4fv(c); \
                glVertex3f(lt_tree[n].pt[PX], lt_tree[n].pt[PZ], lt_tree[n].pt[PY]); \
                glVertex3f(lt_tree[lt_tree[n].prev].pt[PX], lt_tree[lt_tree[n].prev].pt[PZ], lt_tree[lt_tree[n].prev].pt[PY]); \
			glEnd(); glEnable(GL_LINE_SMOOTH);}

	glDepthMask( GL_FALSE );
	glEnable(GL_BLEND);
	glBlendFunc( GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_LIGHTING);
	glDisable( GL_FOG );
	glPushMatrix();
	sgMat4 modelview, tmp;
    ssgGetModelviewMatrix( modelview );
	sgCopyMat4( tmp, sgEnviro.transform );
    sgPostMultMat4( tmp, modelview );
    ssgLoadModelviewMatrix( tmp );

    Point3D start( sgEnviro.last_lon*SG_DEGREES_TO_RADIANS, sgEnviro.last_lat*SG_DEGREES_TO_RADIANS, 0.0 );
    Point3D dest( lon*SG_DEGREES_TO_RADIANS, lat*SG_DEGREES_TO_RADIANS, 0.0 );
    double course = 0.0, dist = 0.0;
    calc_gc_course_dist( dest, start, &course, &dist );
    double ax = 0.0, ay = 0.0;
    ax = cos(course) * dist;
    ay = sin(course) * dist;

	glTranslatef( ax, ay, -sgEnviro.last_alt );

	sgEnviro.radarEcho.push_back( SGWxRadarEcho ( course, 0.0, 0.0, dist, age, true, 0 ) );

	for( int n = 0 ; n < nb_tree ; n++ ) {
        if( lt_tree[n].prev < 0 )
			continue;

        float t1 = sgLerp(0.5, 1.0, lt_tree[n].pt[PY] / h);
		t1 *= flash;
		if( lt_tree[n].depth >= 2 ) {
            glLineWidth(3);
			sgScaleVec4(c, col, t1 * 0.6f);
			DRAW_SEG();
		} else {
			if( lt_tree[n].depth == 0 ) {
                glLineWidth(12);
				sgScaleVec4(c, col, t1 * 0.5f);
				DRAW_SEG();

                glLineWidth(6);
				sgScaleVec4(c, col, t1);
				DRAW_SEG();
			} else {
                glLineWidth(6);
				sgScaleVec4(c, col, t1 * 0.7f);
				DRAW_SEG();
			}

            if( lt_tree[n].depth == 0 ) 
                glLineWidth(3);
			else
                glLineWidth(2);

            sgSetVec4(c, t1, t1, t1, t1);
			DRAW_SEG();
		}

	}
    glLineWidth(1);
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
	glPopMatrix();
	glDepthMask( GL_TRUE );	
	glEnable( GL_FOG );
	glEnable(GL_LIGHTING);
}

void SGEnviro::addLightning(double lon, double lat, double alt) {
	if( lightnings.size() > 10)
		return;
	SGLightning *lt= new SGLightning(lon, lat, alt);
	lightnings.push_back(lt);
}

void SGEnviro::drawLightning(void) {
	list_of_lightning::iterator iLightning;
	// play 'thunder' for lightning
	if( snd_active )
		if( !snd_playing ) {
			// wait until sound has reached us
			snd_timer += dt;
			if( snd_timer >= snd_wait ) {
				snd_playing = true;
				// compute relative position of lightning
				Point3D start( sgEnviro.last_lon*SG_DEGREES_TO_RADIANS, sgEnviro.last_lat*SG_DEGREES_TO_RADIANS, 0.0 );
				Point3D dest( snd_pos_lon*SG_DEGREES_TO_RADIANS, snd_pos_lat*SG_DEGREES_TO_RADIANS, 0.0 );
				double course = 0.0, dist = 0.0;
				calc_gc_course_dist( dest, start, &course, &dist );
				double ax = 0.0, ay = 0.0;
				ax = cos(course) * dist;
				ay = sin(course) * dist;
				SGSharedPtr<SGSoundSample> snd = soundMgr->find("thunder");
				if( snd ) {
					ALfloat pos[3]={ax, ay, -sgEnviro.last_alt };
					snd->set_source_pos(pos);
					snd->play_once();
				}
			}
		} else {
			if( !soundMgr->is_playing("thunder") ) {
				snd_active = false;
				snd_playing = false;
			}
		}

	if( ! lightning_enable_state )
		return;

	for( iLightning = lightnings.begin() ; iLightning != lightnings.end() ; iLightning++ ) {
		if( dt )
			if( sg_random() > 0.95f )
				(*iLightning)->lt_build();
		(*iLightning)->lt_Render();
		(*iLightning)->age -= dt;
		if( (*iLightning)->age < 0.0 ) {
			delete (*iLightning);
			lightnings.erase( iLightning );
			break;
		}
	}

}


void SGEnviro::setFOV( float w, float h ) {
	fov_width = w;
	fov_height = h;
}

void SGEnviro::getFOV( float &w, float &h ) {
	w = fov_width;
	h = fov_height;
}
