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
#include <stdio.h>

#include <iostream>
#include <algorithm>
#include <cstring>
#include <cassert>

#include <boost/foreach.hpp>

#include "soundmgr_aeonwave.hxx"
#include "sample_group.hxx"

#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>


typedef std::map < unsigned int,AAX::Emitter > source_map;
typedef source_map::iterator source_map_iterator;
typedef source_map::const_iterator  const_source_map_iterator;

typedef std::map < unsigned int,AAX::Buffer& > buffer_map;
typedef buffer_map::iterator buffer_map_iterator;
typedef buffer_map::const_iterator  const_buffer_map_iterator;

class SGSoundMgr::SoundManagerPrivate
{
public:
    SoundManagerPrivate() :
        _buffer_id(0),
        _source_id(0),
        _base_pos(SGVec3d::zeros()),
    {
    }
    
    void init()
    {
        _mtx64 = AAX::Matrix64();
    }
    
    void update_pos_and_orientation()
    {
        SGVec3d sgv_at = _orientation.backTransform(-SGVec3d::e3());
        SGVec3d sgv_up = _orientation.backTransform(SGVec3d::e2());
        _mtx64.set(_base_pos, sgv_at, sgv_up);
    }
        
    AAX::AeonWave _aax;
    AAX::Matrix64 _mtx64;

    SGVec3d _base_pos;
    SGQuatd _orientation;

    unsigned int _buffer_id;
    buffer_map _buffers;

    unsigned int _source_id;
    source_map _sources;
    AAX::Emitter& get_source(unsigned int id) {
        source_map_iterator source_it = _d->_sources.find(id);
        if ( source_it != d->_sources.end() )  return source_it->second;
        SG_LOG(SG_SOUND, SG_ALERT, "unknown source id requested.");
        return NULL;
    }

    sample_group_map _sample_groups;

    std::vector<std::string> _devices;
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
void SGSoundMgr::init()
{
#ifdef ENABLE_SOUND
    if (is_working())
    {
        SG_LOG( SG_SOUND, SG_ALERT, "Oops, AeonWave sound manager is already initialized." );
        return;
    }

    SG_LOG( SG_SOUND, SG_INFO, "Initializing AeonWave sound manager" );

    _source_id = 0;
    d->_sources.clear();

    AAX::AeonWave aax;
    AAX::DSP dsp;

    const char* devname = _device_name.c_str();
    if (_device_name == "")
        devname = NULL; // use default device
    else
    {
        // try non-default device
        aax = AAX::AeonWave(devname);
    }

    if ((!devname)||(testForError(device, "Audio device not available, trying default.")) ) {
        aax = AAX::AeonWave(AAX_MODE_WRITE_STEREO);
        if (testForError(aax, "Default audio device not available.") ) {
           return;
        }
    }

    d->_aax = aax;
    d->init();

    d->_aax.set(AAX_INITIALIZED);

    dsp = AAX::DSP(d->_aax, AAX_VOLUME_FILTER);
    dsp.set(AAX_GAIN, 0.0f);
    d->_aax.set(dsp);

    dsp = AAX::DSP(d->_aax, AAX_DISTANCE_FILTER);
    dsp.set(AAX_AL_INVERSE_DISTANCE_CLAMPED);
    d->_aax.set(dsp);

    dsp = AAX::DSP(d->_aax, AAX_VELOCITY_EFFECT);
    dsp.set(AAX_DOPPLER_FACTOR, 1.0f);
    dsp.set(AAX_SOUND_VELOCITY, 340.3f);
    d->_aax.set(dsp);

    testForError("listener initialization");

    _vendor = d->_aax.info(AAX_VENDOR_STRING);
    _renderer = d->_aax.info(AAX_RENDERER_STRING);
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
                
        sample_group_map_iterator sample_grp_current = d->_sample_groups.begin();
        sample_group_map_iterator sample_grp_end = d->_sample_groups.end();
        for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
            SGSampleGroup *sgrp = sample_grp_current->second;
            sgrp->activate();
        }
    }
#endif
}

