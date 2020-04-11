// soundmgr_aeonwave.cxx -- Sound effect management class
//                          for the AeonWave 3D and 4D audio rendering engine.
//
// Sound manager initially written by David Findlay
// <david_j_findlay@yahoo.com.au> 2001
//
// C++-ified by Curtis Olson, started March 2001.
// Modified for the new SoundSystem by Erik Hofman, October 2009
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
// Copyright (C) 2009-2019 Erik Hofman <erik@ehofman.com>
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

#include <aax/aeonwave.hpp>

#include "soundmgr.hxx"
#include "sample_group.hxx"

#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>


// We keep track of the emitters ourselves.
using source_map = std::map< unsigned int, aax::Emitter >;

// The AeonWave class keeps track of the buffers, so use a reference instead.
using buffer_map = std::map< unsigned int, aax::Buffer& >;
using sample_group_map = std::map< std::string, SGSharedPtr<SGSampleGroup> >;

#ifndef NDEBUG
# define TRY(a)	if ((a) == 0) printf("%i: %s\n", __LINE__, d->_aax.strerror())
#else
# define TRY(a)	(a)
#endif
#define BLOCKSIZE_TO_SMP(a)		((a) > 1) ? (((a)-4)*2) : 1

class SGSoundMgr::SoundManagerPrivate
{
public:
    SoundManagerPrivate() = default;

    ~SoundManagerPrivate()
    {
        _sample_groups.clear();
    }

    void init() {
        _mtx = aax::Matrix64();
    }
    
    void update_pos_and_orientation()
    {
        SGVec3d sgv_at = _orientation.backTransform(-SGVec3d::e3());
        SGVec3d sgv_up = _orientation.backTransform(SGVec3d::e2());
        SGVec3d pos = SGVec3d::zeros();

        _mtx.set(pos.data(), toVec3f(sgv_at).data(), toVec3f(sgv_up).data());

        _absolute_pos = _base_pos;
    }
        
    aax::AeonWave _aax;
    aax::Matrix64 _mtx;

    SGVec3d _absolute_pos = SGVec3d::zeros();
    SGVec3d _base_pos = SGVec3d::zeros();
    SGQuatd _orientation = SGQuatd::zeros();

    float _sound_velocity = SPEED_OF_SOUND;

    unsigned int _buffer_id = 0;
    buffer_map _buffers;
    aax::Buffer nullBuffer;
    aax::Buffer& get_buffer(unsigned int id) {
        auto buffer_it = _buffers.find(id);
        if ( buffer_it != _buffers.end() )  return buffer_it->second;
        SG_LOG(SG_SOUND, SG_ALERT, "unknown buffer id requested.");
        return nullBuffer;
    }

    unsigned int _source_id = 0;
    source_map _sources;
    aax::Emitter nullEmitter;
    aax::Emitter& get_emitter(unsigned int id) {
        auto source_it = _sources.find(id);
        if ( source_it != _sources.end() )  return source_it->second;
        SG_LOG(SG_SOUND, SG_ALERT, "unknown source id requested.");
        return nullEmitter;
    }

    sample_group_map _sample_groups;
};


//
// Sound Manager
//

// constructor
SGSoundMgr::SGSoundMgr() {
    d.reset(new SoundManagerPrivate);
    d->_base_pos = SGVec3d::fromGeod(_geod_pos);
}

// destructor

SGSoundMgr::~SGSoundMgr() {

    if (is_working())
        stop();
}

// initialize the sound manager
void SGSoundMgr::init()
{
#ifdef ENABLE_SOUND
    if (is_working())
    {
        SG_LOG( SG_SOUND, SG_ALERT, "Oops, AeonWave sound manager is already initialized." );
        return;
    }

    SG_LOG( SG_SOUND, SG_INFO, "Initializing AeonWave sound manager" );

    d->_source_id = 0;
    d->_sources.clear();

    if (!_device_name.empty()) {
        // try non-default device
        d->_aax = aax::AeonWave(_device_name.c_str());
    }
    else
    {
        testForError(d->_aax, "Audio device not available, trying default.");
        d->_aax = aax::AeonWave(AAX_MODE_WRITE_STEREO);
        if (testForError(d->_aax, "Default audio device not available.") ) {
           return;
        }
    }

    d->init();

    if ( is_working() )
    {
        TRY( d->_aax.set(AAX_INITIALIZED) );
        TRY( d->_aax.set(AAX_PLAYING) );
        testForError("initialization");

        aax::dsp dsp = aax::dsp(d->_aax, AAX_VOLUME_FILTER);
        TRY( dsp.set(AAX_GAIN, 0.0f) );
        TRY( d->_aax.set(dsp) );

        dsp = aax::dsp(d->_aax, AAX_DISTANCE_FILTER);
        TRY( dsp.set(AAX_ISO9613_DISTANCE) );
        TRY( d->_aax.set(dsp) );

        dsp = aax::dsp(d->_aax, AAX_VELOCITY_EFFECT);
        TRY( dsp.set(AAX_DOPPLER_FACTOR, 1.0f) );
        TRY( dsp.set(AAX_SOUND_VELOCITY, _sound_velocity) );
        TRY( d->_aax.set(dsp) );
        testForError("scenery setup");

        _vendor = (const char *)d->_aax.info(AAX_VENDOR_STRING);
        _renderer = (const char *)d->_aax.info(AAX_RENDERER_STRING);
    }
    testForError("init");
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

    d->_buffer_id = 0;
    d->_buffers.clear();

    d->_source_id = 0;
    d->_sources.clear();

    if (is_working()) {
        _active = false;
        TRY( d->_aax.set(AAX_PROCESSED) );

        _renderer = "unknown";
        _vendor = "unknown";
    }
#endif
}

