// soundmgr_openal.cxx -- Sound effect management class for OpenAL
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

#include <stdio.h>

#include <iostream>
#include <algorithm>
#include <cstring>
#include <cassert>

#include "soundmgr.hxx"
#include "readwav.hxx"
#include "sample_group.hxx"

#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#if defined(__APPLE__)
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#elif defined(OPENALSDK)
# include <al.h>
# include <alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

using std::vector;


#define MAX_SOURCES	128

#ifndef ALC_ALL_DEVICES_SPECIFIER
# define ALC_ALL_DEVICES_SPECIFIER	0x1013
#endif
#ifndef AL_FORMAT_MONO_MULAW_EXT
# define AL_FORMAT_MONO_MULAW_EXT	0x10014
#endif
#ifndef AL_FORMAT_MONO_IMA4
# define AL_FORMAT_MONO_IMA4		0x1300
#endif
#ifndef AL_UNPACK_BLOCK_ALIGNMENT_SOFT
# define AL_UNPACK_BLOCK_ALIGNMENT_SOFT	0x200C
#endif


struct refUint {
    unsigned int refctr;
    ALuint id;

    refUint() { refctr = 0; id = (ALuint)-1; };
    refUint(ALuint i) { refctr = 1; id = i; };
    ~refUint() {};
};

using buffer_map = std::map < std::string, refUint >;
using sample_group_map = std::map < std::string, SGSharedPtr<SGSampleGroup> >;

inline bool isNaN(float *v) {
   return (SGMisc<float>::isNaN(v[0]) || SGMisc<float>::isNaN(v[1]) || SGMisc<float>::isNaN(v[2]));
}


class SGSoundMgr::SoundManagerPrivate
{
public:
    SoundManagerPrivate() = default;
    ~SoundManagerPrivate() = default;
    
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
        
    ALCdevice *_device = nullptr;
    ALCcontext *_context = nullptr;
    
    std::vector<ALuint> _free_sources;
    std::vector<ALuint> _sources_in_use;
    
    SGVec3d _absolute_pos = SGVec3d::zeros();
    SGVec3d _base_pos = SGVec3d::zeros();
    SGQuatd _orientation = SGQuatd::zeros();
    // Orientation of the listener. 
    // first 3 elements are "at" vector, second 3 are "up" vector
    ALfloat _at_up_vec[6];

    bool _bad_doppler = false;
    
    sample_group_map _sample_groups;
    buffer_map _buffers;
};


//
// Sound Manager
//

// constructor
SGSoundMgr::SGSoundMgr() {
    d.reset(new SoundManagerPrivate);
    d->_base_pos = SGVec3d::fromGeod(_geod_pos);

    _block_support = alIsExtensionPresent((ALchar *)"AL_SOFT_block_alignment");
}

// destructor

SGSoundMgr::~SGSoundMgr() {

    if (is_working())
        stop();
    d->_sample_groups.clear();
}

// initialize the sound manager
void SGSoundMgr::init()
{
#ifdef ENABLE_SOUND
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

    ALCdevice *device = nullptr;
    const char* devname = _device_name.c_str();
    if (_device_name == "")
        devname = nullptr; // use default device
    else
    {
        // try non-default device
        device = alcOpenDevice(devname);
    }

    if ((!devname)||(testForError(device, "Audio device not available, trying default.")) ) {
        // REVIEW: Memory Leak - 26 bytes in 1 blocks are still reachable
        device = alcOpenDevice(nullptr);
        if (testForError(device, "Default audio device not available.") ) {
           return;
        }
    }

    // REVIEW: Memory Leak - 64 bytes in 1 blocks are still reachable
    ALCcontext *context = alcCreateContext(device, nullptr);
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

    if (d->_context != nullptr)
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

    alDopplerFactor(1.0f);
    alDopplerVelocity(_sound_velocity);

    // gain = AL_REFERENCE_DISTANCE / (AL_REFERENCE_DISTANCE +
    //        AL_ROLLOFF_FACTOR * (distance - AL_REFERENCE_DISTANCE));
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

    testForError("listener initialization");

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
       alDopplerFactor(100.0f);
       d->_bad_doppler = true;
    } else if (_vendor == "OpenAL Community" && _renderer == "OpenAL Soft") {
       alDopplerFactor(100.0f);
       d->_bad_doppler = true;
    }

    if (d->_free_sources.empty()) {
        SG_LOG(SG_SOUND, SG_ALERT, "Unable to grab any OpenAL sources!");
    }
