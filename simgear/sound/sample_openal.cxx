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

#include <stdlib.h>	// rand()

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/SGMath.hxx>

#include "soundmgr_openal.hxx"
#include "sample_openal.hxx"


//
// SGSoundSample
//

// empty constructor
SGSoundSample::SGSoundSample() :
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3d::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3d::zeros()),
    _orientation(SGQuatd::zeros()),
    _orivec(SGVec3f::zeros()),
    _base_pos(SGGeod::fromDeg(0,0)),
    _refname(random_string()),
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
    _relative_pos(SGVec3d::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3d::zeros()),
    _orientation(SGQuatd::zeros()),
    _orivec(SGVec3f::zeros()),
    _base_pos(SGGeod::fromDeg(0,0)),
    _refname(file),
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
    _is_file(true)
{
    SGPath samplepath( path );
    if ( strlen(file) ) {
        samplepath.append( file );
    }
    _refname = samplepath.str();

     SG_LOG( SG_GENERAL, SG_DEBUG, "From file sounds sample = "
            << samplepath.str() );
}

// constructor
SGSoundSample::SGSoundSample( const unsigned char** data,
                              int len, int freq, int format ) :
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3d::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3d::zeros()),
    _orientation(SGQuatd::zeros()),
    _orivec(SGVec3f::zeros()),
    _base_pos(SGGeod::fromDeg(0,0)),
    _refname(random_string()),
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
    SG_LOG( SG_GENERAL, SG_DEBUG, "In memory sounds sample" );
    _data = (unsigned char*)*data; *data = NULL;
}

// constructor
SGSoundSample::SGSoundSample( void** data, int len, int freq, int format ) :
    _absolute_pos(SGVec3d::zeros()),
    _relative_pos(SGVec3d::zeros()),
    _direction(SGVec3d::zeros()),
    _velocity(SGVec3d::zeros()),
    _orientation(SGQuatd::zeros()),
    _orivec(SGVec3f::zeros()),
    _base_pos(SGGeod::fromDeg(0,0)),
    _refname(random_string()),
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
    SG_LOG( SG_GENERAL, SG_DEBUG, "In memory sounds sample" );
    _data = (unsigned char*)*data; *data = NULL;
}


// destructor
SGSoundSample::~SGSoundSample() {
    if (_data) free( _data );
    _data = NULL;
}

void SGSoundSample::set_orientation( const SGQuatd& ori ) {
    _orientation = ori;
    update_absolute_position();
    _changed = true;
}

void SGSoundSample::set_direction( const SGVec3d& dir ) {
    _direction = dir;
    update_absolute_position();
    _changed = true;
}

void SGSoundSample::set_relative_position( const SGVec3f& pos ) {
    _relative_pos = toVec3d(pos);
    update_absolute_position();
    _changed = true;
}

void SGSoundSample::set_position( const SGGeod& pos ) {
    _base_pos = pos;
    update_absolute_position();
    _changed = true;
}

void SGSoundSample::update_absolute_position() {
    //  SGQuatd orient = SGQuatd::fromLonLat(_base_pos) * _orientation;
    //  _orivec = -toVec3f(orient.rotate(-SGVec3d::e1()));

    // The rotation rotating from the earth centerd frame to
    // the horizontal local frame
    SGQuatd hlOr = SGQuatd::fromLonLat(_base_pos);

    // Compute the eyepoints orientation and position
    // wrt the earth centered frame - that is global coorinates
    SGQuatd ec2body = hlOr*_orientation;

    // This is rotates the x-forward, y-right, z-down coordinate system where
    // simulation runs into the OpenGL camera system with x-right, y-up, z-back.
    SGQuatd q(-0.5, -0.5, 0.5, 0.5);

    // The cartesian position of the basic view coordinate
    SGVec3d position = SGVec3d::fromGeod(_base_pos);

    _absolute_pos = position + (ec2body*q).backTransform(_relative_pos);
    _orivec = toVec3f( (ec2body*q).backTransform(_direction) );
}

string SGSoundSample::random_string() {
      static const char *r = "0123456789abcdefghijklmnopqrstuvwxyz"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      string rstr;
      for (int i=0; i<10; i++) {
          rstr.push_back( r[rand() % strlen(r)] );
      }

      return rstr;
}
