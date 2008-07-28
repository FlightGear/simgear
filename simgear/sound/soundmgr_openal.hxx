// soundmgr.hxx -- Sound effect management class
//
// Sound manager initially written by David Findlay
// <david_j_findlay@yahoo.com.au> 2001
//
// C++-ified by Curtis Olson, started March 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

/**
 * \file soundmgr.hxx
 * Provides a sound manager class to keep track of
 * multiple sounds and manage playing them with different effects and
 * timings.
 */

#ifndef _SG_SOUNDMGR_OPENAL_HXX
#define _SG_SOUNDMGR_OPENAL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <string>
#include <map>

#if defined( __APPLE__ )
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

#include "sample_openal.hxx"

using std::map;
using std::string;


typedef map < string, SGSharedPtr<SGSoundSample> > sample_map;
typedef sample_map::iterator sample_map_iterator;
typedef sample_map::const_iterator const_sample_map_iterator;


/**
 * Manage a collection of SGSoundSample instances
 */
class SGSoundMgr
{

    ALCdevice *dev;
    ALCcontext *context;

    // Position of the listener.
    ALfloat listener_pos[3];

    // Velocity of the listener.
    ALfloat listener_vel[3];

    // Orientation of the listener. (first 3 elements are "at", second
    // 3 are "up")
    ALfloat listener_ori[6];

    sample_map samples;

    bool working;
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
    void bind();


    /**
     * Unbind properties for the sound manager.
     */
    void unbind();


    /**
     * Run the audio scheduler.
     */
    void update(double dt);


    /**
     * Pause all sounds.
     */
    void pause();


    /**
     * Resume all sounds.
     */
    void resume();


    /**
     * is audio working?
     */
    inline bool is_working() const { return working; }

    /**
     * reinitialize the sound manager
     */
    inline void reinit() { init(); }

    /**
     * add a sound effect, return true if successful
     */
    bool add( SGSoundSample *sound, const string& refname);

    /** 
     * remove a sound effect, return true if successful
     */
    bool remove( const string& refname );

    /**
     * return true of the specified sound exists in the sound manager system
     */
    bool exists( const string& refname );

    /**
     * return a pointer to the SGSoundSample if the specified sound
     * exists in the sound manager system, otherwise return NULL
     */
    SGSoundSample *find( const string& refname );

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
     * set overall volume for the application.
     * @param vol 1.0 is default, must be greater than 0
     */
    inline void set_volume( const ALfloat vol ) {
        if ( vol > 0.0 ) {
            alListenerf( AL_GAIN, vol );
        }
    }

    /**
     * set the position of the listener (in opengl coordinates)
     */
    inline void set_listener_pos( ALfloat *pos ) {
        listener_pos[0] = pos[0];
        listener_pos[1] = pos[1];
        listener_pos[2] = pos[2];
        alListenerfv( AL_POSITION, listener_pos );
    }

    /**
     * set the velocity of the listener (in opengl coordinates)
     */
    inline void set_listener_vel( ALfloat *vel ) {
        listener_vel[0] = vel[0];
        listener_vel[1] = vel[1];
        listener_vel[2] = vel[2];
#ifdef USE_OPEN_AL_DOPPLER
        alListenerfv( AL_VELOCITY, listener_vel );
#endif
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
    inline void set_listener_orientation( ALfloat *ori ) {
        listener_ori[0] = ori[0];
        listener_ori[1] = ori[1];
        listener_ori[2] = ori[2];
        listener_ori[3] = ori[3];
        listener_ori[4] = ori[4];
        listener_ori[5] = ori[5];
        alListenerfv( AL_ORIENTATION, listener_ori );
    }

    /**
     * set the positions of all managaged sound sources 
     */
    void set_source_pos_all( ALfloat *pos );

    /**
     * set the velocities of all managaged sound sources 
     */
    void set_source_vel_all( ALfloat *pos );
};


#endif // _SG_SOUNDMGR_OPENAL_HXX


