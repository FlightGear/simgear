// sample_group.cxx -- Manage a group of samples relative to a base position
//
// Written for the new SoundSystem by Erik Hofman, October 2009
//
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

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>

#include "soundmgr.hxx"
#include "sample_group.hxx"

SGSampleGroup::SGSampleGroup ()
{
    _samples.clear();
}

SGSampleGroup::SGSampleGroup ( SGSoundMgr *smgr,
                               const std::string &refname ):
    _smgr(smgr),
    _refname(refname)
{
    _smgr->add(this, refname);
    _samples.clear();
}

SGSampleGroup::~SGSampleGroup ()
{
    _active = false;
    stop();
    _smgr = nullptr;
}

void SGSampleGroup::cleanup_removed_samples()
{
    // Delete any buffers that might still be in use.
    unsigned int size = _removed_samples.size();
    for (unsigned int i=0; i<size; ) {
        SGSoundSample *sample = _removed_samples[i];

        _smgr->sample_stop(sample);
        bool stopped = _smgr->is_sample_stopped(sample);

        if ( stopped ) {
            sample->stop();
            if (( !sample->is_queue() )&&
                (sample->is_valid_buffer()))
            {
                _smgr->release_buffer(sample);
            }
            _removed_samples.erase( _removed_samples.begin()+i );
            size--;
            continue;
        }
        i++;
    }
}

void SGSampleGroup::start_playing_sample(SGSoundSample *sample)
{
    _smgr->sample_init( sample );
    update_sample_config( sample );
    _smgr->sample_play( sample );
}

void SGSampleGroup::check_playing_sample(SGSoundSample *sample)
{
    // check if the sound has stopped by itself
    if (_smgr->is_sample_stopped(sample)) {
        // sample is stopped because it wasn't looping
        sample->stop();
        sample->no_valid_source();
        _smgr->release_source( sample->get_source() );
        _smgr->release_buffer( sample );
        // Delayed removal because this is called while iterating over _samples
        remove( sample->get_sample_name(), true );
    } else if ( sample->has_changed() ) {
        if ( !sample->is_playing() ) {
            // a request to stop playing the sound has been filed.
            sample->stop();
            sample->no_valid_source();
            _smgr->release_source( sample->get_source() );
        } else if ( _smgr->has_changed() ) {
            update_sample_config( sample );
        }
    }
}

void SGSampleGroup::update( double dt ) {

    if ( !_active || _pause ) return;

    testForMgrError("start of update!!\n");

    cleanup_removed_samples();

    // Update the position and orientation information for all samples.
    if ( _changed || _smgr->has_changed() ) {
        update_pos_and_orientation();
        _changed = false;
    }

    for (auto current : _samples) {
        SGSoundSample *sample = current.second;

        if ( !sample->is_valid_source() && sample->is_playing() && !sample->test_out_of_range()) {
            start_playing_sample(sample);

        } else if ( sample->is_valid_source() ) {
            check_playing_sample(sample);
        }
        testForMgrError("update");
    }

    for (const auto& refname : _refsToRemoveFromSamplesMap) {
        _samples.erase(refname);
    }
    _refsToRemoveFromSamplesMap.clear();
}

// add a sound effect, return true if successful
bool SGSampleGroup::add( SGSharedPtr<SGSoundSample> sound,
                         const std::string& refname )
{
    auto sample_it = _samples.find( refname );
    if ( sample_it != _samples.end() ) {
        // sample name already exists
        return false;
    }

    _samples[refname] = sound;
    return true;
}


// remove a sound effect, return true if successful
//
// TODO: the 'delayedRemoval' boolean parameter has been specifically added
// for check_playing_sample() which is only called while update() is iterating
// over _samples. It may be better style to remove this parameter and add a
// separate member function for use in check_playing_sample(), whose behavior
// would be the same as when passing delayedRemoval=true here.
bool SGSampleGroup::remove( const std::string &refname, bool delayedRemoval )
{
    auto sample_it = _samples.find( refname );
    if ( sample_it == _samples.end() ) {
        // sample was not found
        return false;
    }

    if ( sample_it->second->is_valid_buffer() )
        _removed_samples.push_back( sample_it->second );

    // Do not erase within the loop in update()
    if (delayedRemoval) {
        _refsToRemoveFromSamplesMap.push_back(refname);
    } else {
        _samples.erase(refname);
    }

    return true;
}


// return true of the specified sound exists in the sound manager system
bool SGSampleGroup::exists( const std::string &refname ) {
    auto sample_it = _samples.find( refname );
    if ( sample_it == _samples.end() ) {
        // sample was not found
        return false;
    }

    return true;
}


// return a pointer to the SGSoundSample if the specified sound exists
// in the sound manager system, otherwise return nullptr
SGSoundSample *SGSampleGroup::find( const std::string &refname ) {
    auto sample_it = _samples.find( refname );
    if ( sample_it == _samples.end() ) {
        // sample was not found
        return nullptr;
    }

    return sample_it->second;
}


