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

#ifndef _SG_SOUNDMGR_OPENAL_HXX
#define _SG_SOUNDMGR_OPENAL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include STL_STRING
#include <map>

#if defined( __APPLE__ )
# include <OpenAL/al.h>
#else
# include <AL/al.h>
#endif

#include "sample_openal.hxx"

SG_USING_STD(map);
SG_USING_STD(string);


typedef map < string, SGSoundSample * > sample_map;
typedef sample_map::iterator sample_map_iterator;
typedef sample_map::const_iterator const_sample_map_iterator;


/**
 * Manage a collection of SGSoundSample instances
 */
class SGSoundMgr
{

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
};


#endif // _SG_SOUNDMGR_OPENAL_HXX


