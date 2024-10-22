// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2004 Curtis L. Olson - http://www.flightgear.org/~curt
// SPDX-FileCopyrightText: 2009-2019 Erik Hofman <erik@ehofman.com>

/**
 * @file
 * @brief Audio sample encapsulation class
 * @note   C++-ified by Curtis Olson, started March 2001.
 * @note   Modified for the new SoundSystem by Erik Hofman, October 2009
 */


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

// Constructor
SGSoundSample::SGSoundSample(const SGPath& file) :
    _is_file(true)
{
    _refname = file.utf8Str();
}

// Delegating constructor that goes through the ResourceManager
SGSoundSample::SGSoundSample(const string& file, const SGPath& dir) :
    SGSoundSample(
        simgear::ResourceManager::instance()->findPath(file, dir))
{ }

// constructor
SGSoundSample::SGSoundSample( std::unique_ptr<unsigned char, decltype(free)*> data,
                              int len, int freq, int format )
{
    SG_LOG( SG_SOUND, SG_DEBUG, "In memory sounds sample" );

    _data = std::move(data);

    set_frequency(freq);
    set_format(format);
    set_size(len);
}

void SGSoundSample::update_pos_and_orientation() {

    if (_use_pos_props) {
        if (_pos_prop[0]) _relative_pos[0] = -_pos_prop[0]->getDoubleValue();
        if (_pos_prop[1]) _relative_pos[1] = -_pos_prop[1]->getDoubleValue();
        if (_pos_prop[2]) _relative_pos[2] = -_pos_prop[2]->getDoubleValue();
    }
    _absolute_pos = _base_pos;
    if (_relative_pos[0] || _relative_pos[1] || _relative_pos[2] ) {
       _absolute_pos += _rotation.rotate( _relative_pos );
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
