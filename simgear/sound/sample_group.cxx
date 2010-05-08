// sample_group.cxx -- Manage a group of samples relative to a base position
//
// Written for the new SoundSystem by Erik Hofman, October 2009
//
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

#include <simgear/compiler.h>

#include "soundmgr_openal.hxx"
#include "sample_group.hxx"

bool isNaN(float *v) {
   return (isnan(v[0]) || isnan(v[1]) || isnan(v[2]));
}

SGSampleGroup::SGSampleGroup () :
    _smgr(NULL),
    _refname(""),
    _active(false),
    _changed(false),
    _pause(false),
    _volume(1.0),
    _tied_to_listener(false),
    _velocity(SGVec3d::zeros()),
    _orientation(SGQuatd::zeros())
{
    _samples.clear();
}

SGSampleGroup::SGSampleGroup ( SGSoundMgr *smgr, const string &refname ) :
    _smgr(smgr),
    _refname(refname),
    _active(false), 
    _changed(false),
    _pause(false),
    _volume(1.0),
    _tied_to_listener(false),
    _velocity(SGVec3d::zeros()),
    _orientation(SGQuatd::zeros())
{
    _smgr->add(this, refname);
    _samples.clear();
}

SGSampleGroup::~SGSampleGroup ()
{
    _active = false;

    sample_map_iterator sample_current = _samples.begin();
    sample_map_iterator sample_end = _samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        SGSoundSample *sample = sample_current->second;

        if ( sample->is_valid_source() && sample->is_playing() ) {
            sample->no_valid_source();
            _smgr->release_source( sample->get_source() );
            _smgr->release_buffer( sample );
        }
    }

    _smgr = 0;
}

void SGSampleGroup::update( double dt ) {

    if ( !_active || _pause ) return;

    testForALError("start of update!!\n");

    // Delete any OpenAL buffers that might still be in use.
    unsigned int size = _removed_samples.size();
    for (unsigned int i=0; i<size; ) {
        SGSoundSample *sample = _removed_samples[i];
        ALint result = AL_STOPPED;

        if ( sample->is_valid_source() ) {
            if ( sample->is_looping() ) {
                sample->no_valid_source();
                _smgr->release_source( sample->get_source() );
            }
            else
                alGetSourcei( sample->get_source(), AL_SOURCE_STATE, &result );
        }

        if ( result == AL_STOPPED ) {
            ALuint buffer = sample->get_buffer();
            alDeleteBuffers( 1, &buffer );
            testForALError("buffer remove");
            _removed_samples.erase( _removed_samples.begin()+i );
            size--;
            continue;
        }
        i++;
    }

    // Update the position and orientation information for all samples.
    if ( _changed || _smgr->has_changed() ) {
        update_pos_and_orientation();
        _changed = false;
    }

    sample_map_iterator sample_current = _samples.begin();
    sample_map_iterator sample_end = _samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        SGSoundSample *sample = sample_current->second;

        if ( !sample->is_valid_source() && sample->is_playing() ) {
            //
            // a request to start playing a sound has been filed.
            //
            if ( _smgr->request_buffer(sample) == SGSoundMgr::NO_BUFFER )
                continue;

            // start playing the sample
            ALuint buffer = sample->get_buffer();
            ALuint source = _smgr->request_source();
            if (alIsSource(source) == AL_TRUE && alIsBuffer(buffer) == AL_TRUE)
            {
                sample->set_source( source );
                
                alSourcei( source, AL_BUFFER, buffer );
                testForALError("assign buffer to source");

                sample->set_source( source );
                update_sample_config( sample );

                ALboolean looping = sample->is_looping() ? AL_TRUE : AL_FALSE;
                alSourcei( source, AL_LOOPING, looping );
                alSourcef( source, AL_ROLLOFF_FACTOR, 0.3 );
                alSourcei( source, AL_SOURCE_RELATIVE, AL_TRUE );
                alSourcePlay( source );
                testForALError("sample play");
            } else {
                if (alIsBuffer(buffer) == AL_FALSE) 
                   SG_LOG( SG_GENERAL, SG_ALERT, "No such buffer!\n");
                // sample->no_valid_source();
                // sadly, no free source available at this time
            }

        } else if ( sample->is_valid_source() && sample->has_changed() ) {
            if ( !sample->is_playing() ) {
                // a request to stop playing the sound has been filed.

                sample->stop();
                sample->no_valid_source();
                _smgr->release_source( sample->get_source() );
            } else if ( _smgr->has_changed() ) {
                update_sample_config( sample );
            }

        } else if ( sample->is_valid_source() ) {
            // check if the sound has stopped by itself

            unsigned int source = sample->get_source();
            int result;

            alGetSourcei( source, AL_SOURCE_STATE, &result );
            if ( result == AL_STOPPED ) {
                // sample is stoped because it wasn't looping
                sample->stop();
                sample->no_valid_source();
                _smgr->release_source( source );
                _smgr->release_buffer( sample );
                remove( sample->get_sample_name() );
            }
        }
        testForALError("update");
    }
}

