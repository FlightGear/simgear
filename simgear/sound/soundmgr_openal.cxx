// soundmgr.cxx -- Sound effect management class
//
// Sound manager initially written by David Findlay
// <david_j_findlay@yahoo.com.au> 2001
//
// C++-ified by Curtis Olson, started March 2001.
// Modified for the new SoundSystem by Erik Hofman, October 2009
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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

#if defined( __APPLE__ )
# include <OpenAL/alut.h>
#else
# include <AL/alut.h>
#endif

#include <iostream>
#include <algorithm>

#include "soundmgr_openal.hxx"

#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/SGMath.hxx>


#define MAX_SOURCES	128

//
// Sound Manager
//

int SGSoundMgr::_alut_init = 0;

// constructor
SGSoundMgr::SGSoundMgr() :
    _working(false),
    _active(false),
    _changed(true),
    _volume(0.0),
    _device(NULL),
    _context(NULL),
    _position(SGVec3d::zeros()),
    _velocity(SGVec3d::zeros()),
    _orientation(SGQuatd::zeros()),
    _devname(NULL)
{
#if defined(ALUT_API_MAJOR_VERSION) && ALUT_API_MAJOR_VERSION >= 1
    if (_alut_init == 0) {
        if ( !alutInitWithoutContext(NULL, NULL) ) {
            testForALUTError("alut initialization");
            return;
        }
    }
    _alut_init++;
#endif
}

// destructor

SGSoundMgr::~SGSoundMgr() {

    stop();
#if defined(ALUT_API_MAJOR_VERSION) && ALUT_API_MAJOR_VERSION >= 1
    _alut_init--;
    if (_alut_init == 0) {
        alutExit ();
    }
#endif
}

// initialize the sound manager
void SGSoundMgr::init() {

    SG_LOG( SG_GENERAL, SG_INFO, "Initializing OpenAL sound manager" );

    ALCdevice *device = alcOpenDevice(_devname);
    if ( testForError(device, "No default audio device available.") ) {
        return;
    }

    ALCcontext *context = alcCreateContext(device, NULL);
    if ( testForError(context, "Unable to create a valid context.") ) {
        alcCloseDevice (device);
        return;
    }

    if ( !alcMakeContextCurrent(context) ) {
        testForALCError("context initialization");
        alcDestroyContext (context);
        alcCloseDevice (device);
        return;
    }

    _context = context;
    _working = true;

    _at_up_vec[0] = 0.0; _at_up_vec[1] = 0.0; _at_up_vec[2] = -1.0;
    _at_up_vec[3] = 0.0; _at_up_vec[4] = 1.0; _at_up_vec[5] = 0.0;

    alListenerf( AL_GAIN, 0.0f );
    alListenerfv( AL_ORIENTATION, _at_up_vec );
    alListenerfv( AL_POSITION, SGVec3f::zeros().data() );
    alListenerfv( AL_VELOCITY, SGVec3f::zeros().data() );

    alDopplerFactor(1.0);
    alDopplerVelocity(340.3);   // speed of sound in meters per second.

    if ( alIsExtensionPresent((const ALchar*)"EXT_exponent_distance") ) {
        alDistanceModel(AL_EXPONENT_DISTANCE);
    } else {
        alDistanceModel(AL_INVERSE_DISTANCE);
    }

    testForALError("listener initialization");

    // get a free source one at a time
    // if an error is returned no more (hardware) sources are available
    for (unsigned int i=0; i<MAX_SOURCES; i++) {
        ALuint source;
        ALenum error;

        alGetError();
        alGenSources(1, &source);
        error = alGetError();
        if ( error == AL_NO_ERROR ) {
            _free_sources.push_back( source );
        }
        else break;
    }

    if (_free_sources.size() == 0) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Unable to grab any OpenAL sources!");
    }

    sample_group_map_iterator sample_grp_current = _sample_groups.begin();
    sample_group_map_iterator sample_grp_end = _sample_groups.end();
    for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
        SGSampleGroup *sgrp = sample_grp_current->second;
        sgrp->activate();
    }
}

