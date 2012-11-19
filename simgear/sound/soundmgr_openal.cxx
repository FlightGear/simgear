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

#include <iostream>
#include <algorithm>
#include <cstring>

#include <boost/foreach.hpp>

#include "soundmgr_openal.hxx"
#include "readwav.hxx"
#include "soundmgr_openal_private.hxx"
#include "sample_group.hxx"

#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

using std::string;
using std::vector;


#define MAX_SOURCES	128


#ifndef ALC_ALL_DEVICES_SPECIFIER
# define ALC_ALL_DEVICES_SPECIFIER	0x1013
#endif

class SGSoundMgr::SoundManagerPrivate
{
public:
    SoundManagerPrivate() :
        _device(NULL),
        _context(NULL),
        _absolute_pos(SGVec3d::zeros()),
        _base_pos(SGVec3d::zeros()),
        _orientation(SGQuatd::zeros())
    {
        
        
    }
    
    void init()
    {
        _at_up_vec[0] = 0.0; _at_up_vec[1] = 0.0; _at_up_vec[2] = -1.0;
        _at_up_vec[3] = 0.0; _at_up_vec[4] = 1.0; _at_up_vec[5] = 0.0;
    }
    
    void update_pos_and_orientation()
    {
        /**
         * Description: ORIENTATION is a pair of 3-tuples representing the
         * 'at' direction vector and 'up' direction of the Object in
         * Cartesian space. AL expects two vectors that are orthogonal to
         * each other. These vectors are not expected to be normalized. If
         * one or more vectors have zero length, implementation behavior
         * is undefined. If the two vectors are linearly dependent,
         * behavior is undefined.
         *
         * This is in the same coordinate system as OpenGL; y=up, z=back, x=right.
         */
        SGVec3d sgv_at = _orientation.backTransform(-SGVec3d::e3());
        SGVec3d sgv_up = _orientation.backTransform(SGVec3d::e2());
        _at_up_vec[0] = sgv_at[0];
        _at_up_vec[1] = sgv_at[1];
        _at_up_vec[2] = sgv_at[2];
        _at_up_vec[3] = sgv_up[0];
        _at_up_vec[4] = sgv_up[1];
        _at_up_vec[5] = sgv_up[2];

        _absolute_pos = _base_pos;
    }
        
    ALCdevice *_device;
    ALCcontext *_context;
    
    std::vector<ALuint> _free_sources;
    std::vector<ALuint> _sources_in_use;
    
    SGVec3d _absolute_pos;
    SGVec3d _base_pos;
    SGQuatd _orientation;
    // Orientation of the listener. 
    // first 3 elements are "at" vector, second 3 are "up" vector
    ALfloat _at_up_vec[6];
    
    sample_group_map _sample_groups;
    buffer_map _buffers;
};


//
// Sound Manager
//

// constructor
SGSoundMgr::SGSoundMgr() :
    _active(false),
    _changed(true),
    _volume(0.0),
    _offset_pos(SGVec3d::zeros()),
    _velocity(SGVec3d::zeros()),
    _bad_doppler(false),
    _renderer("unknown"),
    _vendor("unknown")
{
    d.reset(new SoundManagerPrivate);
    d->_base_pos = SGVec3d::fromGeod(_geod_pos);
}

// destructor

SGSoundMgr::~SGSoundMgr() {

    if (is_working())
        stop();
    d->_sample_groups.clear();
}

// initialize the sound manager
void SGSoundMgr::init() {

    if (is_working())
    {
        SG_LOG( SG_SOUND, SG_ALERT, "Oops, OpenAL sound manager is already initialized." );
        return;
    }

    SG_LOG( SG_SOUND, SG_INFO, "Initializing OpenAL sound manager" );

    d->_free_sources.clear();
    d->_free_sources.reserve( MAX_SOURCES );
    d->_sources_in_use.clear();
    d->_sources_in_use.reserve( MAX_SOURCES );

    ALCdevice *device = NULL;
    const char* devname = _device_name.c_str();
    if (_device_name == "")
        devname = NULL; // use default device
    else
    {
        // try non-default device
        device = alcOpenDevice(devname);
    }

    if ((!devname)||(testForError(device, "Audio device not available, trying default.")) ) {
        device = alcOpenDevice(NULL);
        if (testForError(device, "Default audio device not available.") ) {
           return;
        }
    }

    ALCcontext *context = alcCreateContext(device, NULL);
    testForALCError("context creation.");
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

    if (d->_context != NULL)
    {
        SG_LOG(SG_SOUND, SG_ALERT, "context is already assigned");
    }
    
    d->_context = context;
    d->_device = device;
    d->init();

    alListenerf( AL_GAIN, 0.0f );
    alListenerfv( AL_ORIENTATION, d->_at_up_vec );
    alListenerfv( AL_POSITION, SGVec3f::zeros().data() );
    alListenerfv( AL_VELOCITY, SGVec3f::zeros().data() );

    alDopplerFactor(1.0);
    alDopplerVelocity(340.3);   // speed of sound in meters per second.

    // gain = AL_REFERENCE_DISTANCE / (AL_REFERENCE_DISTANCE +
    //        AL_ROLLOFF_FACTOR * (distance - AL_REFERENCE_DISTANCE));
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

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
            d->_free_sources.push_back( source );
        } else {
            SG_LOG(SG_SOUND, SG_INFO, "allocating source failed:" << i);
            break;
        }
    }

    _vendor = (const char *)alGetString(AL_VENDOR);
    _renderer = (const char *)alGetString(AL_RENDERER);

    if (_vendor == "Creative Labs Inc.") {
       _bad_doppler = true;

    } else if (_vendor == "OpenAL Community" && _renderer == "OpenAL Soft") {
       _bad_doppler = true;
    }

    if (d->_free_sources.empty()) {
        SG_LOG(SG_SOUND, SG_ALERT, "Unable to grab any OpenAL sources!");
    }
}