// add a sound effect, return true if successful
bool SGSampleGroup::add( SGSharedPtr<SGSoundSample> sound, const string& refname ) {

    sample_map_iterator sample_it = _samples.find( refname );
    if ( sample_it != _samples.end() ) {
        // sample name already exists
        return false;
    }

    _samples[refname] = sound;
    return true;
}


// remove a sound effect, return true if successful
bool SGSampleGroup::remove( const string &refname ) {

    sample_map_iterator sample_it = _samples.find( refname );
    if ( sample_it == _samples.end() ) {
        // sample was not found
        return false;
    }

    if ( sample_it->second->is_valid_buffer() )
        _removed_samples.push_back( sample_it->second );
    _samples.erase( sample_it );

    return true;
}


// return true of the specified sound exists in the sound manager system
bool SGSampleGroup::exists( const string &refname ) {
    sample_map_iterator sample_it = _samples.find( refname );
    if ( sample_it == _samples.end() ) {
        // sample was not found
        return false;
    }

    return true;
}


// return a pointer to the SGSoundSample if the specified sound exists
// in the sound manager system, otherwise return NULL
SGSoundSample *SGSampleGroup::find( const string &refname ) {
    sample_map_iterator sample_it = _samples.find( refname );
    if ( sample_it == _samples.end() ) {
        // sample was not found
        return NULL;
    }

    return sample_it->second;
}


void
SGSampleGroup::stop ()
{
    _pause = true;
    sample_map_iterator sample_current = _samples.begin();
    sample_map_iterator sample_end = _samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        SGSoundSample *sample = sample_current->second;

        if ( sample->is_valid_source() ) {
            ALint source = sample->get_source();
            if ( sample->is_playing() ) {
                alSourceStop( source );
                alSourcei( source, AL_BUFFER, 0 );
            }
            _smgr->release_source( source );
            sample->no_valid_source();
        }

        if (sample->is_valid_buffer() ) {
            _smgr->release_buffer( sample );
            sample->no_valid_buffer();
        }
    }
    testForALError("stop");
}

// stop playing all associated samples
void
SGSampleGroup::suspend ()
{
    if (_active && _pause == false) {
        _pause = true;
        sample_map_iterator sample_current = _samples.begin();
        sample_map_iterator sample_end = _samples.end();
        for ( ; sample_current != sample_end; ++sample_current ) {
            SGSoundSample *sample = sample_current->second;

            if ( sample->is_valid_source() && sample->is_playing() ) {
                alSourcePause( sample->get_source() );
            }
        }
        testForALError("suspend");
    }
}

// resume playing all associated samples
void
SGSampleGroup::resume ()
{
    if (_active && _pause == true) {
        sample_map_iterator sample_current = _samples.begin();
        sample_map_iterator sample_end = _samples.end();
        for ( ; sample_current != sample_end; ++sample_current ) {
            SGSoundSample *sample = sample_current->second;

            if ( sample->is_valid_source() && sample->is_playing() ) {
                alSourcePlay( sample->get_source() );
            }
        }
        testForALError("resume");
        _pause = false;
    }
}


// tell the scheduler to play the indexed sample in a continuous loop
bool SGSampleGroup::play( const string &refname, bool looping = false ) {
    SGSoundSample *sample = find( refname );

    if ( sample == NULL ) {
        return false;
    }

    sample->play( looping );
    return true;
}


