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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/constants.h>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/sound/sample_group.hxx>
#include <simgear/scene/sky/cloudfield.hxx>
#include <simgear/scene/sky/newcloud.hxx>
#include <simgear/props/props.hxx>
#include "visual_enviro.hxx"

#include <vector>

using std::vector;


typedef struct {
	SGVec3d		pt;
	int			depth;
	int			prev;
} lt_tree_seg;

#define MAX_RAIN_SLICE	200
static float rainpos[MAX_RAIN_SLICE];
#define MAX_LT_TREE_SEG	400

#define DFL_MIN_LIGHT 0.35
sgVec3 SGEnviro::min_light = {DFL_MIN_LIGHT, DFL_MIN_LIGHT, DFL_MIN_LIGHT};
#define DFL_STREAK_BRIGHT_NEARMOST_LAYER 0.9
SGfloat SGEnviro::streak_bright_nearmost_layer = DFL_STREAK_BRIGHT_NEARMOST_LAYER;
#define DFL_STREAK_BRIGHT_FARMOST_LAYER 0.5
SGfloat SGEnviro::streak_bright_farmost_layer = DFL_STREAK_BRIGHT_FARMOST_LAYER;
#define DFL_STREAK_PERIOD_MAX 2.5
SGfloat SGEnviro::streak_period_max = DFL_STREAK_PERIOD_MAX;
#define DFL_STREAK_PERIOD_CHANGE_PER_KT 0.005
SGfloat SGEnviro::streak_period_change_per_kt = DFL_STREAK_PERIOD_CHANGE_PER_KT;
#define DFL_STREAK_PERIOD_MIN 1.0
SGfloat SGEnviro::streak_period_min = DFL_STREAK_PERIOD_MIN;
#define DFL_STREAK_LENGTH_MIN 0.03
SGfloat SGEnviro::streak_length_min = DFL_STREAK_LENGTH_MIN;
#define DFL_STREAK_LENGTH_CHANGE_PER_KT 0.0005
SGfloat SGEnviro::streak_length_change_per_kt = DFL_STREAK_LENGTH_CHANGE_PER_KT;
#define DFL_STREAK_LENGTH_MAX 0.1
SGfloat SGEnviro::streak_length_max = DFL_STREAK_LENGTH_MAX;
#define DFL_STREAK_COUNT_MIN 40
int SGEnviro::streak_count_min = DFL_STREAK_COUNT_MIN;
#define DFL_STREAK_COUNT_MAX 190
#if (DFL_STREAK_COUNT_MAX > MAX_RAIN_SLICE)
#error "Bad default!"
#endif
int SGEnviro::streak_count_max = DFL_STREAK_COUNT_MAX;
#define DFL_CONE_BASE_RADIUS 15.0
SGfloat SGEnviro::cone_base_radius = DFL_CONE_BASE_RADIUS;
#define DFL_CONE_HEIGHT 30.0
SGfloat SGEnviro::cone_height = DFL_CONE_HEIGHT;


void SGEnviro::config(const SGPropertyNode* n)
{
	if (!n)
		return;

	const float ml = n->getFloatValue("min-light", DFL_MIN_LIGHT);
	sgSetVec3(min_light, ml, ml, ml);

	streak_bright_nearmost_layer = n->getFloatValue(
			"streak-brightness-nearmost-layer",
			DFL_STREAK_BRIGHT_NEARMOST_LAYER);
	streak_bright_farmost_layer = n->getFloatValue(
			"streak-brightness-farmost-layer",
			DFL_STREAK_BRIGHT_FARMOST_LAYER);

	streak_period_max = n->getFloatValue(
			"streak-period-max",
			DFL_STREAK_PERIOD_MAX);
	streak_period_min = n->getFloatValue(
			"streak-period-min",
			DFL_STREAK_PERIOD_MIN);
	streak_period_change_per_kt = n->getFloatValue(
			"streak-period-change-per-kt",
			DFL_STREAK_PERIOD_CHANGE_PER_KT);

	streak_length_max = n->getFloatValue(
			"streak-length-max",
			DFL_STREAK_LENGTH_MAX);
	streak_length_min = n->getFloatValue(
			"streak-length-min",
			DFL_STREAK_LENGTH_MIN);
	streak_length_change_per_kt = n->getFloatValue(
			"streak-length-change-per-kt",
			DFL_STREAK_LENGTH_CHANGE_PER_KT);

	streak_count_min = n->getIntValue(
			"streak-count-min", DFL_STREAK_COUNT_MIN);
	streak_count_max = n->getIntValue(
			"streak-count-max", DFL_STREAK_COUNT_MAX);
	if (streak_count_max > MAX_RAIN_SLICE)
		streak_count_max = MAX_RAIN_SLICE;

	cone_base_radius = n->getFloatValue(
			"cone-base-radius", DFL_CONE_BASE_RADIUS);
	cone_height = n->getFloatValue("cone_height", DFL_CONE_HEIGHT);
}


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
	void lt_build_tree_branch(int tree_nr, SGVec3d &start, float energy, int nbseg, float segsize);

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