#endif
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

void SGSoundMgr::activate()
{
#ifdef ENABLE_SOUND
    if ( is_working() ) {
        _active = true;
                
        for ( auto current : d->_sample_groups ) {
            current.second->activate();
        }
    }
#endif
}

// stop the sound manager
void SGSoundMgr::stop()
{
#ifdef ENABLE_SOUND
    // first stop all sample groups
    for ( auto current : d->_sample_groups ) {
        current.second->stop();
    }

    // clear all OpenAL sources
    for(ALuint source : d->_free_sources) {
        alDeleteSources( 1 , &source );
        testForError("SGSoundMgr::stop: delete sources");
    }
    d->_free_sources.clear();

    // clear any OpenAL buffers before shutting down
    for ( auto current : d->_buffers ) {
        refUint ref = current.second;
        ALuint buffer = ref.id;
        alDeleteBuffers(1, &buffer);
        testForError("SGSoundMgr::stop: delete buffers");
    }
    
    d->_buffers.clear();
    d->_sources_in_use.clear();

    if (is_working()) {
        _active = false;
        alcDestroyContext(d->_context);
        alcCloseDevice(d->_device);
        d->_context = nullptr;
        d->_device = nullptr;

        _renderer = "unknown";
        _vendor = "unknown";
    }
#endif
}

void SGSoundMgr::suspend()
{
#ifdef ENABLE_SOUND
    if (is_working()) {
        for ( auto current : d->_sample_groups ) {
            current.second->suspend();
        }
        _active = false;
    }
#endif
}

void SGSoundMgr::resume()
{
#ifdef ENABLE_SOUND
    if (is_working()) {
        for ( auto current : d->_sample_groups ) {
            current.second->resume();
        }
        _active = true;
    }
#endif
}

// run the audio scheduler
void SGSoundMgr::update( double dt )
{
#ifdef ENABLE_SOUND
    if (_active) {
        alcSuspendContext(d->_context);

        if (_changed) {
            d->update_pos_and_orientation();
        }

        for ( auto current : d->_sample_groups ) {
            current.second->update(dt);
        }

        if (_changed) {
#if 0
if (isNaN(d->_at_up_vec)) printf("NaN in listener orientation\n");
if (isNaN(toVec3f(d->_absolute_pos).data())) printf("NaN in listener position\n");
if (isNaN(toVec3f(_velocity).data())) printf("NaN in listener velocity\n");
#endif
            alListenerf( AL_GAIN, _volume );
            alListenerfv( AL_ORIENTATION, d->_at_up_vec );
            // alListenerfv( AL_POSITION, toVec3f(_absolute_pos).data() );

            SGQuatd hlOr = SGQuatd::fromLonLat( _geod_pos );
            SGVec3d velocity = SGVec3d::zeros();
            if ( _velocity[0] || _velocity[1] || _velocity[2] ) {
                velocity = hlOr.backTransform(_velocity*SG_FEET_TO_METER);

                if ( d->_bad_doppler ) {
                    float fact = 100.0f;
                    float mag = length( velocity );

                    if (mag > _sound_velocity) {
                        fact *= _sound_velocity / mag;
                    }
                    alDopplerFactor(fact);
                }
            }

            alListenerfv( AL_VELOCITY, toVec3f(velocity).data() );
//          alDopplerVelocity(_sound_velocity);
            testForError("update");
            _changed = false;
        }

        alcProcessContext(d->_context);
    }
#endif
}

// add a sample group, return true if successful
bool SGSoundMgr::add( SGSampleGroup *sgrp, const std::string& refname )
{
    auto sample_grp_it = d->_sample_groups.find( refname );
    if ( sample_grp_it != d->_sample_groups.end() ) {
        // sample group already exists
        return false;
    }

    if (_active) sgrp->activate();
    d->_sample_groups[refname] = sgrp;

    return true;
}


