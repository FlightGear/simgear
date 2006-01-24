// sample.hxx -- Sound sample encapsulation class
// 
// Written by Curtis Olson, started April 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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
 * \file sample.hxx
 * Provides a sound sample encapsulation
 */

#ifndef _SG_SAMPLE_HXX
#define _SG_SAMPLE_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include STL_STRING

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include <plib/sg.h>

#if defined(__APPLE__)
# define AL_ILLEGAL_ENUM AL_INVALID_ENUM
# define AL_ILLEGAL_COMMAND AL_INVALID_OPERATION
# include <OpenAL/al.h>
# include <OpenAL/alut.h>
#else
# include <AL/al.h>
# include <AL/alut.h>
#endif

SG_USING_STD(string);

/**
 * manages everything we need to know for an individual sound sample
 */

class SGSoundSample : public SGReferenced {

private:

    string sample_name;

    // Buffers hold sound data.
    ALuint buffer;

    // Sources are points emitting sound.
    ALuint source;

    // Position of the source sound.
    ALfloat source_pos[3];

    // A constant offset to be applied to the final source_pos
    ALfloat offset_pos[3];

    // The orientation of the sound (direction and cut-off angles)
    ALfloat direction[3];
    ALfloat inner, outer, outergain;

    // Velocity of the source sound.
    ALfloat source_vel[3];

    // configuration values
    ALenum format;
    ALsizei size;
    ALsizei freq;

    double pitch;
    double volume;
    double reference_dist;
    double max_dist;
    ALboolean loop;

    bool playing;
    bool bind_source();

public:

     /**
      * Empty constructor, can be used to read data to the systems
      * memory and not to the driver.
      */
    SGSoundSample();

    /**
     * Constructor
     * @param path Path name to sound
     * @param file File name of sound
       should usually be true unless you want to manipulate the data
       later.)
     */
    SGSoundSample( const char *path, const char *file );

    /**
     * Constructor.
     * @param _data Pointer to a memory buffer containing the sample data
       the application is responsible for freeing the buffer data.
     * @param len Byte length of array
     * @param _freq Frequency of the provided data (bytes per second)
       should usually be true unless you want to manipulate the data
       later.)
     */
    SGSoundSample( unsigned char *_data, int len, int _freq );

    ~SGSoundSample();

    /**
     * Start playing this sample.
     *
     * @param _loop Define wether the sound should be played in a loop.
     */
    void play( bool _loop );

    /**
     * Stop playing this sample.
     *
     * @param sched A pointer to the appropriate scheduler.
     */
    void stop();

    /**
     * Play this sample once.
     * @see #play
     */
    inline void play_once() { play(false); }

    /** 
     * Play this sample looped.
     * @see #play
     */
    inline void play_looped() { play(true); }

    /**
     * Test if a sample is curretnly playing.
     * @return true if is is playing, false otherwise.
     */
    bool is_playing( );

    /**
     * Get the current pitch setting of this sample.
     */
    inline double get_pitch() const { return pitch; }

    /**
     * Set the pitch of this sample.
     */
    void set_pitch( double p );

    /**
     * Get the current volume setting of this sample.
     */
    inline double get_volume() const { return volume; }

    /**
     * Set the volume of this sample.
     */
    void set_volume( double v );

    /**
     * Returns the size of the sounds sample
     */
    inline int get_size() {
        return size;
    }

    /**
     * Set position of sound source (uses same coordinate system as opengl)
     */
    void set_source_pos( ALfloat *pos );

    /**
     * Set "constant" offset position of sound source (uses same
     * coordinate system as opengl)
     */
    void set_offset_pos( ALfloat *pos );

    /**
     * Set the orientation of the sound source, both for direction
     * and audio cut-off angles.
     */
    void set_orientation( ALfloat *dir, ALfloat inner_angle=360.0,
                                               ALfloat outer_angle=360.0,
                                               ALfloat outer_gain=0.0);

    /**
     * Set velocity of sound source (uses same coordinate system as opengl)
     */
    void set_source_vel( ALfloat *vel );


    /**
     * Set reference distance of sound (the distance where the gain
     * will be half.)
     */
    void set_reference_dist( ALfloat dist );


    /**
     * Set maximume distance of sound (the distance where the sound is
     * no longer audible.
     */
    void set_max_dist( ALfloat dist );

    /**
     * Load a sound file into a memory buffer only.
     */
    ALvoid* load_file(const char *path, const char *file);
};


#endif // _SG_SAMPLE_HXX


