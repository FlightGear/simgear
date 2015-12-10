// sample_openal.cxx -- Audio sample encapsulation class
// 
// Written by Curtis Olson, started April 2004.
// Modified to match the new SoundSystem by Erik Hofman, October 2009
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
// Copyright (C) 2009 Erik Hofman <erik@ehofman.com>
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
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <stdlib.h>	// rand(), free()
#include <cstring>
#include <stdio.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/ResourceManager.hxx>

#include "soundmgr_openal.hxx"
#include "sample_openal.hxx"
#include "soundmgr_openal_private.hxx"

#define AL_FALSE 0

using std::string;

//
// SGSoundSample
//

// empty constructor
SGSoundSample::SGSoundSample() :
    _format(AL_FORMAT_MONO8),
    _size(0),
    _freq(0),
    _changed(true),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3d::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3f::zeros()),
    _orientation(SGQuatd::zeros()),
    _orivec(SGVec3f::zeros()),
    _base_pos(SGVec3d::zeros()),
    _rotation(SGQuatd::zeros()),
    _refname(random_string()),
    _data(NULL),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER),
    _inner_angle(360.0),
    _outer_angle(360.0),
    _outer_gain(0.0),
    _pitch(1.0),
    _volume(1.0),
    _master_volume(1.0),
    _reference_dist(500.0),
    _max_dist(3000.0),
    _loop(AL_FALSE),
    _playing(false),
    _static_changed(true),
    _out_of_range(false),
    _is_file(false),
    _use_pos_props(false)
{
    _pos_prop[0] = 0;
    _pos_prop[1] = 0;
    _pos_prop[2] = 0;
}

// constructor
SGSoundSample::SGSoundSample(const char *file, const SGPath& currentDir) :
    _format(AL_FORMAT_MONO8),
    _size(0),
    _freq(0),
    _changed(true),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3d::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3f::zeros()),
    _orientation(SGQuatd::zeros()),
    _orivec(SGVec3f::zeros()),
    _base_pos(SGVec3d::zeros()),
    _rotation(SGQuatd::zeros()),
    _refname(file),
    _data(NULL),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER),
    _inner_angle(360.0),
    _outer_angle(360.0),
    _outer_gain(0.0),
    _pitch(1.0),
    _volume(1.0),
    _master_volume(1.0),
    _reference_dist(500.0),
    _max_dist(3000.0),
    _loop(AL_FALSE),
    _playing(false),
    _static_changed(true),
    _out_of_range(false),
    _is_file(true),
    _use_pos_props(false)
{
    SGPath p = simgear::ResourceManager::instance()->findPath(file, currentDir);
    _refname = p.str();
    _pos_prop[0] = 0;
    _pos_prop[1] = 0;
    _pos_prop[2] = 0;
}

// constructor
SGSoundSample::SGSoundSample( const unsigned char** data,
                              int len, int freq, int format ) :
    _format(format),
    _size(len),
    _freq(freq),
    _changed(true),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3d::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3f::zeros()),
    _orientation(SGQuatd::zeros()),
    _orivec(SGVec3f::zeros()),
    _base_pos(SGVec3d::zeros()),
    _rotation(SGQuatd::zeros()),
    _refname(random_string()),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER),
    _inner_angle(360.0),
    _outer_angle(360.0),
    _outer_gain(0.0),
    _pitch(1.0),
    _volume(1.0),
    _master_volume(1.0),
    _reference_dist(500.0),
    _max_dist(3000.0),
    _loop(AL_FALSE),
    _playing(false),
    _static_changed(true),
    _out_of_range(false),
    _is_file(false),
    _use_pos_props(false)
{
    SG_LOG( SG_SOUND, SG_DEBUG, "In memory sounds sample" );
    _data = (unsigned char*)*data; *data = NULL;
    _pos_prop[0] = 0;
    _pos_prop[1] = 0;
    _pos_prop[2] = 0;
}

// constructor
SGSoundSample::SGSoundSample( void** data, int len, int freq, int format ) :
    _format(format),
    _size(len),
    _freq(freq),
    _changed(true),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3d::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3f::zeros()),
    _orientation(SGQuatd::zeros()),
    _orivec(SGVec3f::zeros()),
    _base_pos(SGVec3d::zeros()),
    _rotation(SGQuatd::zeros()),
    _refname(random_string()),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER),
    _inner_angle(360.0),
    _outer_angle(360.0),
    _outer_gain(0.0),
    _pitch(1.0),
    _volume(1.0),
    _master_volume(1.0),
    _reference_dist(500.0),
    _max_dist(3000.0),
    _loop(AL_FALSE),
    _playing(false),
    _static_changed(true),
    _out_of_range(false),
    _is_file(false),
    _use_pos_props(false)
{
    SG_LOG( SG_SOUND, SG_DEBUG, "In memory sounds sample" );
    _data = (unsigned char*)*data; *data = NULL;
    _pos_prop[0] = 0;
    _pos_prop[1] = 0;
    _pos_prop[2] = 0;
}


// destructor
SGSoundSample::~SGSoundSample() {
    if ( _data != NULL ) free(_data);
}

void SGSoundSample::update_pos_and_orientation() {

    if (_use_pos_props) {
        if (_pos_prop[0]) _absolute_pos[0] = _pos_prop[0]->getDoubleValue();
        if (_pos_prop[1]) _absolute_pos[1] = _pos_prop[1]->getDoubleValue();
        if (_pos_prop[2]) _absolute_pos[2] = _pos_prop[2]->getDoubleValue();
    }
    else {
        _absolute_pos = _base_pos;
        if (_relative_pos[0] || _relative_pos[1] || _relative_pos[2] ) {
           _absolute_pos += _rotation.rotate( _relative_pos );
        }
    }

    _orivec = SGVec3f::zeros();
    if ( _direction[0] || _direction[1] || _direction[2] ) {
        _orivec = toVec3f( _rotation.rotate( _direction ) );
    }
}

string SGSoundSample::random_string() {
      static const char *r = "0123456789abcdefghijklmnopqrstuvwxyz"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      string rstr = "System generated name: ";
      for (int i=0; i<10; i++) {
          rstr.push_back( r[rand() % strlen(r)] );
      }

      return rstr;
}

SGPath SGSoundSample::file_path() const
{
  if (!_is_file) {
    return SGPath();
  }
  
  return SGPath(_refname);
}

void SGSoundSample::free_data()
{
   if ( _data != NULL ) free( _data ); _data = NULL;
}
