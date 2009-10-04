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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

#include <string>

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGMath.hxx>

#include <plib/sg.h>

using std::string;

/**
 * manages everything we need to know for an individual sound sample
 */

class SGSoundSample : public SGReferenced {

private:

    // Position of the source sound.
    SGVec3d _absolute_pos;	// absolute position
    SGVec3f _relative_pos;	// position relative to the base position
    SGVec3d _base_pos;		// base position

    // The orientation of the sound (direction and cut-off angles)
    SGVec3f _direction;

    // Velocity of the source sound.
    SGVec3f _velocity;

    string _sample_name;
    unsigned char *_data;

    // configuration values
    int _format;
    int _size;
    int _freq;

    // Buffers hold sound data.
    bool _valid_buffer;
    unsigned int _buffer;

    // Sources are points emitting sound.
    bool _valid_source;
    unsigned int _source;

    // The orientation of the sound (direction and cut-off angles)
    float _inner_angle;
    float _outer_angle;
    float _outer_gain;

    float _pitch;
    float _volume;
    float _master_volume;
    float _reference_dist;
    float _max_dist;
    bool _loop;

    bool _playing;
    bool _changed;
    bool _static_changed;
    bool _is_file;

    void update_absolute_position();

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
    SGSoundSample( unsigned char *data, int len, int freq, int format = AL_FORMAT_MONO8 );

    ~SGSoundSample ();

    /**
     * detect wheter the sample holds the information of a sound file
     */
    inline bool is_file() const { return _is_file; }

    /**
     * Test whether this sample has a changed configuration since the last
     * call. (Calling this function resets the value).
     */
    inline bool has_changed() {
        bool b = _changed; _changed = false; return b;
    }

    inline bool has_static_data_changed() {
        bool b = _static_changed; _static_changed = false; return b;
    }


    /**
     * Start playing this sample.
     *
     * @param _loop Define whether the sound should be played in a loop.
     */
    inline void play( bool loop ) {
        _playing = true; _loop = loop; _changed = true;
    }

    /**
     * Return if the sample is looping or not.
     */
    inline bool get_looping() { return _loop; }

    /**
     * Stop playing this sample.
     *
     * @param sched A pointer to the appropriate scheduler.
     */
    inline void stop() {
        _playing = false; _changed = true;
    }

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
     * Test if a sample is currently playing.
     * @return true if is is playing, false otherwise.
     */
    inline bool is_playing() { return _playing; }

    /**
     * set the data associated with this sample
     */
    inline void set_data( unsigned char* data ) {
        _data = data;
    }

    /**
     * @return the data associated with this sample
     */
    inline void* get_data() const { return _data; }

    /**
     * free the data associated with this sample
     */
    inline void free_data() {
        if (_data != NULL) { delete[] _data; _data = NULL; }
    }

    /**
     * set the source id of this source
     */
    inline void set_source(unsigned int s) {
        _source = s; _valid_source = true; _changed = true;
    }

    /**
     * get the source id of this source
     */
    inline unsigned int get_source() { return _source; }

    /**
     * detect wheter the source id of the sample is valid
     */
    inline bool is_valid_source() const { return _valid_source; }

    /**
     * set the source id of the sample to invalid.
     */
    inline void no_valid_source() {
        _valid_source = false;
    }

    /**
     * set the buffer id of this source
     */
    inline void set_buffer(unsigned int b) {
        _buffer = b; _valid_buffer = true; _changed = true;
    } 

    /**
     * get the buffer id of this source
     */
    inline unsigned int get_buffer() { return _buffer; }

    /**
     * detect wheter the source id of the sample is valid
     */
    inline bool is_valid_buffer() const { return _valid_buffer; }

    /**
     * set the source id of the sample to invalid.
     */
    inline void no_valid_buffer() {
        _valid_buffer = false;
    }

    /**
     * Get the current pitch setting of this sample.
     */
    inline float get_pitch() { return _pitch; }

    /**
     * Set the pitch of this sample.
     */
    inline void set_pitch( float p ) {
        _pitch = p; _changed = true;
    }

    /**
     * Get the current volume setting of this sample.
     */
    inline float get_volume() { return _volume * _master_volume; }

    /**
     * Set the master (sampel group) volume of this sample.
     */
    inline void set_master_volume( float v ) {
        _master_volume = v; _changed = true;
    }

    /**
     * Set the volume of this sample.
     */
    inline void set_volume( float v ) {
        _volume = v; _changed = true;
    }

    /**
     * Set the format of the sounds sample
     */
    inline void set_format( int format ) {
        _format = format;
    }

    /**
     * Returns the format of the sounds sample
     */
    inline int get_format() { return _format; }


    /**
     * Set the frequency of the sounds sample
     */
    inline void set_frequency( int freq ) {
        _freq = freq; _changed = true;
    }

    /**
     * Returns the frequency of the sounds sample
     */
    inline int get_frequency() { return _freq; }

    /**
     * Returns the size of the sounds sample
     */
    inline void set_size( int size ) {
        _size = size;
    }

    /**
     * Returns the size of the sounds sample
     */
    inline int get_size() const { return _size; }

    /**
     * Set position of the sound source (uses same coordinate system as opengl)
     */
    void set_base_position( SGVec3d pos );
    void set_relative_position( SGVec3f pos );

    /**
     * Get position of the sound source (uses same coordinate system as opengl)
     */
    inline float *get_position() const { return toVec3f(_absolute_pos).data(); }

    /**
     * Set the orientation of the sound source, both for direction
     * and audio cut-off angles.
     */
    void set_orientation( SGVec3f dir );

    /**
     * Define the audio cone parameters for directional audio
     */
    inline void set_audio_cone( float inner, float outer, float gain ) {
        _inner_angle = inner;
        _outer_angle = outer;
        _outer_gain = gain;
        _static_changed = true;
    }

    /**
     * Get the orientation of the sound source, the inner or outer angle
     * or outer gain.
     */
    inline float *get_orientation() { return _direction.data(); }
    inline float *get_direction() { return _direction.data(); }
    inline float get_innerangle() { return _inner_angle; }
    inline float get_outerangle() { return _outer_angle; }
    inline float get_outergain() { return _outer_gain; }

    /**
     * Set velocity of the sound source (uses same coordinate system as opengl)
     */
    inline void set_velocity( SGVec3f vel ) {
        _velocity = SGVec3f(vel); _changed = true;
    }

    /**
     * Get velocity of the sound source (uses same coordinate system as opengl)
     */
    inline float *get_velocity() { return _velocity.data(); }


    /**
     * Set reference distance of sound (the distance where the gain
     * will be half.)
     */
    inline void set_reference_dist( float dist ) {
        _reference_dist = dist; _static_changed = true;
    }

    /**
     * Get reference distance of sound (the distance where the gain
     * will be half.)
     */
    inline float get_reference_dist() { return _reference_dist; }


    /**
     * Set maximum distance of sound (the distance where the sound is
     * no longer audible.
     */
    void set_max_dist( float dist ) {
        _max_dist = dist; _static_changed = true;
    }

    /**
     * Get maximum istance of sound (the distance where the sound is
     * no longer audible.
     */
    inline float get_max_dist() { return _max_dist; }

    /**
     * Get the name of this sample
     */
    inline string get_sample_name() { return _sample_name; }
};


#endif // _SG_SAMPLE_HXX