// stop the sound manager
void SGSoundMgr::stop()
{
#ifdef ENABLE_SOUND
    // first stop all sample groups
    sample_group_map_iterator sample_grp_current = d->_sample_groups.begin();
    sample_group_map_iterator sample_grp_end = d->_sample_groups.end();
    for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
        SGSampleGroup *sgrp = sample_grp_current->second;
        sgrp->stop();
    }

    _bufefr_id = 0;
    d->_buffers.clear();

    _source_id = 0;
    d->_sources.clear();

    if (is_working()) {
        _active = false;
        d->_aax.set(AAX_STOPPED);

        _renderer = "unknown";
        _vendor = "unknown";
    }
#endif
}

void SGSoundMgr::suspend()
{
#ifdef ENABLE_SOUND
    if (is_working()) {
        sample_group_map_iterator sample_grp_current = d->_sample_groups.begin();
        sample_group_map_iterator sample_grp_end = d->_sample_groups.end();
        for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
            SGSampleGroup *sgrp = sample_grp_current->second;
            sgrp->stop();
        }
        _active = false;
    }
#endif
}

void SGSoundMgr::resume()
{
#ifdef ENABLE_SOUND
    if (is_working()) {
        sample_group_map_iterator sample_grp_current = d->_sample_groups.begin();
        sample_group_map_iterator sample_grp_end = d->_sample_groups.end();
        for ( ; sample_grp_current != sample_grp_end; ++sample_grp_current ) {
            SGSampleGroup *sgrp = sample_grp_current->second;
            sgrp->resume();
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
            AAX::DSP dsp = d->_aax.get(AAX_VOLUME_FILTER);
            dsp.set(AAX_GAIN, _volume);
            d->_aax.set(dsp);

#if 0
            // TODO: altitude dependent
            dsp = d->_aax.get(AAX_VELOCITY_EFFECT);
            dsp.set(AAX_SOUND_VELOCITY, 340.3f);
            d->_aax.set(dsp);
#endif

            SGQuatd hlOr = SGQuatd::fromLonLat( _geod_pos );
            SGVec3d velocity = SGVec3d::zeros();
            if ( _velocity[0] || _velocity[1] || _velocity[2] ) {
                velocity = hlOr.backTransform(_velocity*SG_FEET_TO_METER);
            }

            if ( _bad_doppler ) {
                velocity *= 100.0f;
            }

            d->_aax.sensor_velocity(AAX::Vector(toVec3f(velocity).data()));
            d->_aax.sensor_matrix(d->_mtx64);

            testForError("update");
            _changed = false;
        }
    }
#endif
}

// add a sample group, return true if successful
bool SGSoundMgr::add( SGSampleGroup *sgrp, const std::string& refname )
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
bool SGSoundMgr::remove( const std::string &refname )
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
bool SGSoundMgr::exists( const std::string &refname ) {
    sample_group_map_iterator sample_grp_it = d->_sample_groups.find( refname );
    return ( sample_grp_it != d->_sample_groups.end() );
}


