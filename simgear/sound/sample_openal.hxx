// sample.hxx -- Sound sample encapsulation class
// 
// Written by Curtis Olson, started April 2004.
//
// Copyright (C) 2004  Curtis L. Olson - curt@flightgear.org
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

#if defined(__APPLE__)
# define AL_ILLEGAL_ENUM AL_INVALID_ENUM
# define AL_ILLEGAL_COMMAND AL_INVALID_OPERATION
# include <OpenAL/al.h>
# include <OpenAL/alut.h>
#else
# include <AL/al.h>
# include <AL/alut.h>
#endif

#include <simgear/debug/logstream.hxx>

SG_USING_STD(string);


/**
 * manages everything we need to know for an individual sound sample
 */

class SGSoundSample {

private:

    string sample_name;

    // Buffers hold sound data.
    ALuint buffer;

    // Sources are points emitting sound.
    ALuint source;

    // Position of the source sound.
    ALfloat source_pos[3];

    // Velocity of the source sound.
    ALfloat source_vel[3];

    // configuration values
    ALenum format;
    ALsizei size;
    ALvoid* data;
    ALsizei freq;

    double pitch;
    double volume;
    ALboolean loop;

public:

    /**
     * Constructor
     * @param path Path name to sound
     * @param file File name of sound
     * @param cleanup Request clean up the intermediate data (this
       should usually be true unless you want to manipulate the data
       later.)
     */
    SGSoundSample( const char *path, const char *file, bool cleanup );
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
    inline bool is_playing( ) {
        ALint result;
        alGetSourcei( source, AL_SOURCE_STATE, &result );
        if ( alGetError() != AL_NO_ERROR) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Oops AL error in sample is_playing(): " << sample_name );
        }
        return (result == AL_PLAYING) ;
    }

    /**
     * Get the current pitch setting of this sample.
     */
    inline double get_pitch() const { return pitch; }

    /**
     * Set the pitch of this sample.
     */
    inline void set_pitch( double p ) {
        // clamp in the range of 0.01 to 2.0
        if ( p < 0.01 ) { p = 0.01; }
        if ( p > 2.0 ) { p = 2.0; }
        pitch = p;
        alSourcef( source, AL_PITCH, pitch );
        if ( alGetError() != AL_NO_ERROR) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Oops AL error in sample set_pitch()! " << p
                    << " for " << sample_name );
        }
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
        alSourcef( source, AL_GAIN, volume );
        if ( alGetError() != AL_NO_ERROR) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Oops AL error in sample set_volume()! " << v
                    << " for " << sample_name  );
        }
    }

    /**
     * Returns the size of the sounds sample
     */
    inline int get_size() {
        return size;
    }

    /**
     * Return a pointer to the raw data
     */
    inline char *get_data() {
        return (char *)data;
    }

    /**
     * Set position of sound source (uses same coordinate system as opengl)
     */
    inline void set_source_pos( ALfloat *pos ) {
        source_pos[0] = pos[0];
        source_pos[1] = pos[1];
        source_pos[2] = pos[2];
        alSourcefv( source, AL_POSITION, source_pos );
    }

    /**
     * Set velocity of sound source (uses same coordinate system as opengl)
     */
    inline void set_source_vel( ALfloat *vel ) {
        source_vel[0] = vel[0];
        source_vel[1] = vel[1];
        source_vel[2] = vel[2];
        alSourcefv( source, AL_VELOCITY, source_vel );
    }

};


#endif // _SG_SAMPLE_HXX