void SGSoundMgr::reinit()
{
    bool was_active = _active;

    if (was_active)
    {
        suspend();
    }

    SGSoundMgr::stop();
    SGSoundMgr::init();

    if (was_active)
        resume();
}

void SGSoundMgr::activate() {
    if ( is_working() ) {
        _active = true;
                
        sample_group_map_iterator sample_grp_current = d->_sample_groups.begin();
        sample_group_map_iterator sample_grp_end = d->_sample_groups.end();
        for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
            SGSampleGroup *sgrp = sample_grp_current->second;
            sgrp->activate();
        }
    }
}

// stop the sound manager
void SGSoundMgr::stop() {

    // first stop all sample groups
    sample_group_map_iterator sample_grp_current = d->_sample_groups.begin();
    sample_group_map_iterator sample_grp_end = d->_sample_groups.end();
    for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
        SGSampleGroup *sgrp = sample_grp_current->second;
        sgrp->stop();
    }

    // clear all OpenAL sources
    BOOST_FOREACH(ALuint source, d->_free_sources) {
        alDeleteSources( 1 , &source );
        testForALError("SGSoundMgr::stop: delete sources");
    }
    d->_free_sources.clear();

    // clear any OpenAL buffers before shutting down
    buffer_map_iterator buffers_current = d->_buffers.begin();
    buffer_map_iterator buffers_end = d->_buffers.end();
    for ( ; buffers_current != buffers_end; ++buffers_current ) {
        refUint ref = buffers_current->second;
        ALuint buffer = ref.id;
        alDeleteBuffers(1, &buffer);
        testForALError("SGSoundMgr::stop: delete buffers");
    }
    
    d->_buffers.clear();
    d->_sources_in_use.clear();

    if (is_working()) {
        _active = false;
        alcDestroyContext(d->_context);
        alcCloseDevice(d->_device);
        d->_context = NULL;
        d->_device = NULL;

        _renderer = "unknown";
        _vendor = "unknown";
    }
}

void SGSoundMgr::suspend() {
    if (is_working()) {
        sample_group_map_iterator sample_grp_current = d->_sample_groups.begin();
        sample_group_map_iterator sample_grp_end = d->_sample_groups.end();
        for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
            SGSampleGroup *sgrp = sample_grp_current->second;
            sgrp->stop();
        }
        _active = false;
    }
}

void SGSoundMgr::resume() {
    if (is_working()) {
        sample_group_map_iterator sample_grp_current = d->_sample_groups.begin();
        sample_group_map_iterator sample_grp_end = d->_sample_groups.end();
        for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
            SGSampleGroup *sgrp = sample_grp_current->second;
            sgrp->resume();
        }
        _active = true;
    }
}

// run the audio scheduler
void SGSoundMgr::update( double dt ) {
    if (_active) {
        alcSuspendContext(d->_context);

        if (_changed) {
            d->update_pos_and_orientation();
        }

        sample_group_map_iterator sample_grp_current = d->_sample_groups.begin();
        sample_group_map_iterator sample_grp_end = d->_sample_groups.end();
        for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
            SGSampleGroup *sgrp = sample_grp_current->second;
            sgrp->update(dt);
        }

        if (_changed) {
#if 0
if (isNaN(_at_up_vec)) printf("NaN in listener orientation\n");
if (isNaN(toVec3f(_absolute_pos).data())) printf("NaN in listener position\n");
if (isNaN(_velocity.data())) printf("NaN in listener velocity\n");
#endif
            alListenerf( AL_GAIN, _volume );
            alListenerfv( AL_ORIENTATION, d->_at_up_vec );
            // alListenerfv( AL_POSITION, toVec3f(_absolute_pos).data() );

            SGQuatd hlOr = SGQuatd::fromLonLat( _geod_pos );
            SGVec3d velocity = SGVec3d::zeros();
            if ( _velocity[0] || _velocity[1] || _velocity[2] ) {
                velocity = hlOr.backTransform(_velocity*SG_FEET_TO_METER);
            }

            if ( _bad_doppler ) {
                velocity *= 100.0f;
            }

            alListenerfv( AL_VELOCITY, toVec3f(velocity).data() );
            // alDopplerVelocity(340.3);	// TODO: altitude dependent
            testForALError("update");
            _changed = false;
        }

        alcProcessContext(d->_context);
    }
}

