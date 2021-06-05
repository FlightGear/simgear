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
#include <simgear/math/sg_random.hxx>
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
SGVec3f SGEnviro::min_light(DFL_MIN_LIGHT, DFL_MIN_LIGHT, DFL_MIN_LIGHT);
#define DFL_STREAK_BRIGHT_NEARMOST_LAYER 0.9
float SGEnviro::streak_bright_nearmost_layer = DFL_STREAK_BRIGHT_NEARMOST_LAYER;
#define DFL_STREAK_BRIGHT_FARMOST_LAYER 0.5
float SGEnviro::streak_bright_farmost_layer = DFL_STREAK_BRIGHT_FARMOST_LAYER;
#define DFL_STREAK_PERIOD_MAX 2.5
float SGEnviro::streak_period_max = DFL_STREAK_PERIOD_MAX;
#define DFL_STREAK_PERIOD_CHANGE_PER_KT 0.005
float SGEnviro::streak_period_change_per_kt = DFL_STREAK_PERIOD_CHANGE_PER_KT;
#define DFL_STREAK_PERIOD_MIN 1.0
float SGEnviro::streak_period_min = DFL_STREAK_PERIOD_MIN;
#define DFL_STREAK_LENGTH_MIN 0.03
float SGEnviro::streak_length_min = DFL_STREAK_LENGTH_MIN;
#define DFL_STREAK_LENGTH_CHANGE_PER_KT 0.0005
float SGEnviro::streak_length_change_per_kt = DFL_STREAK_LENGTH_CHANGE_PER_KT;
#define DFL_STREAK_LENGTH_MAX 0.1
float SGEnviro::streak_length_max = DFL_STREAK_LENGTH_MAX;
#define DFL_STREAK_COUNT_MIN 40
int SGEnviro::streak_count_min = DFL_STREAK_COUNT_MIN;
#define DFL_STREAK_COUNT_MAX 190
#if (DFL_STREAK_COUNT_MAX > MAX_RAIN_SLICE)
#error "Bad default!"
#endif
int SGEnviro::streak_count_max = DFL_STREAK_COUNT_MAX;
#define DFL_CONE_BASE_RADIUS 15.0
float SGEnviro::cone_base_radius = DFL_CONE_BASE_RADIUS;
#define DFL_CONE_HEIGHT 30.0
float SGEnviro::cone_height = DFL_CONE_HEIGHT;


void SGEnviro::config(const SGPropertyNode* n)
{
	if (!n)
		return;

	const float ml = n->getFloatValue("min-light", DFL_MIN_LIGHT);
  min_light = SGVec3f(ml, ml, ml);

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
  // if (sampleGroup) delete sampleGroup;

  // OSGFIXME
  return;
}

void SGEnviro::startOfFrame( SGVec3f p, SGVec3f up, double lon, double lat, double alt, double delta_time) {
  // OSGFIXME
  return;
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

void SGEnviro::setLight(SGVec4f adj_fog_color) {
  // OSGFIXME
  return;
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
}

void SGEnviro::drawRain(double pitch, double roll, double heading, double hspeed, double rain_norm) {
  // OSGFIXME
  return;
}

void SGEnviro::set_sampleGroup(SGSampleGroup *sgr) {
	sampleGroup = sgr;
}

void SGEnviro::drawPrecipitation(double rain_norm, double snow_norm, double hail_norm, double pitch, double roll, double heading, double hspeed) {
  // OSGFIXME
  return;
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
