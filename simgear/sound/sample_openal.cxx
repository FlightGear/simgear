// sample.cxx -- Sound sample encapsulation class
// 
// Written by Curtis Olson, started April 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/SGQuat.hxx>

#include "soundmgr_openal.hxx"
#include "sample_openal.hxx"


//
// SGSoundSample
//

// empty constructor
SGSoundSample::SGSoundSample() :
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3f::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3d::zeros()),
    _base_pos(SGGeod()),
    _orientation(SGQuatd::zeros()),
    _sample_name(""),
    _data(NULL),
    _format(AL_FORMAT_MONO8),
    _size(0),
    _freq(0),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
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
    _changed(true),
    _static_changed(true),
    _is_file(false)
{
}

// constructor
SGSoundSample::SGSoundSample( const char *path, const char *file ) :
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3f::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3d::zeros()),
    _base_pos(SGGeod()),
    _orientation(SGQuatd::zeros()),
    _format(AL_FORMAT_MONO8),
    _size(0),
    _freq(0),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
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
    _changed(true),
    _static_changed(true),
    _is_file(true)
{
    SGPath samplepath( path );
    if ( strlen(file) ) {
        samplepath.append( file );
    }
    _sample_name = samplepath.str();

     SG_LOG( SG_GENERAL, SG_DEBUG, "From file sounds sample = "
            << samplepath.str() );
}

// constructor
SGSoundSample::SGSoundSample( unsigned char *data, int len, int freq, int format ) :
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3f::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3d::zeros()),
    _base_pos(SGGeod()),
    _orientation(SGQuatd::zeros()),
    _data(data),
    _format(format),
    _size(len),
    _freq(freq),
    _valid_buffer(false),
    _buffer(SGSoundMgr::NO_BUFFER),
    _valid_source(false),
    _source(SGSoundMgr::NO_SOURCE),
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
    _changed(true),
    _static_changed(true),
    _is_file(false)
{
    _sample_name = "unknown, data supplied by caller";
    SG_LOG( SG_GENERAL, SG_DEBUG, "In memory sounds sample" );
}


// destructor
SGSoundSample::~SGSoundSample() {
}

void SGSoundSample::set_orientation( SGQuatd ori ) {
    _orientation = ori;
    update_absolute_position();
    _changed = true;
}

void SGSoundSample::set_direction( SGVec3d dir ) {
    _direction = dir;
    update_absolute_position();
    _changed = true;
}

float *SGSoundSample::get_orientation() {
    SGQuatd orient = SGQuatd::fromLonLat(_base_pos) * _orientation;
    SGVec3d orivec = -orient.rotate(_direction);
    return toVec3f(orivec).data();
}

void SGSoundSample::set_relative_position( SGVec3f pos ) {
    _relative_pos = pos;
    update_absolute_position();
    _changed = true;
}

void SGSoundSample::set_position( SGGeod pos ) {
    _base_pos = pos;
    update_absolute_position();
    _changed = true;
}

void SGSoundSample::update_absolute_position() {
    _absolute_pos = -SGVec3d::fromGeod(_base_pos);
    // TODO: add relative position
}