// add a sample group, return true if successful
bool SGSoundMgr::add( SGSampleGroup *sgrp, const string& refname )
{
    sample_group_map_iterator sample_grp_it = d->_sample_groups.find( refname );
    if ( sample_grp_it != d->_sample_groups.end() ) {
        // sample group already exists
        return false;
    }

    if (_active) sgrp->activate();
    d->_sample_groups[refname] = sgrp;

    return true;
}


// remove a sound effect, return true if successful
bool SGSoundMgr::remove( const string &refname )
{
    sample_group_map_iterator sample_grp_it = d->_sample_groups.find( refname );
    if ( sample_grp_it == d->_sample_groups.end() ) {
        // sample group was not found.
        return false;
    }

    d->_sample_groups.erase( sample_grp_it );

    return true;
}


// return true of the specified sound exists in the sound manager system
bool SGSoundMgr::exists( const string &refname ) {
    sample_group_map_iterator sample_grp_it = d->_sample_groups.find( refname );
    return ( sample_grp_it != d->_sample_groups.end() );
}


// return a pointer to the SGSampleGroup if the specified sound exists
// in the sound manager system, otherwise return NULL
SGSampleGroup *SGSoundMgr::find( const string &refname, bool create ) {
    sample_group_map_iterator sample_grp_it = d->_sample_groups.find( refname );
    if ( sample_grp_it == d->_sample_groups.end() ) {
        // sample group was not found.
        if (create) {
            SGSampleGroup* sgrp = new SGSampleGroup(this, refname);
            add( sgrp, refname );
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
    SG_CLAMP_RANGE(_volume, 0.0f, 1.0f);
    _changed = true;
}

// Get an unused source id
//
// The Sound Manager should keep track of the sources in use, the distance
// of these sources to the listener and the volume (also based on audio cone
// and hence orientation) of the sources.
//
// The Sound Manager is (and should be) the only one knowing about source
// management. Sources further away should be suspended to free resources for
// newly added sounds close by.
unsigned int SGSoundMgr::request_source()
{
    unsigned int source = NO_SOURCE;

    if (!d->_free_sources.empty()) {
       source = d->_free_sources.back();
       d->_free_sources.pop_back();
       d->_sources_in_use.push_back(source);
    }
    else
       SG_LOG( SG_SOUND, SG_BULK, "Sound manager: No more free sources available!\n");

    return source;
}

// Free up a source id for further use
void SGSoundMgr::release_source( unsigned int source )
{
    vector<ALuint>::iterator it;

    it = std::find(d->_sources_in_use.begin(), d->_sources_in_use.end(), source);
    if ( it != d->_sources_in_use.end() ) {
        ALint result;

        alGetSourcei( source, AL_SOURCE_STATE, &result );
        if ( result == AL_PLAYING || result == AL_PAUSED ) {
            alSourceStop( source );
        }

        alSourcei( source, AL_BUFFER, 0 );	// detach the associated buffer
        testForALError("release_source");
        d->_free_sources.push_back( source );
        d->_sources_in_use.erase( it );
    }
}

unsigned int SGSoundMgr::request_buffer(SGSoundSample *sample)
{
    ALuint buffer = NO_BUFFER;

    if ( !sample->is_valid_buffer() ) {
        // sample was not yet loaded or removed again
        string sample_name = sample->get_sample_name();
        void *sample_data = NULL;

        // see if the sample name is already cached
        buffer_map_iterator buffer_it = d->_buffers.find( sample_name );
        if ( buffer_it != d->_buffers.end() ) {
            buffer_it->second.refctr++;
            buffer = buffer_it->second.id;
            sample->set_buffer( buffer );
            return buffer;
        }

        // sample name was not found in the buffer cache.
        if ( sample->is_file() ) {
            int freq, format;
            size_t size;

            try {
              bool res = load(sample_name, &sample_data, &format, &size, &freq);
              if (res == false) return NO_BUFFER;
            } catch (sg_exception& e) {
              SG_LOG(SG_SOUND, SG_ALERT,
                    "failed to load sound buffer: " << e.getFormattedMessage());
              sample->set_buffer( SGSoundMgr::FAILED_BUFFER );
              return FAILED_BUFFER;
            }
            
            sample->set_frequency( freq );
            sample->set_format( format );
            sample->set_size( size );

        } else {
            sample_data = sample->get_data();
        }

        // create an OpenAL buffer handle
        alGenBuffers(1, &buffer);
        if ( !testForALError("generate buffer") ) {
            // Copy data to the internal OpenAL buffer

            ALenum format = sample->get_format();
            ALsizei size = sample->get_size();
            ALsizei freq = sample->get_frequency();
            alBufferData( buffer, format, sample_data, size, freq );

            if ( !testForALError("buffer add data") ) {
                sample->set_buffer(buffer);
                d->_buffers[sample_name] = refUint(buffer);
            }
        }

        if ( sample->is_file() ) free(sample_data);
    }
    else {
        buffer = sample->get_buffer();
    }

    return buffer;
}

void SGSoundMgr::release_buffer(SGSoundSample *sample)
{
    if ( !sample->is_queue() )
    {
        string sample_name = sample->get_sample_name();
        buffer_map_iterator buffer_it = d->_buffers.find( sample_name );
        if ( buffer_it == d->_buffers.end() ) {
            // buffer was not found
            return;
        }

        sample->no_valid_buffer();
        buffer_it->second.refctr--;
        if (buffer_it->second.refctr == 0) {
            ALuint buffer = buffer_it->second.id;
            alDeleteBuffers(1, &buffer);
            d->_buffers.erase( buffer_it );
            testForALError("release buffer");
        }
    }
}

bool SGSoundMgr::load(const string &samplepath, void **dbuf, int *fmt,
                                          size_t *sz, int *frq )
{
    if ( !is_working() )
        return false;

    ALenum format;
    ALsizei size;
    ALsizei freq;
    ALvoid *data;

    ALfloat freqf;

    data = simgear::loadWAVFromFile(samplepath, format, size, freqf );
    freq = (ALsizei)freqf;
    if (data == NULL) {
        throw sg_io_exception("Failed to load wav file", sg_location(samplepath));
    }

    if (format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16) {
        free(data);
        throw sg_io_exception("Warning: STEREO files are not supported for 3D audio effects: " + samplepath);
    }

    *dbuf = (void *)data;
    *fmt = (int)format;
    *sz = (size_t)size;
    *frq = (int)freq;

    return true;
}

vector<const char*> SGSoundMgr::get_available_devices()
{
    vector<const char*> devices;
    const ALCchar *s;

    if (alcIsExtensionPresent(NULL, "ALC_enumerate_all_EXT") == AL_TRUE) {
        s = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    } else {
        s = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    }

    if (s) {
        ALCchar *nptr, *ptr = (ALCchar *)s;

        nptr = ptr;
        while (*(nptr += strlen(ptr)+1) != 0)
        {
            devices.push_back(ptr);
            ptr = nptr;
        }
        devices.push_back(ptr);
    }

    return devices;
}


bool SGSoundMgr::testForError(void *p, string s)
{
   if (p == NULL) {
      SG_LOG( SG_SOUND, SG_ALERT, "Error: " << s);
      return true;
   }
   return false;
}


bool SGSoundMgr::testForALError(string s)
{
    ALenum error = alGetError();
    if (error != AL_NO_ERROR)  {
       SG_LOG( SG_SOUND, SG_ALERT, "AL Error (sound manager): "
                                      << alGetString(error) << " at " << s);
       return true;
    }
    return false;
}

bool SGSoundMgr::testForALCError(string s)
{
    ALCenum error;
    error = alcGetError(d->_device);
    if (error != ALC_NO_ERROR) {
        SG_LOG( SG_SOUND, SG_ALERT, "ALC Error (sound manager): "
                                       << alcGetString(d->_device, error) << " at "
                                       << s);
        return true;
    }
    return false;
}

bool SGSoundMgr::is_working() const 
{
    return (d->_device != NULL);
}

const SGQuatd& SGSoundMgr::get_orientation() const
{
    return d->_orientation;
}

void SGSoundMgr::set_orientation( const SGQuatd& ori )
{
    d->_orientation = ori;
    _changed = true;
}

const SGVec3d& SGSoundMgr::get_position() const
{ 
    return d->_absolute_pos;
}

void SGSoundMgr::set_position( const SGVec3d& pos, const SGGeod& pos_geod )
{
    d->_base_pos = pos; _geod_pos = pos_geod; _changed = true;
}

SGVec3f SGSoundMgr::get_direction() const
{
    return SGVec3f(d->_at_up_vec[0], d->_at_up_vec[1], d->_at_up_vec[2]);
}