// stop the sound manager
void SGSoundMgr::stop() {
    if (_working) {
        _working = false;
        _active = false;

        // clear any OpenAL buffers before shutting down
        buffer_map_iterator buffers_current = _buffers.begin();
        buffer_map_iterator buffers_end = _buffers.end();
        for ( ; buffers_current != buffers_end; ++buffers_current ) {
            refUint ref = buffers_current->second;
            ALuint buffer = ref.id;
            alDeleteBuffers(1, &buffer);
        }
        _buffers.clear();

        _context = alcGetCurrentContext();
        _device = alcGetContextsDevice(_context);
        alcDestroyContext(_context);
        alcCloseDevice(_device);
    }
}

void SGSoundMgr::suspend() {
    if (_active) {
        sample_group_map_iterator sample_grp_current = _sample_groups.begin();
        sample_group_map_iterator sample_grp_end = _sample_groups.end();
        for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
            SGSampleGroup *sgrp = sample_grp_current->second;
            sgrp->suspend();
        }
        _active = false;
    }
}

void SGSoundMgr::resume() {
    if (!_active) {
        sample_group_map_iterator sample_grp_current = _sample_groups.begin();
        sample_group_map_iterator sample_grp_end = _sample_groups.end();
        for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
            SGSampleGroup *sgrp = sample_grp_current->second;
            sgrp->resume();
        }
        _active = true;
    }
}

void SGSoundMgr::bind ()
{
    _free_sources.clear();
    _free_sources.reserve( MAX_SOURCES );
    _sources_in_use.clear();
    _sources_in_use.reserve( MAX_SOURCES );
}


void SGSoundMgr::unbind ()
{
    _sample_groups.clear();

    // delete free sources
    for (unsigned int i=0; i<_free_sources.size(); i++) {
        ALuint source = _free_sources.at( i );
        alDeleteSources( 1 , &source );
    }

    _free_sources.clear();
    _sources_in_use.clear();
}

// run the audio scheduler
void SGSoundMgr::update_late( double dt ) {
    if (_active) {
        sample_group_map_iterator sample_grp_current = _sample_groups.begin();
        sample_group_map_iterator sample_grp_end = _sample_groups.end();
        for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
            SGSampleGroup *sgrp = sample_grp_current->second;
            sgrp->update(dt);
        }

        if (_changed) {
            alListenerf( AL_GAIN, _volume );
            alListenerfv( AL_ORIENTATION, _at_up_vec );
            alListenerfv( AL_POSITION, toVec3f(_position).data() );
            alListenerfv( AL_VELOCITY, toVec3f(_velocity).data() );
            // alDopplerVelocity(340.3);	// TODO: altitude dependent
            testForALError("update");
            _changed = false;
        }
    }
}

// add a sample group, return true if successful
bool SGSoundMgr::add( SGSampleGroup *sgrp, const string& refname )
{
    sample_group_map_iterator sample_grp_it = _sample_groups.find( refname );
    if ( sample_grp_it != _sample_groups.end() ) {
        // sample group already exists
        return false;
    }

    if (_working) sgrp->activate();
    _sample_groups[refname] = sgrp;

    return true;
}


// remove a sound effect, return true if successful
bool SGSoundMgr::remove( const string &refname )
{
    sample_group_map_iterator sample_grp_it = _sample_groups.find( refname );
    if ( sample_grp_it == _sample_groups.end() ) {
        // sample group was not found.
        return false;
    }

    _sample_groups.erase( refname );

    return true;
}


// return true of the specified sound exists in the sound manager system
bool SGSoundMgr::exists( const string &refname ) {
    sample_group_map_iterator sample_grp_it = _sample_groups.find( refname );
    if ( sample_grp_it == _sample_groups.end() ) {
        // sample group was not found.
        return false;
    }

    return true;
}