// remove a sound effect, return true if successful
bool SGSoundMgr::remove( const std::string &refname )
{
    auto sample_grp_it = d->_sample_groups.find( refname );
    if ( sample_grp_it == d->_sample_groups.end() ) {
        // sample group was not found.
        return false;
    }

    d->_sample_groups.erase( sample_grp_it );

    return true;
}


// return true of the specified sound exists in the sound manager system
bool SGSoundMgr::exists( const std::string &refname ) {
    auto sample_grp_it = d->_sample_groups.find( refname );
    return ( sample_grp_it != d->_sample_groups.end() );
}


// return a pointer to the SGSampleGroup if the specified sound exists
// in the sound manager system, otherwise return nullptr
SGSampleGroup *SGSoundMgr::find( const std::string &refname, bool create ) {
    auto sample_grp_it = d->_sample_groups.find( refname );
    if ( sample_grp_it == d->_sample_groups.end() ) {
        // sample group was not found.
        if (create) {
            SGSampleGroup* sgrp = new SGSampleGroup(this, refname);
            add( sgrp, refname );
            return sgrp;
        }
        else 
            return nullptr;
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
    auto it = std::find(d->_sources_in_use.begin(), d->_sources_in_use.end(), source);
    if ( it != d->_sources_in_use.end() ) {
  #ifdef ENABLE_SOUND
        ALint result;

        alGetSourcei( source, AL_SOURCE_STATE, &result );
        if ( result == AL_PLAYING || result == AL_PAUSED ) {
            alSourceStop( source );
        }

        alSourcei( source, AL_BUFFER, 0 );	// detach the associated buffer
        testForError("release_source");
  #endif
        d->_free_sources.push_back( source );
        d->_sources_in_use.erase( it );
    }
}

unsigned int SGSoundMgr::request_buffer(SGSoundSample *sample)
{
    ALuint buffer = NO_BUFFER;
#ifdef ENABLE_SOUND
    if ( !sample->is_valid_buffer() ) {
        // sample was not yet loaded or removed again
        std::string sample_name = sample->get_sample_name();
        void* sample_data = nullptr;

        // see if the sample name is already cached
        auto buffer_it = d->_buffers.find( sample_name );
        if ( buffer_it != d->_buffers.end() ) {
            buffer_it->second.refctr++;
            buffer = buffer_it->second.id;
            sample->set_buffer( buffer );
            return buffer;
        }

        // sample name was not found in the buffer cache.
        if ( sample->is_file() ) {
            int freq, format, block;
            size_t size;

            try {
              bool res = load(sample_name, &sample_data, &format, &size, &freq, &block);
              if (res == false) return NO_BUFFER;
            } catch (sg_exception& e) {
              SG_LOG(SG_SOUND, SG_ALERT,
                    "failed to load sound buffer:\n" << e.getFormattedMessage());
              sample->set_buffer( SGSoundMgr::FAILED_BUFFER );
              return FAILED_BUFFER;
            }
            
            sample->set_block_align( block );
            sample->set_frequency( freq );
            sample->set_format( format );
            sample->set_size( size );

        } else {
            sample_data = sample->get_data();
        }

        ALenum format = AL_NONE;
        switch( sample->get_format() )
        {
        case SG_SAMPLE_MONO16:
            format = AL_FORMAT_MONO16;
            break;
        case SG_SAMPLE_MONO8:
            format = AL_FORMAT_MONO8;
            break;
        case SG_SAMPLE_MULAW:
            format = AL_FORMAT_MONO_MULAW_EXT;
            break;
        case SG_SAMPLE_ADPCM:
            format = AL_FORMAT_MONO_IMA4;
            break;

        case SG_SAMPLE_STEREO16:
            SG_LOG(SG_SOUND, SG_POPUP, "Stereo sound detected:\n" << sample->get_sample_name() << "\nUse two separate mono files instead if required.");
            format = AL_FORMAT_STEREO16;
            break;
        case SG_SAMPLE_STEREO8:
            SG_LOG(SG_SOUND, SG_POPUP, "Stereo sound detected:\n" << sample->get_sample_name() << "\nUse two separate mono files instead if required.");
            format = AL_FORMAT_STEREO8;
            break;
        default:
            SG_LOG(SG_SOUND, SG_ALERT, "unsupported audio format");
            return buffer;
        }

        // create an OpenAL buffer handle
        alGenBuffers(1, &buffer);
        if ( !testForError("generate buffer") ) {
            // Copy data to the internal OpenAL buffer

            ALsizei size = sample->get_size();
            ALsizei freq = sample->get_frequency();
            alBufferData( buffer, format, sample_data, size, freq );

            if (format == AL_FORMAT_MONO_IMA4 && _block_support) {
                ALsizei samples_block = BLOCKSIZE_TO_SMP( sample->get_block_align() );
                alBufferi (buffer, AL_UNPACK_BLOCK_ALIGNMENT_SOFT, samples_block );
            }

            if ( !testForError("buffer add data") ) {
                sample->set_buffer(buffer);
                d->_buffers[sample_name] = refUint(buffer);
            }
        }

        if ( sample->is_file() ) free(sample_data);
    }
    else {
        buffer = sample->get_buffer();
    }
#endif
    return buffer;
}

void SGSoundMgr::release_buffer(SGSoundSample *sample)
{
    if ( !sample->is_queue() )
    {
        std::string sample_name = sample->get_sample_name();
        auto buffer_it = d->_buffers.find( sample_name );
        if ( buffer_it == d->_buffers.end() ) {
            // buffer was not found
            return;
        }

        sample->no_valid_buffer();
        buffer_it->second.refctr--;
        if (buffer_it->second.refctr == 0) {
#ifdef ENABLE_SOUND
            ALuint buffer = buffer_it->second.id;
            alDeleteBuffers(1, &buffer);
#endif
            d->_buffers.erase( buffer_it );
            testForError("release buffer");
        }
    }
}

void SGSoundMgr::sample_suspend( SGSoundSample *sample )
{
    if ( sample->is_valid_source() && sample->is_playing() ) {
        alSourcePause( sample->get_source() );
    }
}

void SGSoundMgr::sample_resume( SGSoundSample *sample )
{
    if ( sample->is_valid_source() && sample->is_playing() ) {
        alSourcePlay( sample->get_source() );
    }
}

void SGSoundMgr::sample_init( SGSoundSample *sample )
{
#ifdef ENABLE_SOUND
    //
    // a request to start playing a sound has been filed.
    //
    ALuint source = request_source();
    if (alIsSource(source) == AL_FALSE ) {
        return;
    }

    sample->set_source( source );
#endif
}

void SGSoundMgr::sample_play( SGSoundSample *sample )
{
#ifdef ENABLE_SOUND
    ALboolean looping = sample->is_looping() ? AL_TRUE : AL_FALSE;
    ALint source = sample->get_source();

    if ( !sample->is_queue() )
    {
        ALuint buffer = request_buffer(sample);
        if (buffer == SGSoundMgr::FAILED_BUFFER ||
            buffer == SGSoundMgr::NO_BUFFER)
        {
            release_source(source);
            return;
        }

        // start playing the sample
        buffer = sample->get_buffer();
        if ( alIsBuffer(buffer) == AL_TRUE )
        {
            alSourcei( source, AL_BUFFER, buffer );
            testForError("assign buffer to source");
        } else
            SG_LOG( SG_SOUND, SG_ALERT, "No such buffer!");
    }

    alSourcef( source, AL_ROLLOFF_FACTOR, 0.3 );
    alSourcei( source, AL_LOOPING, looping );
    alSourcei( source, AL_SOURCE_RELATIVE, AL_FALSE );
    alSourcePlay( source );
    testForError("sample play");
#endif
}

void SGSoundMgr::sample_stop( SGSoundSample *sample )
{
    if ( sample->is_valid_source() ) {
        int source = sample->get_source();
    
        bool stopped = is_sample_stopped(sample);
        if ( sample->is_looping() && !stopped) {
#ifdef ENABLE_SOUND
            alSourceStop( source );
#endif          
            stopped = is_sample_stopped(sample);
        }
    
        if ( stopped ) {
            sample->no_valid_source();
            release_source( source );
        }
    }
}

void SGSoundMgr::sample_destroy( SGSoundSample *sample )
{ 
    if ( sample->is_valid_source() ) {
#ifdef ENABLE_SOUND
        ALint source = sample->get_source();
        if ( sample->is_playing() ) {
            alSourceStop( source );
            testForError("stop");
        }
        release_source( source );
#endif
        sample->no_valid_source();
    }

    if ( sample->is_valid_buffer() ) {
        release_buffer( sample );
        sample->no_valid_buffer();
    }
}

bool SGSoundMgr::is_sample_stopped(SGSoundSample *sample)
{
#ifdef ENABLE_SOUND
    if ( sample->is_valid_source() ) {
        ALint source = sample->get_source();
        ALint result;
        alGetSourcei( source, AL_SOURCE_STATE, &result );
        return (result == AL_STOPPED);
    }
#endif
    return true;
}

void SGSoundMgr::update_sample_config( SGSoundSample *sample, SGVec3d& position, SGVec3f& orientation, SGVec3f& velocity )
{
    unsigned int source = sample->get_source();
    alSourcefv( source, AL_POSITION, toVec3f(position).data() );
    alSourcefv( source, AL_VELOCITY, velocity.data() );
    alSourcefv( source, AL_DIRECTION, orientation.data() );
    testForError("position and orientation");

    alSourcef( source, AL_PITCH, sample->get_pitch() );
    alSourcef( source, AL_GAIN, sample->get_volume() );
    testForError("pitch and gain");

    if ( sample->has_static_data_changed() ) {
        alSourcef( source, AL_CONE_INNER_ANGLE, sample->get_innerangle() );
        alSourcef( source, AL_CONE_OUTER_ANGLE, sample->get_outerangle() );
        alSourcef( source, AL_CONE_OUTER_GAIN, sample->get_outergain() );
        testForError("audio cone");

        alSourcef( source, AL_MAX_DISTANCE, sample->get_max_dist() );
        alSourcef( source, AL_REFERENCE_DISTANCE,
                           sample->get_reference_dist() );
        testForError("distance rolloff");
    }
}


bool SGSoundMgr::load( const std::string &samplepath,
                       void** dbuf,
                       int *fmt,
                       size_t *sz,
                       int *frq,
                       int *block )
{
    if ( !is_working() )
        return false;

    unsigned int format;
    unsigned int blocksz;
    ALsizei size;
    ALsizei freq;
    ALfloat freqf;

    auto data = simgear::loadWAVFromFile(samplepath, format, size, freqf, blocksz);
    freq = (ALsizei)freqf;
    if (!data) {
        return false;
    }

    if (format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16) {
         free(data);
        SG_LOG(SG_IO, SG_DEV_ALERT, "Warning: STEREO files are not supported for 3D audio effects: " << samplepath);
        return false;
    }

    *dbuf = (void *)data;
    *fmt = (int)format;
    *block = (int)blocksz;
    *sz = (size_t)size;
    *frq = (int)freq;

    return true;
}

vector<std::string> SGSoundMgr::get_available_devices()
{
    vector<std::string> devices;
#ifdef ENABLE_SOUND
    const ALCchar *s;

    if (alcIsExtensionPresent(nullptr, "ALC_enumerate_all_EXT") == AL_TRUE) {
        // REVIEW: Memory Leak - 4,136 bytes in 1 blocks are still reachable
        s = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
    } else {
        s = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
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
#endif
    return devices;
}


bool SGSoundMgr::testForError(void *p, std::string s)
{
   if (p == nullptr) {
      SG_LOG( SG_SOUND, SG_ALERT, "Error: " << s);
      return true;
   }
   return false;
}


bool SGSoundMgr::testForError(std::string s, std::string name)
{
#ifdef ENABLE_SOUND
    ALenum error = alGetError();
    if (error != AL_NO_ERROR)  {
       SG_LOG( SG_SOUND, SG_ALERT, "AL Error (" << name << "): "
                                      << alGetString(error) << " at " << s);
       return true;
    }
#endif
    return false;
}

bool SGSoundMgr::testForALCError(std::string s)
{
#ifdef ENABLE_SOUND
    ALCenum error;
    error = alcGetError(d->_device);
    if (error != ALC_NO_ERROR) {
        SG_LOG( SG_SOUND, SG_ALERT, "ALC Error (sound manager): "
                                       << alcGetString(d->_device, error) << " at "
                                       << s);
        return true;
    }
#endif
    return false;
}

bool SGSoundMgr::is_working() const 
{
    return (d->_device != nullptr);
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
