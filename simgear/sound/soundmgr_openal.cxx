// soundmgr.cxx -- Sound effect management class
//
// Sound manager initially written by David Findlay
// <david_j_findlay@yahoo.com.au> 2001
//
// C++-ified by Curtis Olson, started March 2001.
//
// Copyright (C) 2001  Curtis L. Olson - curt@flightgear.org
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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#include <iostream>

#if defined(__APPLE__)
# include <OpenAL/al.h>
# include <OpenAL/alut.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alut.h>
# include <AL/alc.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include "soundmgr_openal.hxx"


//
// Sound Manager
//

// constructor
SGSoundMgr::SGSoundMgr() {

    SG_LOG( SG_GENERAL, SG_ALERT, "Initializing OpenAL sound manager" );

    // initialize OpenAL
    alutInit( 0, NULL );
    alGetError();
    if ( alGetError() == AL_NO_ERROR) {
        working = true;
    } else {
        working = false;
	SG_LOG( SG_GENERAL, SG_ALERT, "Audio initialization failed!" );
    }

    listener_pos[0] = 0.0;
    listener_pos[1] = 0.0;
    listener_pos[2] = 0.0;

    listener_vel[0] = 0.0;
    listener_vel[1] = 0.0;
    listener_vel[2] = 0.0;
    
    listener_ori[0] = 0.0;
    listener_ori[1] = 0.0;
    listener_ori[2] = -1.0;
    listener_ori[3] = 0.0;
    listener_ori[4] = 1.0;
    listener_ori[5] = 0.0;

    alListenerfv( AL_POSITION, listener_pos );
    alListenerfv( AL_VELOCITY, listener_vel );
    alListenerfv( AL_ORIENTATION, listener_ori );
    alGetError();
    if ( alGetError() != AL_NO_ERROR) {
	SG_LOG( SG_GENERAL, SG_ALERT,
                "Oops AL error after audio initialization!" );
    }

    // exaggerate the ear candy?
    alDopplerFactor(1.0);
    alDopplerVelocity(340.0);	// speed of sound in meters per second.
}

// destructor

SGSoundMgr::~SGSoundMgr() {

    //
    // Remove the samples from the sample manager.
    //
    sample_map_iterator sample_current = samples.begin();
    sample_map_iterator sample_end = samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
	SGSoundSample *sample = sample_current->second;
	delete sample;
    }

    alutExit();
}


// initialize the sound manager
void SGSoundMgr::init() {
    //
    // Remove the samples from the sample manager.
    //
    sample_map_iterator sample_current = samples.begin();
    sample_map_iterator sample_end = samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
        SGSoundSample *sample = sample_current->second;
        delete sample;
    }
    samples.clear();
}


void SGSoundMgr::bind ()
{
    // no properties
}


void SGSoundMgr::unbind ()
{
    // no properties
}


// run the audio scheduler
void SGSoundMgr::update( double dt ) {
}


void
SGSoundMgr::pause ()
{
    ALCcontext *pCurContext = alcGetCurrentContext();
    alcSuspendContext( pCurContext );
    if ( alGetError() != AL_NO_ERROR) {
	SG_LOG( SG_GENERAL, SG_ALERT,
                "Oops AL error after soundmgr pause()!" );
    }
}


void
SGSoundMgr::resume ()
{
    ALCcontext *pCurContext = alcGetCurrentContext();
    alcProcessContext( pCurContext );
    if ( alGetError() != AL_NO_ERROR) {
	SG_LOG( SG_GENERAL, SG_ALERT,
                "Oops AL error after soundmgr resume()!" );
    }
}


// add a sound effect, return true if successful
bool SGSoundMgr::add( SGSoundSample *sound, const string& refname ) {

    sample_map_iterator sample_it = samples.find( refname );
    if ( sample_it != samples.end() ) {
        // sound already exists
        return false;
    }

    samples[refname] = sound;

    return true;
}


// remove a sound effect, return true if successful
bool SGSoundMgr::remove( const string &refname ) {

    sample_map_iterator sample_it = samples.find( refname );
    if ( sample_it != samples.end() ) {
	// first stop the sound from playing (so we don't bomb the
	// audio scheduler)
	SGSoundSample *sample = sample_it->second;
        delete sample;
        samples.erase( sample_it );

        // cout << "sndmgr: removed -> " << refname << endl;
	return true;
    } else {
        // cout << "sndmgr: failed remove -> " << refname << endl;
        return false;
    }
}


// return true of the specified sound exists in the sound manager system
bool SGSoundMgr::exists( const string &refname ) {
    sample_map_iterator sample_it = samples.find( refname );
    if ( sample_it != samples.end() ) {
	return true;
    } else {
	return false;
    }
}


// return a pointer to the SGSoundSample if the specified sound exists
// in the sound manager system, otherwise return NULL
SGSoundSample *SGSoundMgr::find( const string &refname ) {
    sample_map_iterator sample_it = samples.find( refname );
    if ( sample_it != samples.end() ) {
	return sample_it->second;
    } else {
	return NULL;
    }
}


// tell the scheduler to play the indexed sample in a continuous
// loop
bool SGSoundMgr::play_looped( const string &refname ) {
    SGSoundSample *sample;

    if ( (sample = find( refname )) == NULL ) {
        return false;
    } else {
        sample->play( true );
        return true;
    }
}


// tell the scheduler to play the indexed sample once
bool SGSoundMgr::play_once( const string& refname ) {
    SGSoundSample *sample;

    if ( (sample = find( refname )) == NULL ) {
        return false;
    } else {
        sample->play( false );
        return true;
    }
}


// return true of the specified sound is currently being played
bool SGSoundMgr::is_playing( const string& refname ) {
    SGSoundSample *sample;

    if ( (sample = find( refname )) == NULL ) {
        return false;
    } else {
        return ( sample->is_playing() );
    }
}


// immediate stop playing the sound
bool SGSoundMgr::stop( const string& refname ) {
    SGSoundSample *sample;

    if ( (sample = find( refname )) == NULL ) {
        return false;
    } else {
        sample->stop();
        return true;
    }
}


// set source position of all managed sounds
void SGSoundMgr::set_source_pos_all( ALfloat *pos ) {
    sample_map_iterator sample_current = samples.begin();
    sample_map_iterator sample_end = samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
	SGSoundSample *sample = sample_current->second;
        sample->set_source_pos( pos );
    }
}


// set source velocity of all managed sounds
void SGSoundMgr::set_source_vel_all( ALfloat *vel ) {
    sample_map_iterator sample_current = samples.begin();
    sample_map_iterator sample_end = samples.end();
    for ( ; sample_current != sample_end; ++sample_current ) {
	SGSoundSample *sample = sample_current->second;
        sample->set_source_vel( vel );
    }
}