// return a pointer to the SGSampleGroup if the specified sound exists
// in the sound manager system, otherwise return NULL
SGSampleGroup *SGSoundMgr::find( const string &refname, bool create ) {
    sample_group_map_iterator sample_grp_it = _sample_groups.find( refname );
    if ( sample_grp_it == _sample_groups.end() ) {
        // sample group was not found.
        if (create) {
            SGSampleGroup* sgrp = new SGSampleGroup(this, refname);
            return sgrp;
        }
        else 
            return NULL;
    }

    return sample_grp_it->second;
}


void SGSoundMgr::set_volume( float v )
{
    _volume = v;
    if (_volume > 1.0) _volume = 1.0;
    if (_volume < 0.0) _volume = 0.0;
    _changed = true;
}

/**
 * set the orientation of the listener (in opengl coordinates)
 *
 * Description: ORIENTATION is a pair of 3-tuples representing the
 * 'at' direction vector and 'up' direction of the Object in
 * Cartesian space. AL expects two vectors that are orthogonal to
 * each other. These vectors are not expected to be normalized. If
 * one or more vectors have zero length, implementation behavior
 * is undefined. If the two vectors are linearly dependent,
 * behavior is undefined.
 */
void SGSoundMgr::set_orientation( SGQuatd ori )
{
    _orientation = ori;

    SGVec3d sgv_up = ori.rotate(SGVec3d::e2());
    SGVec3d sgv_at = ori.rotate(SGVec3d::e3());
    _at_up_vec[0] = sgv_at[0];
    _at_up_vec[1] = sgv_at[1];
    _at_up_vec[2] = sgv_at[2];
    _at_up_vec[3] = sgv_up[0];
    _at_up_vec[4] = sgv_up[1];
    _at_up_vec[5] = sgv_up[2];
    _changed = true;
}

// Get an unused source id
//
// The Sound Manager should keep track of the sources in use, the distance
// of these sources to the listener and the volume (also based on audio cone
// and hence orientation) of the sources.
//
// The Sound Manager is (and should be) the only one knowing about source
// management. Sources further away should be suspendped to free resources for
// newly added sounds close by.
unsigned int SGSoundMgr::request_source()
{
    unsigned int source = NO_SOURCE;

    if (_free_sources.size() > 0) {
       source = _free_sources.back();
       _free_sources.pop_back();
       _sources_in_use.push_back(source);
    }

    return source;
}

// Free up a source id for further use
void SGSoundMgr::release_source( unsigned int source )
{
    vector<ALuint>::iterator it;

    it = std::find(_sources_in_use.begin(), _sources_in_use.end(), source);
    if ( it != _sources_in_use.end() ) {
        ALint result;

        alGetSourcei( source, AL_SOURCE_STATE, &result );
        if ( result == AL_PLAYING )
            alSourceStop( source );
        testForALError("release source");

        _free_sources.push_back(source);
        _sources_in_use.erase(it, it+1);
    }
}

unsigned int SGSoundMgr::request_buffer(SGSoundSample *sample)
{
    ALuint buffer = NO_BUFFER;

    if ( !sample->is_valid_buffer() ) {
        // sample was not yet loaded or removed again
        string sample_name = sample->get_sample_name();

        // see if the sample name is already cached
        buffer_map_iterator buffer_it = _buffers.find( sample_name );
        if ( buffer_it != _buffers.end() ) {
            buffer_it->second.refctr++;
            buffer = buffer_it->second.id;
            sample->set_buffer( buffer );
            return buffer;
        }

        // sample name was not found in the buffer cache.
        if ( sample->is_file() ) {
            size_t size;
            int freq, format;
            void *data;

            load(sample_name, &data, &format, &size, &freq);
            std::auto_ptr<unsigned char> ptr;
            ptr.reset((unsigned char *)data);

            sample->set_data( ptr );
            sample->set_frequency( freq );
            sample->set_format( format );
            sample->set_size( size );
        }

        // create an OpenAL buffer handle
        alGenBuffers(1, &buffer);
        if ( !testForALError("generate buffer") ) {
            // Copy data to the internal OpenAL buffer

            const ALvoid *data = sample->get_data();
            ALenum format = sample->get_format();
            ALsizei size = sample->get_size();
            ALsizei freq = sample->get_frequency();
            alBufferData( buffer, format, data, size, freq );

            // If this sample was read from a file we have all the information
            // needed to read it again. For data buffers provided by the
            // program we don't; so don't delete it's data.
            if ( sample->is_file() ) sample->free_data();

            if ( !testForALError("buffer add data") ) {
                sample->set_buffer(buffer);
                _buffers[sample_name] = refUint(buffer);
            }
        }
    }
    else
        buffer = sample->get_buffer();

    return buffer;
}