void
SGSampleGroup::stop ()
{
    _pause = true;
    for (auto current : _samples) {
        _smgr->sample_destroy( current.second );
    }
}

// stop playing all associated samples
void
SGSampleGroup::suspend ()
{
    if (_active && _pause == false) {
        _pause = true;
#ifdef ENABLE_SOUND
        for (auto current : _samples) {
            _smgr->sample_suspend( current.second );
        }
#endif
        testForMgrError("suspend");
    }
}

// resume playing all associated samples
void
SGSampleGroup::resume ()
{
    if (_active && _pause == true) {
#ifdef ENABLE_SOUND
        for (auto current : _samples) {
            _smgr->sample_resume( current.second );
        }
        testForMgrError("resume");
#endif
        _pause = false;
    }
}


// tell the scheduler to play the indexed sample in a continuous loop
bool SGSampleGroup::play( const std::string &refname,
                          bool looping )
{
    SGSoundSample *sample = find( refname );

    if ( sample == nullptr ) {
        return false;
    }

    sample->play( looping );
    return true;
}


// return true of the specified sound is currently being played
bool SGSampleGroup::is_playing( const std::string& refname ) {
    SGSoundSample *sample = find( refname );

    if ( sample == nullptr ) {
        return false;
    }

    return ( sample->is_playing() ) ? true : false;
}

// immediate stop playing the sound
bool SGSampleGroup::stop( const std::string& refname ) {
    SGSoundSample *sample  = find( refname );

    if ( sample == nullptr ) {
        return false;
    }

    sample->stop();
    return true;
}

void SGSampleGroup::set_volume( float vol )
{
    if (vol > _volume*1.01 || vol < _volume*0.99) {
        _volume = vol;
        SG_CLAMP_RANGE(_volume, 0.0f, 1.0f);
        _changed = true;
    }
}

// set the source position and orientation of all managed sounds
void SGSampleGroup::update_pos_and_orientation() {

    SGVec3d base_position = SGVec3d::fromGeod(_base_pos);
    SGVec3d smgr_position = _smgr->get_position();
    SGQuatd hlOr = SGQuatd::fromLonLat(_base_pos);
    SGQuatd ec2body = hlOr*_orientation;

    SGVec3f velocity = SGVec3f::zeros();
    if ( _velocity[0] || _velocity[1] || _velocity[2] ) {
       velocity = toVec3f( hlOr.backTransform(_velocity*SG_FEET_TO_METER) );
    }

    for (auto current : _samples ) {
        SGSoundSample *sample = current.second;
        sample->set_master_volume( _volume );
        sample->set_orientation( _orientation );
        sample->set_rotation( ec2body );
        sample->set_position(base_position);
        sample->set_velocity( velocity );
        sample->set_atmosphere( _degC, _humidity, _pressure );

        // Test if a sample is farther away than max distance, if so
        // stop the sound playback and free it's source.
        if (!_tied_to_listener) {
            sample->update_pos_and_orientation();
            SGVec3d position = sample->get_position() - smgr_position;
            float max2 = sample->get_max_dist() * sample->get_max_dist();
            float dist2 = position[0]*position[0]
                          + position[1]*position[1] + position[2]*position[2];
            if ((dist2 > max2) && !sample->test_out_of_range()) {
                sample->set_out_of_range(true);
            } else if ((dist2 < max2) && sample->test_out_of_range()) {
                sample->set_out_of_range(false);
            }
        }
    }
}

void SGSampleGroup::update_sample_config( SGSoundSample *sample )
{
#ifdef ENABLE_SOUND
    SGVec3f orientation, velocity;
    SGVec3d position;

    if ( _tied_to_listener ) {
        orientation = _smgr->get_direction();
        position = SGVec3d::zeros();
        velocity = _smgr->get_velocity();
    } else {
        sample->update_pos_and_orientation();
        orientation = sample->get_orientation();
        position = sample->get_position() - _smgr->get_position();
        velocity = sample->get_velocity();
    }

#if 0
    if (length(position) > 20000)
        printf("%s source and listener distance greater than 20km!\n",
               _refname.c_str());
    if (isNaN(toVec3f(position).data())) printf("NaN in source position\n");
    if (isNaN(orientation.data())) printf("NaN in source orientation\n");
    if (isNaN(velocity.data())) printf("NaN in source velocity\n");
#endif

    _smgr->update_sample_config( sample, position, orientation, velocity );
#endif
}

bool SGSampleGroup::testForError(void *p, std::string s)
{
   if (p == nullptr) {
      SG_LOG( SG_SOUND, SG_ALERT, "Error (sample group): " << s);
      return true;
   }
   return false;
}

bool SGSampleGroup::testForMgrError(std::string s)
{
    _smgr->testForError(s+" (sample group)", _refname);
    return false;
}
