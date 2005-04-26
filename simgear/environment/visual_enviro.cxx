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

#include <simgear/scene/sky/cloudfield.hxx>
#include <simgear/scene/sky/newcloud.hxx>
#include "visual_enviro.hxx"

SGEnviro sgEnviro;

SGEnviro::SGEnviro(void) :
	view_in_cloud(false),
	precipitation_enable_state(false),
	precipitation_density(100.0)

{
}

SGEnviro::~SGEnviro(void) {
}

void SGEnviro::startOfFrame(void) {
	view_in_cloud = false;
	if(SGNewCloud::cldCache)
		SGNewCloud::cldCache->startNewFrame();
}

void SGEnviro::endOfFrame(void) {
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
	return false;
}

void SGEnviro::set_lightning_enable_state(bool enable) {
}