// return true of the specified sound is currently being played
bool SGSampleGroup::is_playing( const string& refname ) {
    SGSoundSample *sample = find( refname );

    if ( sample == NULL ) {
        return false;
    }

    return ( sample->is_playing() ) ? true : false;
}

// immediate stop playing the sound
bool SGSampleGroup::stop( const string& refname ) {
    SGSoundSample *sample  = find( refname );

    if ( sample == NULL ) {
        return false;
    }

    sample->stop();
    return true;
}

void SGSampleGroup::set_volume( float vol )
{
    if (vol > _volume*1.01 || vol < _volume*0.99) {
        _volume = vol;
        if (_volume < 0.0) _volume = 0.0;
        if (_volume > 1.0) _volume = 1.0;
        _changed = true;
    }
}

// set the source position and orientation of all managed sounds
void SGSampleGroup::update_pos_and_orientation() {
 
    SGVec3d position = SGVec3d::fromGeod(_base_pos) - _smgr->get_position();
    SGQuatd hlOr = SGQuatd::fromLonLat(_base_pos);
    SGQuatd ec2body = hlOr*_orientation;

    SGVec3f velocity = SGVec3f::zeros();
    if ( _velocity[0] || _velocity[1] || _velocity[2] ) {
       velocity = toVec3f( hlOr.backTransform(_velocity*SG_FEET_TO_METER) );
    }

    sample_map_iterator sample_current = _samples.begin();
    sample_map_iterator sample_end = _samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        SGSoundSample *sample = sample_current->second;
        sample->set_master_volume( _volume );
        sample->set_orientation( _orientation );
        sample->set_rotation( ec2body );
        sample->set_position( position );
        sample->set_velocity( velocity );
    }
}

void SGSampleGroup::update_sample_config( SGSoundSample *sample ) {
    SGVec3f orientation, velocity;
    SGVec3d position;

    if ( _tied_to_listener ) {
        orientation = _smgr->get_direction();
        position = SGVec3d::zeros();
        velocity = _smgr->get_velocity();
    } else {
        sample->update_pos_and_orientation();
        orientation = sample->get_orientation();
        position = sample->get_position();
        velocity = sample->get_velocity();
    }

    if (_smgr->bad_doppler_effect()) {
        velocity *= 100.0f;
    }

#if 0
    if (length(position) > 20000)
        printf("%s source and listener distance greater than 20km!\n",
               _refname.c_str());
    if (isNaN(toVec3f(position).data())) printf("NaN in source position\n");
    if (isNaN(orientation.data())) printf("NaN in source orientation\n");
    if (isNaN(velocity.data())) printf("NaN in source velocity\n");
#endif

    unsigned int source = sample->get_source();
    alSourcefv( source, AL_POSITION, toVec3f(position).data() );
    alSourcefv( source, AL_VELOCITY, velocity.data() );
    alSourcefv( source, AL_DIRECTION, orientation.data() );
    testForALError("position and orientation");

    alSourcef( source, AL_PITCH, sample->get_pitch() );
    alSourcef( source, AL_GAIN, sample->get_volume() );
    testForALError("pitch and gain");

    if ( sample->has_static_data_changed() ) {
        alSourcef( source, AL_CONE_INNER_ANGLE, sample->get_innerangle() );
        alSourcef( source, AL_CONE_OUTER_ANGLE, sample->get_outerangle() );
        alSourcef( source, AL_CONE_OUTER_GAIN, sample->get_outergain() );
        testForALError("audio cone");

        alSourcef( source, AL_MAX_DISTANCE, sample->get_max_dist() );
        alSourcef( source, AL_REFERENCE_DISTANCE,
                           sample->get_reference_dist() );
        testForALError("distance rolloff");
    }
}

bool SGSampleGroup::testForError(void *p, string s)
{
   if (p == NULL) {
      SG_LOG( SG_GENERAL, SG_ALERT, "Error (sample group): " << s);
      return true;
   }
   return false;
}

bool SGSampleGroup::testForALError(string s)
{
    ALenum error = alGetError();
    if (error != AL_NO_ERROR)  {
       SG_LOG( SG_GENERAL, SG_ALERT, "AL Error (" << _refname << "): "
                                      << alGetString(error) << " at " << s);
       return true;
    }
    return false;
}