void SGSoundMgr::release_buffer(SGSoundSample *sample)
{
    string sample_name = sample->get_sample_name();
    buffer_map_iterator buffer_it = _buffers.find( sample_name );
    if ( buffer_it == _buffers.end() ) {
        // buffer was not found
        return;
    }

    sample->no_valid_buffer();
    buffer_it->second.refctr--;
    if (buffer_it->second.refctr == 0) {
        ALuint buffer = buffer_it->second.id;
        _buffers.erase( sample_name );
        alDeleteBuffers(1, &buffer);
        testForALError("release buffer");
    }
}

bool SGSoundMgr::load(string &samplepath, void **dbuf, int *fmt,
                                          size_t *sz, int *frq )
{
    if ( !_working ) return false;

    ALenum format;
    ALsizei size;
    ALsizei freq;
    ALvoid *data;

#if defined(ALUT_API_MAJOR_VERSION) && ALUT_API_MAJOR_VERSION >= 1
    ALfloat freqf;
    data = alutLoadMemoryFromFile(samplepath.c_str(), &format, &size, &freqf );
    freq = (ALsizei)freqf;
    if (data == NULL) {
        int error = alutGetError();
        string msg = "Failed to load wav file: ";
        msg.append(alutGetErrorString(error));
        throw sg_io_exception(msg.c_str(), sg_location(samplepath));
        return false;
    }

#else
    ALbyte *fname = (ALbyte *)samplepath.c_str();
# if defined (__APPLE__)
    alutLoadWAVFile( fname, &format, &data, &size, &freq );
# else
    ALboolean loop;
    alutLoadWAVFile( fname, &format, &data, &size, &freq, &loop );
# endif
    ALenum error =  alGetError();
    if ( error != AL_NO_ERROR ) {
        string msg = "Failed to load wav file: ";
        msg.append(alGetString(error));
        throw sg_io_exception(msg.c_str(), sg_location(samplepath));
        return false;
    }
#endif

    *dbuf = (void *)data;
    *fmt = (int)format;
    *sz = (size_t)size;
    *frq = (int)freq;

    return true;
}


bool SGSoundMgr::testForError(void *p, string s)
{
   if (p == NULL) {
      SG_LOG( SG_GENERAL, SG_ALERT, "Error: " << s);
      return true;
   }
   return false;
}


bool SGSoundMgr::testForALError(string s)
{
    ALenum error = alGetError();
    if (error != AL_NO_ERROR)  {
       SG_LOG( SG_GENERAL, SG_ALERT, "AL Error (sound manager): "
                                      << alGetString(error) << " at " << s);
       return true;
    }
    return false;
}

bool SGSoundMgr::testForALCError(string s)
{
    ALCenum error;
    error = alcGetError(_device);
    if (error != ALC_NO_ERROR) {
        SG_LOG( SG_GENERAL, SG_ALERT, "ALC Error (sound manager): "
                                       << alcGetString(_device, error) << " at "
                                       << s);
        return true;
    }
    return false;
}

bool SGSoundMgr::testForALUTError(string s)
{
#if defined(ALUT_API_MAJOR_VERSION) && ALUT_API_MAJOR_VERSION >= 1
    ALenum error;
    error =  alutGetError ();
    if (error != ALUT_ERROR_NO_ERROR) {
        SG_LOG( SG_GENERAL, SG_ALERT, "ALUT Error (sound manager): "
                                       << alutGetErrorString(error) << " at "
                                       << s);
        return true;
    }
#endif
    return false;
}