SGEnviro::SGEnviro() :
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
	sampleGroup(NULL),
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
	if (sampleGroup) delete sampleGroup;

  // OSGFIXME
  return;
	list_of_lightning::iterator iLightning;
	for( iLightning = lightnings.begin() ; iLightning != lightnings.end() ; iLightning++ ) {
		delete (*iLightning);
	}
	lightnings.clear();
}

void SGEnviro::startOfFrame( sgVec3 p, sgVec3 up, double lon, double lat, double alt, double delta_time) {
  // OSGFIXME
  return;
	view_in_cloud = false;
	// ask the impostor cache to do some cleanup
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

bool SGEnviro::get_turbulence_enable_state(void) const {
	return turbulence_enable_state;
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
  // OSGFIXME
  return;
	sgCopyVec4( fog_color, adj_fog_color );
	if( false ) {
	//    ssgGetLight( 0 ) -> setColour( GL_DIFFUSE, l->scene_diffuse() );
	}
}
#if 0
void SGEnviro::callback_cloud(float heading, float alt, float radius, int family, float dist, int cloudId) {
	// send data to wx radar
	// compute turbulence
	// draw precipitation
	// draw lightning
	// compute illumination

	// http://www.pilotfriend.com/flight_training/weather/THUNDERSTORM%20HAZARDS1.htm
	double turbulence = 0.0;
	if( dist < radius * radius * 2.25f ) {
		switch(family) {
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
	switch(family) {
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
	if(lightning_enable_state && min_time_before_lt <= 0.0 && (family == SGNewCloud::CLFamilly_cb) &&
		dist < 15000.0 * 15000.0 && sg_random() > 0.9f) {
		double lat, lon;
		SGVec3d orig, dest;
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
		switch(family) {
			case SGNewCloud::CLFamilly_st:
			case SGNewCloud::CLFamilly_cu:
			case SGNewCloud::CLFamilly_cb:
			case SGNewCloud::CLFamilly_ns:
			case SGNewCloud::CLFamilly_sc:
				precipitation_max_alt = alt - radius * 0.1;
				break;
		}
}

#endif

list_of_SGWxRadarEcho *SGEnviro::get_radar_echo(void) {
	return &radarEcho;
}

// precipitation rendering code
void SGEnviro::DrawCone2(float baseRadius, float height, int slices, bool down, double rain_norm, double speed) {
  // OSGFIXME
  return;
	sgVec3 light;
	sgAddVec3( light, fog_color, min_light );
	float da = SG_PI * 2.0f / (float) slices;
	// low number = faster
	float speedf = streak_period_max - speed * streak_period_change_per_kt;
	if( speedf < streak_period_min )
		speedf = streak_period_min;
	float lenf = streak_length_min + speed * streak_length_change_per_kt;
	if( lenf > streak_length_max )
		lenf = streak_length_max;
    float t = fmod((float) elapsed_time, speedf) / speedf;
//	t = 0.1f;
	if( !down )
		t = 1.0f - t;
	float angle = 0.0f;
	//glColor4f(1.0f, 0.7f, 0.7f, 0.9f); // XXX unneeded? overriden below
	glBegin(GL_LINES);
	if (slices >  MAX_RAIN_SLICE)
		slices = MAX_RAIN_SLICE; // should never happen
	for( int i = 0 ; i < slices ; i++ ) {
		float x = cos(angle) * baseRadius;
		float y = sin(angle) * baseRadius;
		angle += da;
		sgVec3 dir = {x, -height, y};

		// rain drops at 2 different speed to simulate depth
		float t1 = (i & 1 ? t : t + t) + rainpos[i];
		if(t1 > 1.0f)	t1 -= 1.0f;
		if(t1 > 1.0f)	t1 -= 1.0f;

		// distant raindrops are more transparent
		float c = t1 * (i & 1 ?
				streak_bright_farmost_layer
				: streak_bright_nearmost_layer);
		glColor4f(c * light[0], c * light[1], c * light[2], c);
		sgVec3 p1, p2;
		sgScaleVec3(p1, dir, t1);
		// distant raindrops are shorter
		float t2 = t1 + (i & 1 ? lenf : lenf+lenf);
		sgScaleVec3(p2, dir, t2);

		glVertex3f(p1[0], p1[1] + height, p1[2]);
		glVertex3f(p2[0], p2[1] + height, p2[2]);
	}
	glEnd();
}

void SGEnviro::drawRain(double pitch, double roll, double heading, double hspeed, double rain_norm) {
  // OSGFIXME
  return;

#if 0
	static int debug_period = 0;
	if (debug_period++ == 50) {
		debug_period = 0;
		cout << "drawRain("
			<< pitch << ", "
			<< roll  << ", "
			<< heading << ", "
			<< hspeed << ", "
			<< rain_norm << ");"
			//" angle = " << angle
			//<< " raindrop(KTS) = " << raindrop_speed_kts
			<< endl;
	}
#endif


	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_FOG );
	glDisable(GL_LIGHTING);

	int slice_count = static_cast<int>(
			(streak_count_min + rain_norm*(streak_count_max-streak_count_min))
				* precipitation_density / 100.0);

	// www.wonderquest.com/falling-raindrops.htm says that
	// Raindrop terminal velocity is 5 to 20mph
	// Rather than model it accurately (temp, pressure, diameter), and make it
	// smaller than terminal when closer to the precipitation cloud base,
	// we interpolate in the 5-20mph range according to rain_norm.
	double raindrop_speed_kts
		= (5.0 + rain_norm*15.0) * SG_MPH_TO_MPS * SG_MPS_TO_KT;

	float angle = atanf(hspeed / raindrop_speed_kts) * SG_RADIANS_TO_DEGREES;
	glPushMatrix();
		// the cone rotate with hspeed
		angle = -pitch - angle;
		glRotatef(roll, 0.0, 0.0, 1.0);
		glRotatef(heading, 0.0, 1.0, 0.0);
		glRotatef(angle, 1.0, 0.0, 0.0);

		// up cone
		DrawCone2(cone_base_radius, cone_height, 
				slice_count, true, rain_norm, hspeed);
		// down cone (usually not visible)
		if(angle > 0.0 || heading != 0.0)
			DrawCone2(cone_base_radius, -cone_height, 
					slice_count, false, rain_norm, hspeed);

	glPopMatrix();

	glEnable(GL_LIGHTING);
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
	glEnable( GL_FOG );
	glEnable(GL_DEPTH_TEST);

}

void SGEnviro::set_sampleGroup(SGSampleGroup *sgr) {
	sampleGroup = sgr;
}

void SGEnviro::drawPrecipitation(double rain_norm, double snow_norm, double hail_norm, double pitch, double roll, double heading, double hspeed) {
  // OSGFIXME
  return;
	if( precipitation_enable_state && rain_norm > 0.0)
	  if( precipitation_max_alt >= last_alt )
		drawRain(pitch, roll, heading, hspeed, rain_norm);
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
void SGLightning::lt_build_tree_branch(int tree_nr, SGVec3d &start, float energy, int nbseg, float segsize) {
  // OSGFIXME
  return;
}

void SGLightning::lt_build(void) {
  // OSGFIXME
  return;

}


void SGLightning::lt_Render(void) {
  // OSGFIXME
  return;
}

void SGEnviro::addLightning(double lon, double lat, double alt) {
  // OSGFIXME
  return;
	if( lightnings.size() > 10)
		return;
	SGLightning *lt= new SGLightning(lon, lat, alt);
	lightnings.push_back(lt);
}

void SGEnviro::drawLightning(void) {
  // OSGFIXME
  return;
}


void SGEnviro::setFOV( float w, float h ) {
	fov_width = w;
	fov_height = h;
}

void SGEnviro::getFOV( float &w, float &h ) {
	w = fov_width;
	h = fov_height;
}