// return a pointer to the SGSampleGroup if the specified sound exists
// in the sound manager system, otherwise return NULL
SGSampleGroup *SGSoundMgr::find( const std::string &refname, bool create ) {
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
unsigned int SGSoundMgr::request_source()
{
    unsigned int id = d->_source_id++;
    d->_sources.insert( std::pair<id,AAX::Emitter()> );
    return id;
}

// Free up a source id
void SGSoundMgr::release_source( unsigned int source )
{
    source_map_iterator source_it = _d->_sources.find(source);
    if ( source_it != d->_sources.end() )
    {
        AAX::Emitter& emitter = source_it->second;
        emitter.set(AAX_STOPPED);
        emitter.remove_buffer();
        d->_sources.erase(source_it);
    }
    else
       SG_LOG( SG_SOUND, SG_WARN, "Sound manager: Trying to release an untracked source id!\n");
}

unsigned int  SGSoundMgr::request_buffer(SGSoundSample *sample)
{
    unsigned int buffer = NO_BUFFER;
#ifdef ENABLE_SOUND
    if ( !sample->is_valid_buffer() ) {
        // sample was not yet loaded or removed again
        std::string sample_name = sample->get_sample_name();
        void *sample_data = NULL;

        // sample name was not found in the buffer cache.
        if ( sample->is_file() ) {
            AAX::Buffer& buf = d->_aax->buffer(sample_name);
            if (!buf) {
              SG_LOG(SG_SOUND, SG_ALERT,
                    "failed to load sound file: " << sample_name);
              sample->set_buffer( SGSoundMgr::FAILED_BUFFER );
              return FAILED_BUFFER;
            }

            buffer = d->_buffer_id++;
            d->_buffers.insert( std::pair<buffer,buf> );

            sample->set_block_align( buf.get(AAX_BLOCK_ALIGNMENT) );
            sample->set_frequency( buf.get(AAX_FREQUENCY) );
            sample->set_no_samples( buf.get(AAX_NO_SAMPLES) );
            sample->set_no_tracks( buf.get(AAX_TRACKS) );

            enum aaxFormat fmt = buf.get(AAX_FORMAT);
            sample->set_bits_sample( aaxGetBitsPerSample(fmt) );

            bool c = (fmt == AAX_MULAW || fmt == AAX_IMA4_ADPCM);
            sample->set_compressed(c);

        } else {
            enum aaxFormat format = AAX_FORMAT_NONE;
            switch( sample->get_format() )
            {
            case SG_SAMPLE_MONO16:
                format = AAX_FORMAT_PCM16;
                break;
            case SG_SAMPLE_MONO8:
                format = AAX_FORMAT_PCM8;
                break;
            case SG_SAMPLE_MULAW:
                format = AAX_FORMAT_MULAW;
                break;
            case SG_SAMPLE_ADPCM:
                format = AAX_FORMAT_IMA4_ADPCM;
                break;
            default:
                SG_LOG(SG_SOUND, SG_ALERT, "unsupported audio format");
                return buffer;
            }

            AAX::Buffer& buf = d->_aax->buffer(sample_name);
            if ( buf ) {
                unsigned int no_samples = sample->get_no_samples();

                buf.set(d->_aax, no_samples, 1, format);
                buf.set( AAX_FREQUENCY, sample->get_frequency() );
                buf.fill( sample->get_data() );

                if (format == AAX_FORMAT_IMA4_ADPCM) {
                    size_t samples_block = BLOCKSIZE_TO_SMP( sample->get_block_align() );
                    buf.set( AAX_BLOCK_ALIGNMENT, samples_block );
                }

                buffer = d->_buffer_id++;
                d->_buffers.insert( std::pair<buffer,buf> );
                sample->set_buffer(buffer);
            }
        }
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
        unsigned int buffer = sample->get_buffer();
        buffer_map_iterator buffer_it = _d->_buffers.find(buffer);
        if ( buffer_it != d->_buffers.end() )
        {
            sample->no_valid_buffer();
#ifdef ENABLE_SOUND
            AAX::Buffer& buffer = buffer_it->second;
            d->_aax->destroy(buffer);
#endif
            d->_buffers.erase(buffer_it);
            testForError("release buffer");
        }
    }
}

void SGSoundMgr::sample_suspend( SGSoundSample *sample )
{
    if ( sample->is_valid_source() && sample->is_playing() ) {
        AAX::Emitter& emitter = d->get_source(sample->get_source());
        emitter.set(AAX_PASUED);
    }
}

void SGSoundMgr::sample_resume( SGSoundSample *sample )
{
    if ( sample->is_valid_source() && sample->is_playing() ) {
        AAX::Emitter& emitter = d->get_source(sample->get_source());
        emitter.set(AAX_PLAYING);
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
    AAX::Emitter& emitter = d->get_source(sample->get_source());

    if ( !sample->is_queue() ) {
        emitter.add(request_buffer(sample));
    }

    AAX::DSP dsp = emitter.get(AAX_DISTANCE_FILTER);
    dsp.set(AAX_ROLLOFF_FACTOR, 0.3f);
    emitter.set(dsp);

    emitter.set(AAX_LOOPING, sample->is_looping());
    emitter.set(AAX_POSITION, AAX_ABSOLUTE);
    emitter.set(AAX_PLAYING);
#endif
}

void SGSoundMgr::sample_stop( SGSoundSample *sample )
{
    if ( sample->is_valid_source() ) {
        int source = sample->get_source();
    
        bool stopped = is_sample_stopped(sample);
        if ( sample->is_looping() && !stopped) {
#ifdef ENABLE_SOUND
            AAX::Emitter& emitter = d->get_source(source);
            emitter.set(AAX_STOPPED);
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
            AAX::Emitter& emitter = d->get_source(source);
            emitter.set(AAX_STOPPED);
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
    AAX::Emitter& emitter = d->get_source(sample->get_source());
    int result = emitter.state();
    return (result == AAX_STOPPED);
#else
    return true;
#endif
}

void SGSoundMgr::update_sample_config( SGSoundSample *sample, SGVec3d& position, SGVec3f& orientation, SGVec3f& velocity )
{
    AAX::Emitter& emitter = d->get_source(sample->get_source());
    AAX::DSP dsp;

    d->_mtx64.set(position.data(), orientation.data());
    emitter.velocity(velocity.data());
    emitter.matrix(d->_mtx64);

    dsp = emitter.get(AAX_VOLUME_FILTER);
    dsp.set(AAX_GAIN, sample->get_volume());
    emitter.set(dsp);

    dsp = emitter.get(AAX_PITCH_EFFECT);
    dsp.set(AAX_PITCH, sample->get_pitch());
    emitter.set(dsp);

    if ( sample->has_static_data_changed() ) {
        dsp = emitter.get(AAX_ANGULAR_FILTER);
        dsp.set(AAX_INNER_ANGLE, sample->get_innerangle());
        dsp.set(AAX_OUTER_ANGLE, sample->get_outerangle());
        dsp.set(AAX_OUTER_GAIN, sample->get_outergain());
        emitter.set(dsp);

        dsp = emitter.get(AAX_DISTANCE_FILTER);
        dsp.set(AAX_REF_DISTANCE, sample->get_reference_dist());
        sp.set(AAX_MAX_DISTANC, sample->get_max_dist();
        emitter.set(dsp);
    }
}


vector<const char*> SGSoundMgr::get_available_devices()
{
    vector<const char*> devices;
#ifdef ENABLE_SOUND
    std::string on = " on ";
    std::string colon = ": ";
    while (std::string d = d->_aax.drivers()) {
        while (std::string r = d->_aax.devices()) {
            while (std::string i = d->_aax.interfaces()) {
                std::string name;
                if (!i.empty() && !r.empty()) name = d + on + r + colon + i;
                else if (!r.empty()) name = d + on + r;
                else if (!i.empty) name = d + colon + i;
                else name = d;

                d->_devices.push_back(name);
                devices.push_back(name.c_str());
            }
        }
    }
#endif
    return devices;
}


bool SGSoundMgr::testForError(void *p, std::string s)
{
   if (p == NULL) {
      SG_LOG( SG_SOUND, SG_ALERT, "Error: " << s);
      return true;
   }
   return false;
}


bool SGSoundMgr::testForError(std::string s, std::string name)
{
#ifdef ENABLE_SOUND
    enum aaxErrorType error = d->_aax.error_no();
    if (error != AAX_ERROR_NONE) {
       SG_LOG( SG_SOUND, SG_ALERT, "AL Error (" << name << "): "
                                      << d->_aax.error(error) << " at " << s);
       return true;
    }
#endif
    return false;
}

bool SGSoundMgr::testForALCError(std::string s)
{
#ifdef ENABLE_SOUND
    enum aaxErrorType error = d->_aax.error_no();
    if (error != AAX_ERROR_NONE) {
        SG_LOG( SG_SOUND, SG_ALERT, "Error (sound manager): "
                                       << d->_aax.error(error) << " at "
                                       << s);
        return true;
    }
#endif
    return false;
}

bool SGSoundMgr::is_working() const 
{
    return (d->_aax != NULL);
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
    aaxVec3d pos, at, up;
    d->_mtx64.get(pos, at, up);
    return pos;
}

void SGSoundMgr::set_position( const SGVec3d& pos, const SGGeod& pos_geod )
{
    d->_base_pos = pos; _geod_pos = pos_geod; _changed = true;
}

SGVec3f SGSoundMgr::get_direction() const
{
    aaxVec3d pos, at, up;
    d->_mtx64.get(pos, at, up);
    return at;
}
