// sample.cxx -- Audio sample encapsulation class
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

#include "soundmgr.hxx"
#include "sample.hxx"

#define AL_FALSE 0

using std::string;

//
// SGSoundSampleInfo
//

// empty constructor
SGSoundSampleInfo::SGSoundSampleInfo() :
    _refname(random_string()),
    _bits(16),
    _tracks(1),
    _samples(0),
    _frequency(22500),
    _compressed(false),
    _loop(false),
    _static_changed(true),
    _playing(false),
    _pitch(1.0f),
    _volume(1.0f),
    _master_volume(1.0f),
    _use_pos_props(false),
    _out_of_range(false),
    _inner_angle(360.0f),
    _outer_angle(360.0f),
    _outer_gain(0.0f),
    _reference_dist(500.0f),
    _max_dist(3000.0f),
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3d::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3f::zeros()),
    _orientation(SGQuatd::zeros()),
    _orivec(SGVec3f::zeros()),
    _base_pos(SGVec3d::zeros()),
    _rotation(SGQuatd::zeros())
{
    _pos_prop[0] = 0;
    _pos_prop[1] = 0;
    _pos_prop[2] = 0;
}

std::string SGSoundSampleInfo::random_string()
{
    static const char *r = "0123456789abcdefghijklmnopqrstuvwxyz"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    string rstr = "Auto: ";
    for (int i=0; i<10; i++) {
        rstr.push_back( r[rand() % strlen(r)] );
    }

    return rstr;
}

//
// SGSoundSample
//

// empty constructor
SGSoundSample::SGSoundSample() :
    _is_file(false),
    _changed(true),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
    _data(NULL),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER)
{
}

// constructor
SGSoundSample::SGSoundSample(const char *file, const SGPath& currentDir) :
    _is_file(true),
    _changed(true),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
    _data(NULL),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER)
{
    SGPath p = simgear::ResourceManager::instance()->findPath(file, currentDir);
    _refname = p.utf8Str();
}

// constructor
SGSoundSample::SGSoundSample( const unsigned char** data,
                              int len, int freq, int format ) :
    _is_file(false),
    _changed(true),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER)
{
    SG_LOG( SG_SOUND, SG_DEBUG, "In memory sounds sample" );
    _data = (unsigned char*)*data; *data = NULL;

    set_frequency(freq);
    set_format(format);
    set_size(len);
}

// constructor
SGSoundSample::SGSoundSample( void** data, int len, int freq, int format ) :
    _is_file(false),
    _changed(true),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER)
{
    SG_LOG( SG_SOUND, SG_DEBUG, "In memory sounds sample" );
    _data = (unsigned char*)*data; *data = NULL;

    set_frequency(freq);
    set_format(format);
    set_size(len);
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

SGPath SGSoundSample::file_path() const
{
  if (!_is_file) {
    return SGPath();
  }

  return SGPath(_refname);
}

void SGSoundSample::free_data()
{
   if ( _data != NULL ) {
     free( _data );
   }
   _data = NULL;
}