void SGSoundMgr::suspend()
{
#ifdef ENABLE_SOUND
    if (is_working()) {
        for (auto current : d->_sample_groups ) {
            current.second->stop();
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
    if ( is_working() && _active ) {
        if (_changed) {
            d->update_pos_and_orientation();
        }

        for ( auto current : d->_sample_groups ) {
            current.second->update(dt);
        }

        if (_changed) {
            aax::dsp dsp = d->_aax.get(AAX_VOLUME_FILTER);
            TRY( dsp.set(AAX_GAIN, _volume) );
            TRY( d->_aax.set(dsp) );

            aax::Matrix64 mtx = d->_mtx;
            mtx.inverse();
            TRY( d->_aax.sensor_matrix(mtx) );

            SGQuatd hlOr = SGQuatd::fromLonLat( _geod_pos );
            SGVec3d velocity = SGVec3d::zeros();
            if ( _velocity[0] || _velocity[1] || _velocity[2] ) {
                velocity = hlOr.backTransform(_velocity*SG_FEET_TO_METER);
            }
            aax::Vector vel( toVec3f(velocity).data() );
            TRY( d->_aax.sensor_velocity(vel) );

            if (fabsf(_sound_velocity - d->_sound_velocity) > 10.0f) {
                d->_sound_velocity = _sound_velocity;

                dsp = aax::dsp(d->_aax, AAX_VELOCITY_EFFECT);
                TRY( dsp.set(AAX_SOUND_VELOCITY, _sound_velocity) );
                TRY( d->_aax.set(dsp) );
            }

            testForError("update");
            _changed = false;
        }
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
            SGSampleGroup *sgrp = new SGSampleGroup(this, refname);
            add( sgrp, refname );
            return sgrp;
        }
        else 
            return nullptr;
    }

    return sample_grp_it->second.get();
}


void SGSoundMgr::set_volume( float v )
{
    _volume = v;
    SG_CLAMP_RANGE(_volume, 0.0f, 1.0f);
    _changed = true;
}

// Get an unused source id
unsigned int SGSoundMgr::request_source()
{
    unsigned int id = d->_source_id++;
    d->_sources.insert( std::make_pair(id, aax::Emitter(AAX_ABSOLUTE)) );
    return id;
}

// Free up a source id
void SGSoundMgr::release_source( unsigned int source )
{
    auto source_it = d->_sources.find(source);
    if ( source_it != d->_sources.end() )
    {
        aax::Emitter& emitter = source_it->second;
        TRY( emitter.set(AAX_PROCESSED) );
        TRY( d->_aax.remove(emitter) );
        TRY( emitter.remove_buffer() );
        d->_sources.erase(source_it);
    }
    else
       SG_LOG( SG_SOUND, SG_WARN, "Sound manager: Trying to release an untracked source id!\n");
}

unsigned int  SGSoundMgr::request_buffer(SGSoundSample *sample)
{
    unsigned int bufid = NO_BUFFER;
#ifdef ENABLE_SOUND
    if ( !sample->is_valid_buffer() ) {
        // sample was not yet loaded or removed again
        std::string sample_name = sample->get_sample_name();

        aax::Buffer& buf = d->_aax.buffer(sample_name);
        if (sample->is_file() && !buf) {
            SG_LOG(SG_SOUND, SG_ALERT,
                   "Unable to create buffer: " << sample_name);
            sample->set_buffer( SGSoundMgr::FAILED_BUFFER );
            return FAILED_BUFFER;
        }

        bufid = d->_buffer_id++;
        d->_buffers.insert( {bufid, buf} );

        if ( !sample->is_file() ) {
            enum aaxFormat format = AAX_FORMAT_NONE;
            switch( sample->get_format() )
            {
            case SG_SAMPLE_MONO16:
            case SG_SAMPLE_STEREO16:
                format = AAX_PCM16S;
                break;
            case SG_SAMPLE_MONO8:
            case SG_SAMPLE_STEREO8:
                format = AAX_PCM8S;
                break;
            case SG_SAMPLE_MULAW:
                format = AAX_MULAW;
                break;
            case SG_SAMPLE_ADPCM:
                format = AAX_IMA4_ADPCM;
                break;
            default:
                SG_LOG(SG_SOUND, SG_ALERT, "unsupported audio format");
                return bufid;
            }

            unsigned int no_samples = sample->get_no_samples();
            unsigned int no_tracks = sample->get_no_tracks();
            unsigned int frequency = sample->get_frequency();
            buf.set(d->_aax, no_samples, no_tracks, format);
            TRY( buf.set(AAX_FREQUENCY, frequency) );
            TRY( buf.fill(sample->get_data()) );

            if (format == AAX_IMA4_ADPCM) {
                size_t blocksz = sample->get_block_align();
                TRY( buf.set(AAX_BLOCK_ALIGNMENT, BLOCKSIZE_TO_SMP(blocksz)) );
            }
        }

        sample->set_buffer(bufid);
        sample->set_block_align( buf.get(AAX_BLOCK_ALIGNMENT) );
        sample->set_frequency( buf.get(AAX_FREQUENCY) );
        sample->set_no_samples( buf.get(AAX_NO_SAMPLES) );
        sample->set_no_tracks( buf.get(AAX_TRACKS) );

        enum aaxFormat fmt = aaxFormat( buf.get(AAX_FORMAT) );
        sample->set_bits_sample( aaxGetBitsPerSample(fmt) );

        bool c = (fmt == AAX_MULAW || fmt == AAX_IMA4_ADPCM);
        sample->set_compressed(c);
    }
    else {
        bufid = sample->get_buffer();
    }
#endif
    return bufid;
}

void SGSoundMgr::release_buffer(SGSoundSample *sample)
{
    if ( !sample->is_queue() )
    {
        unsigned int buffer = sample->get_buffer();
        auto buffer_it = d->_buffers.find(buffer);
        if ( buffer_it != d->_buffers.end() )
        {
            sample->no_valid_buffer();
#ifdef ENABLE_SOUND
            aax::Buffer& buffer = buffer_it->second;
            d->_aax.destroy(buffer);
#endif
            d->_buffers.erase(buffer_it);
            testForError("release buffer");
        }
    }
}

void SGSoundMgr::sample_suspend( SGSoundSample *sample )
{
    if ( sample->is_valid_source() && sample->is_playing() ) {
        aax::Emitter& emitter = d->get_emitter(sample->get_source());
        TRY( emitter.set(AAX_SUSPENDED) );
    }
}

void SGSoundMgr::sample_resume( SGSoundSample *sample )
{
    if ( sample->is_valid_source() && sample->is_playing() ) {
        aax::Emitter& emitter = d->get_emitter(sample->get_source());
        TRY( emitter.set(AAX_PLAYING) );
    }
}

void SGSoundMgr::sample_init( SGSoundSample *sample )
{
#ifdef ENABLE_SOUND
    sample->set_source(request_source());
#endif
}

void SGSoundMgr::sample_play( SGSoundSample *sample )
{
#ifdef ENABLE_SOUND
    aax::Emitter& emitter = d->get_emitter(sample->get_source());

    if ( !sample->is_queue() ) {
        unsigned int bufid = request_buffer(sample);
        if (bufid == SGSoundMgr::FAILED_BUFFER ||
            bufid == SGSoundMgr::NO_BUFFER)
        {
            release_source(sample->get_source());
            return;
        }

        aax::Buffer& buffer = d->get_buffer(bufid);
        if (buffer) {
            TRY( emitter.add(buffer) );
        } else
            SG_LOG( SG_SOUND, SG_ALERT, "No such buffer!");
    }

    aax::dsp dsp = emitter.get(AAX_DISTANCE_FILTER);
    TRY( dsp.set(AAX_ROLLOFF_FACTOR, 0.3f) );
    TRY( dsp.set(AAX_ISO9613_DISTANCE) );
    TRY( emitter.set(dsp) );

    TRY( emitter.set(AAX_LOOPING, sample->is_looping()) );
    TRY( emitter.set(AAX_POSITION, AAX_ABSOLUTE) );

    TRY( d->_aax.add(emitter) );
    TRY( emitter.set(AAX_INITIALIZED) );
    TRY( emitter.set(AAX_PLAYING) );
#endif
}

void SGSoundMgr::sample_stop( SGSoundSample *sample )
{
    if ( sample->is_valid_source() ) {
        int source = sample->get_source();
    
        bool stopped = is_sample_stopped(sample);
        if ( sample->is_looping() && !stopped) {
#ifdef ENABLE_SOUND
            aax::Emitter& emitter = d->get_emitter(source);
            TRY( emitter.set(AAX_PROCESSED) );
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
        unsigned int source = sample->get_source();
        if ( sample->is_playing() ) {
            aax::Emitter& emitter = d->get_emitter(source);
            TRY( emitter.set(AAX_PROCESSED) );
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
    assert(sample->is_valid_source());
    aax::Emitter& emitter = d->get_emitter(sample->get_source());
    int result = emitter.state();
    return (result == AAX_PROCESSED);
#else
    return true;
#endif
}

void SGSoundMgr::update_sample_config( SGSoundSample *sample, SGVec3d& position, SGVec3f& orientation, SGVec3f& velocity )
{
    aax::Emitter& emitter = d->get_emitter(sample->get_source());
    aax::dsp dsp;

    if (emitter != d->nullEmitter)
    {
        aax::Vector64 pos = position.data();
        aax::Vector ori = orientation.data();
        aax::Vector vel = velocity.data();

        aax::Matrix64 mtx(pos, ori);
        TRY( emitter.matrix(mtx) );
        TRY( emitter.velocity(vel) );

        dsp = emitter.get(AAX_VOLUME_FILTER);
        TRY( dsp.set(AAX_GAIN, sample->get_volume()) );
        TRY( emitter.set(dsp) );

        dsp = emitter.get(AAX_PITCH_EFFECT);
        TRY( dsp.set(AAX_PITCH, sample->get_pitch()) );
        TRY( emitter.set(dsp) );

        if ( sample->has_static_data_changed() ) {
            dsp = emitter.get(AAX_DIRECTIONAL_FILTER);
            TRY( dsp.set(AAX_INNER_ANGLE, sample->get_innerangle(), AAX_DEGREES) );
            TRY( dsp.set(AAX_OUTER_ANGLE, sample->get_outerangle(), AAX_DEGREES) );
            TRY( dsp.set(AAX_OUTER_GAIN, sample->get_outergain()) );
            TRY( emitter.set(dsp) );

            dsp = emitter.get(AAX_DISTANCE_FILTER);
            TRY( dsp.set(AAX_REF_DISTANCE, sample->get_reference_dist()) );
            TRY( dsp.set(AAX_MAX_DISTANCE, sample->get_max_dist()) );
            TRY( dsp.set(AAX_RELATIVE_HUMIDITY, sample->get_humidity()) );
            TRY( dsp.set(AAX_TEMPERATURE, sample->get_temperature(),
                                          AAX_DEGREES_CELSIUS) );
            TRY( emitter.set(dsp) );
       }
    }
    else {
        SG_LOG( SG_SOUND, SG_ALERT, "Error: source: " << sample->get_source() << " not found");
    }
}


vector<std::string> SGSoundMgr::get_available_devices()
{
    vector<std::string> devices;
#ifdef ENABLE_SOUND
    std::string on = " on ";
    std::string colon = ": ";
    while (const char* be = d->_aax.drivers()) {
        while (const char* r = d->_aax.devices()) {
            while (const char* i = d->_aax.interfaces()) {
                std::string name = be;
                if (*i && *r) name += on + r + colon + i;
                else if (*r) name += on + r;
                else if (*i) name += colon + i;

                devices.push_back( name );
            }
        }
    }
    testForError("get_available_devices");
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
    enum aaxErrorType error = aax::error_no();
    if (error != AAX_ERROR_NONE) {
       SG_LOG( SG_SOUND, SG_ALERT, "AeonWave Error (" << name << "): "
                                      << aax::strerror(error) << " at " << s);
       return true;
    }
#endif
    return false;
}

bool SGSoundMgr::is_working() const 
{
    return ((const void*)d->_aax != nullptr ? true : false);
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
    aaxVec3f at, up;
    aaxVec3d pos;
    d->_mtx.get(pos, at, up);
    return SGVec3f( at );
}

bool SGSoundMgr::load( const std::string &samplepath,
                       void **dbuf,
                       int *fmt,
                       size_t *sz,
                       int *frq,
                       int *block )
{
    return true;
}
