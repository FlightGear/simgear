// soundmgr.hxx -- Sound effect management class
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

/**
 * \file soundmgr.hxx
 * Provides a sound manager class to keep track of
 * multiple sounds and manage playing them with different effects and
 * timings.
 */

#ifndef _SG_SOUNDMGR_HXX
#define _SG_SOUNDMGR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>
#include <simgear/timing/timestamp.hxx>

#include STL_STRING
#include <map>

#include <plib/sl.h>
#include <plib/sm.h>

SG_USING_STD(map);
SG_USING_STD(string);


/**
 * manages everything we need to know for an individual sound sample
 */
class SGSimpleSound {

private:

    slSample *sample;
    slEnvelope *pitch_envelope;
    slEnvelope *volume_envelope;
    double pitch;
    double volume;

public:

    SGSimpleSound( const char *path, const char *file = NULL );
    SGSimpleSound( unsigned char *buffer, int len );
    ~SGSimpleSound();

    /**
     * Start playing this sample.
     *
     * @param sched A pointer to the appropriate scheduler.
     * @param looped Define wether the sound should be played in a loop.
     */
    void play( slScheduler *sched, bool looped );

    /**
     * Stop playing this sample.
     *
     * @param sched A pointer to the appropriate scheduler.
     */
    void stop( slScheduler *sched );

    /**
     * Play this sample once.
     * @see #play
     */
    inline void play_once( slScheduler *sched ) { play( sched, false); }

    /** 
     * Play this sample looped.
     * @see #play
     */
    inline void play_looped( slScheduler *sched ) { play( sched, true); }

    /**
     * Test if a sample is curretnly playing.
     * @return true if is is playing, false otherwise.
     */
    inline bool is_playing( ) {
        return ( sample->getPlayCount() > 0 );
    }

    /**
     * Get the current pitch setting of this sample.
     */
    inline double get_pitch() const { return pitch; }

    /**
     * Set the pitch of this sample.
     */
    inline void set_pitch( double p ) {
       pitch = p;
       pitch_envelope->setStep( 0, 0.01, pitch );
    }

    /**
     * Get the current volume setting of this sample.
     */
    inline double get_volume() const { return volume; }

    /**
     * Set the volume of this sample.
     */
    inline void set_volume( double v ) {
       volume = v;
       volume_envelope->setStep( 0, 0.01, volume );
    }

    /**
     * Get a refference to the raw sample.
     */
    inline slSample *get_sample() { return sample; }

    /**
     * Get the pitch envelope setting of this sample.
     */
    inline slEnvelope *get_pitch_envelope() { return pitch_envelope; }

    /**
     * Get the volume envelope setting of this sample.
     */
    inline slEnvelope *get_volume_envelope() { return volume_envelope; }
};


typedef struct {
	int n;
	slSample *sample;
} sample_ref;

typedef map < string, sample_ref * > sample_map;
typedef sample_map::iterator sample_map_iterator;
typedef sample_map::const_iterator const_sample_map_iterator;

typedef map < string, SGSimpleSound * > sound_map;
typedef sound_map::iterator sound_map_iterator;
typedef sound_map::const_iterator const_sound_map_iterator;


/**
 * Manage a collection of SGSimpleSound instances
 */
class SGSoundMgr
{

    slScheduler *audio_sched;
    smMixer *audio_mixer;

    sound_map sounds;
    sample_map samples;

    double safety;

public:

    SGSoundMgr();
    ~SGSoundMgr();


    /**
     * (re) initialize the sound manager.
     */
    void init();


    /**
     * Bind properties for the sound manager.
     */
    void bind ();


    /**
     * Unbind properties for the sound manager.
     */
    void unbind ();


    /**
     * Run the audio scheduler.
     */
    void update(double dt);


    /**
     * Pause all sounds.
     */
    void pause ();


    /**
     * Resume all sounds.
     */
    void resume ();


    /**
     * is audio working?
     */
    inline bool is_working() const { return !audio_sched->notWorking(); }

    /**
     * reinitialize the sound manager
     */
    inline void reinit() { init(); }

    /**
     * add a sound effect, return true if successful
     */
    bool add( SGSimpleSound *sound, const string& refname);

    /**
     * Add a sound file to the sound manager.
     *
     * The advantage of using this function over the previous one is that
     * it doesn't load a sample if it already is in memory, but instead it
     * uses the already loaded sample data.
     *
     * @param refname A refference name to make a distincion between samples.
     * @param path The path or full filename of the sample to load.
     * @param file An optional filename which will be appended to the path.
     * @return An instance of the sound for further manipulation.
     */
    SGSimpleSound *add( const string& refname,
                      const char *path, const char *file = NULL );

    /** 
     * remove a sound effect, return true if successful
     */
    bool remove( const string& refname );

    /**
     * return true of the specified sound exists in the sound manager system
     */
    bool exists( const string& refname );

    /**
     * return a pointer to the SGSimpleSound if the specified sound
     * exists in the sound manager system, otherwise return NULL
     */
     SGSimpleSound *find( const string& refname );

    /**
     * tell the scheduler to play the indexed sample in a continuous
     * loop
     */
    bool play_looped( const string& refname );

    /**
     * tell the scheduler to play the indexed sample once
     */
    bool play_once( const string& refname );

    /**
     * return true of the specified sound is currently being played
     */
    bool is_playing( const string& refname );

    /**
     * immediate stop playing the sound
     */
    bool stop( const string& refname );

    /** 
     * return the audio scheduler 
     */
    inline slScheduler *get_scheduler( ) { return audio_sched; };
};


#endif // _SG_SOUNDMGR_HXX


