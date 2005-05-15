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
#ifndef _VISUAL_ENVIRO_HXX
#define _VISUAL_ENVIRO_HXX

#include <simgear/compiler.h>
#include STL_STRING

SG_USING_STD(string);

class SGLightning;
class SGSoundMgr;

/**
 * Visual environment helper class.
 */
class SGEnviro {
friend struct SGLightning;
private:
	void DrawCone2(float baseRadius, float height, int slices, bool down, double rain_norm, double speed);
	void lt_update(void);

	bool view_in_cloud;
	bool precipitation_enable_state;
	float precipitation_density;
	bool turbulence_enable_state;
	double last_cloud_turbulence, cloud_turbulence;
	bool lightning_enable_state;
	double elapsed_time, dt;
	sgVec4	fog_color;
	sgMat4 transform;
	double last_lon, last_lat, last_alt;
	SGSoundMgr	*soundMgr;
	bool		snd_active, snd_playing;
	double		snd_timer, snd_wait, snd_pos_lat, snd_pos_lon, snd_dist;
	double		min_time_before_lt;

	float fov_width, fov_height;

public:
	SGEnviro();
	~SGEnviro();

    /**
     * Forward a few states used for renderings.
     */
	void startOfFrame( sgVec3 p, sgVec3 up, double lon, double lat, double alt, double delta_time);

	void endOfFrame(void);

    /**
     * Whenever a cloud is drawn we check his 'impact' on the environment.
     * @param heading direction of cloud in radians
     * @param alt asl of cloud in meters
     * @param radius radius of cloud in meters
     * @param familly cloud familly
     * @param dist  squared dist to cloud in meters
     */
	void callback_cloud(float heading, float alt, float radius, int familly, float dist);

	void drawRain(double pitch, double roll, double speed, double rain_norm);
    /**
     * Draw rain or snow precipitation around the viewer.
     * @param rain_norm rain normalized intensity given by metar class
     * @param snow_norm snow normalized intensity given by metar class
     * @param hail_norm hail normalized intensity given by metar class
     * @param pitch pitch rotation of viewer
     * @param roll roll rotation of viewer
     * @param speed moving speed of viewer in kt
     */
	void drawPrecipitation(double rain_norm, double snow_norm, double hail_norm,
							double pitch, double roll, double speed);

    /**
     * Draw the lightnings spawned by cumulo nimbus.
     */
	void drawLightning(void);

    /**
     * Forward the fog color used by the rain rendering.
     * @param adj_fog_color color of the fog
     */
	void setLight(sgVec4 adj_fog_color);

	// this can be queried to add some turbulence for example
	bool is_view_in_cloud(void) const;
	void set_view_in_cloud(bool incloud);
	double get_cloud_turbulence(void) const;

	// Clouds
	// return the size of the memory pool used by texture impostors
	int get_clouds_CacheSize(void) const;
	int get_CacheResolution(void) const;
	float get_clouds_visibility(void) const;
	float get_clouds_density(void) const;
	bool get_clouds_enable_state(void) const;
	bool get_turbulence_enable_state(void) const;

    /**
     * Set the size of the impostor texture cache for 3D clouds.
     * @param sizeKb size of the texture pool in Kb
     */
	void set_clouds_CacheSize(int sizeKb);
    /**
     * Set the resolution of the impostor texture for 3D clouds.
     * @param resolutionPixels size of each texture in pixels (64|128|256)
     */
	void set_CacheResolution(int resolutionPixels);
    /**
     * Set the maximum range used when drawing clouds.
	 * Clouds are blended from totaly transparent at max range to totaly opaque around the viewer
     * @param distance in meters
     */
	void set_clouds_visibility(float distance);
    /**
     * Set the proportion of clouds that will be rendered to limit drop in FPS.
     * @param density 0..100 no clouds drawn when density == 0, all are drawn when density == 100
     */
	void set_clouds_density(float density);
    /**
     * Enable or disable the use of 3D clouds.
     * @param enable when false we draw the 2D layers
     */
	void set_clouds_enable_state(bool enable);
    /**
     * Enable or disable the use of proximity cloud turbulence.
     * @param enable when true the turbulence is computed based on type of cloud around the AC
     */
	void set_turbulence_enable_state(bool enable);

	// rain/snow
	float get_precipitation_density(void) const;
	bool get_precipitation_enable_state(void) const;

	void set_precipitation_density(float density);
    /**
     * Enable or disable the rendering of precipitation around the viewer.
     * @param enable when true we will draw precipitation depending on metar data
     */
	void set_precipitation_enable_state(bool enable);

	// others
	bool get_lightning_enable_state(void) const;
    /**
     * Enable or disable the rendering of lightning and the thunder sound.
     * @param enable when true we will draw lightning spwaned by cumulonimbus
     */
	void set_lightning_enable_state(bool enable);

    /**
     * Spawn a new lighning at specified lon/lat.
     * @param lon position of the new lightning
     * @param lat position of the new lightning
     * @param alt asl of the starting point of the lightning in meters
     */
	void addLightning(double lon, double lat, double alt);

    /**
     * Forward the sound manager instance to be able to play samples.
     * @param mgr a running sound manager
     */
	void set_soundMgr(SGSoundMgr *mgr);

	void setFOV( float w, float h );
	void getFOV( float &w, float &h );

};

extern SGEnviro sgEnviro;

#endif // _VISUAL_ENVIRO_HXX
