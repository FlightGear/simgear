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

class SGEnviro {

private:
	bool view_in_cloud;
	bool precipitation_enable_state;
	float precipitation_density;

public:
	SGEnviro();
	~SGEnviro();

	void startOfFrame(void);
	void endOfFrame(void);

	// this can be queried to add some turbulence for example
	bool is_view_in_cloud(void) const;
	void set_view_in_cloud(bool incloud);

	// Clouds
	// return the size of the memory pool used by texture impostors
	int get_clouds_CacheSize(void) const;
	int get_CacheResolution(void) const;
	float get_clouds_visibility(void) const;
	float get_clouds_density(void) const;
	bool get_clouds_enable_state(void) const;

	void set_clouds_CacheSize(int sizeKb);
	void set_CacheResolution(int resolutionPixels);
	void set_clouds_visibility(float distance);
	void set_clouds_density(float density);
	void set_clouds_enable_state(bool enable);

	// rain/snow
	float get_precipitation_density(void) const;
	bool get_precipitation_enable_state(void) const;

	void set_precipitation_density(float density);
	void set_precipitation_enable_state(bool enable);

	// others
	bool get_lightning_enable_state(void) const;
	void set_lightning_enable_state(bool enable);
};

extern SGEnviro sgEnviro;

#endif // _VISUAL_ENVIRO_HXX