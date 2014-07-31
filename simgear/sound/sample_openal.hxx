///@file
/// Provides an audio sample encapsulation class.
// 
// Written by Curtis Olson, started April 2004.
// Modified to match the new SoundSystem by Erik Hofman, October 2009
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
// Copyright (C) 2009 Erik Hofman <erik@ehofman.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _SG_SAMPLE_HXX
#define _SG_SAMPLE_HXX 1

#include <string>

#include <simgear/compiler.h>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGMath.hxx>
     
class SGPath;

#ifndef AL_FORMAT_MONO8
     #define AL_FORMAT_MONO8    0x1100
#endif
 

/**
 * Encapsulate and audio sample.
 *
 * Manages everything we need to know for an individual audio sample.
 */

class SGSoundSample : public SGReferenced {
public:

     /**
      * Empty constructor, can be used to read data to the systems
      * memory and not to the driver.
      */
    SGSoundSample();

    /**
     * Constructor
     * @param file File name of sound
       Buffer data is freed by the sample group
     */
    SGSoundSample(const char *file, const SGPath& currentDir);

    /**
     * Constructor.
     * @param data Pointer to a memory buffer containing this audio sample data
       The application may free the data by calling free_data(), otherwise it
       will be resident until the class is destroyed. This pointer will be
       set to NULL after calling this function.
     * @param len Byte length of array
     * @param freq Frequency of the provided data (bytes per second)
     * @param format OpenAL format id of the data
     */
    SGSoundSample( void** data, int len, int freq, int format=AL_FORMAT_MONO8 );
    SGSoundSample( const unsigned char** data, int len, int freq,
                   int format = AL_FORMAT_MONO8 );

    /**
     * Destructor
     */
    virtual ~SGSoundSample ();

    /**
     * Detect whether this audio sample holds the information of a sound file.
     * @return Return true if this audio sample is to be constructed from a file.
     */
    inline bool is_file() const { return _is_file; }

    SGPath file_path() const;

    /**
     * Test if this audio sample configuration has changed since the last call.
     * Calling this function will reset the flag so calling it a second
     * time in a row will return false.
     *
     * @return Return true is the configuration has changed in the mean time.
     */
    bool has_changed() {
        bool b = _changed; _changed = false; return b;
    }

    /**
     * Test if static data of audio sample configuration has changed.
     * Calling this function will reset the flag so calling it a second
     * time in a row will return false.
     *
     * @return Return true is the static data has changed in the mean time.
     */
    bool has_static_data_changed() {
        bool b = _static_changed; _static_changed = false; return b;
    }

    /**
     * Schedule this audio sample for playing. Actual playing will only start
     * at the next call op SoundGroup::update()
     *
     * @param loop Whether this sound should be played in a loop.
     */
    void play( bool loop = false ) {
        _playing = true; _loop = loop; _changed = true; _static_changed = true;
    }

    /**
     * Check if this audio sample is set to be continuous looping.
     *
     * @return Return true if this audio sample is set to looping.
     */
    inline bool is_looping() { return _loop; }

    /**
     * Schedule this audio sample to stop playing.
     */
    virtual void stop() {
        _playing = false; _changed = true;
    }

    /**
     * Schedule this audio sample to play once.
     * @see #play
     */
    inline void play_once() { play(false); }

    /** 
     * Schedule this audio sample to play looped.
     * @see #play
     */
    inline void play_looped() { play(true); }

    /**
     * Test if a audio sample is scheduled for playing.
     * @return true if this audio sample is playing, false otherwise.
     */
    inline bool is_playing() { return _playing; }


    /**
     * Set this sample to out-of-range (or not) and
     * Schedule this audio sample to stop (or start) playing.
     */
    inline void set_out_of_range(bool oor = true) {
        _out_of_range = oor; _playing = (!oor && _loop); _changed = true;
    }

    /**
     * Test if this sample to out-of-range or not.
     */
    inline bool test_out_of_range() {
        return _out_of_range;
    }

    /**
     * Set the data associated with this audio sample
     * @param data Pointer to a memory block containg this audio sample data.
       This pointer will be set to NULL after calling this function.
     */
    inline void set_data( const unsigned char **data ) {
        _data = (unsigned char*)*data; *data = NULL;
    }
    inline void set_data( const void **data ) {
        _data = (unsigned char*)*data; *data = NULL;
    }

    /**
     * Return the data associated with this audio sample.
     * @return A pointer to this sound data of this audio sample.
     */
    inline void* get_data() const { return _data; }

    /**
     * Free the data associated with this audio sample
     */
    void free_data();

    /**
     * Set the source id of this source
     * @param sid OpenAL source-id
     */
    virtual void set_source(unsigned int sid) {
        _source = sid; _valid_source = true; _changed = true;
    }

    /**
     * Get the OpenAL source id of this source
     * @return OpenAL source-id
     */
    virtual unsigned int get_source() { return _source; }

    /**
     * Test if the source-id of this audio sample may be passed to OpenAL.
     * @return true if the source-id is valid
     */
    virtual bool is_valid_source() const { return _valid_source; }

    /**
     * Set the source-id of this audio sample to invalid.
     */
    virtual void no_valid_source() { _valid_source = false; }

    /**
     * Set the OpenAL buffer-id of this source
     * @param bid OpenAL buffer-id
     */
    inline void set_buffer(unsigned int bid) {
        _buffer = bid; _valid_buffer = true; _changed = true;
    } 

    /**
     * Get the OpenAL buffer-id of this source
     * @return OpenAL buffer-id
     */
    inline unsigned int get_buffer() { return _buffer; }

    /**
     * Test if the buffer-id of this audio sample may be passed to OpenAL.
     * @return true if the buffer-id is valid
     */
    inline bool is_valid_buffer() const { return _valid_buffer; }

    /**
     * Set the buffer-id of this audio sample to invalid.
     */
    inline void no_valid_buffer() { _valid_buffer = false; }

    /**
     * Set the playback pitch of this audio sample. 
     * Should be between 0.0 and 2.0 for maximum compatibility.
     * @param p Pitch
     */
    inline void set_pitch( float p ) {
        if (p > 2.0) p = 2.0; else if (p < 0.01) p = 0.01;
        _pitch = p; _changed = true;
    }

    /**
     * Get the current pitch value of this audio sample.
     * @return Pitch
     */
    inline float get_pitch() { return _pitch; }

    /**
     * Set the master volume of this sample. Should be between 0.0 and 1.0.
     * The final volume is calculated by multiplying the master and audio sample
     * volume.
     * @param v Volume
     */
    inline void set_master_volume( float v ) {
        if (v > 1.0) v = 1.0; else if (v < 0.0) v = 0.0;
        _master_volume = v; _changed = true;
    }

    /**
     * Set the volume of this audio sample. Should be between 0.0 and 1.0.
     * The final volume is calculated by multiplying the master and audio sample
     * volume.
     * @param v Volume
     */
    inline void set_volume( float v ) {
        if (v > 1.0) v = 1.0; else if (v < 0.0) v = 0.0;
        _volume = v; _changed = true;
    }

    /**
     * Get the final volume value of this audio sample.
     * @return Volume
     */
    inline float get_volume() { return _volume * _master_volume; }

    /**
     * Set the OpenAL format of this audio sample.
     * @param format OpenAL format-id
     */
    inline void set_format( int format ) { _format = format; }

    /**
     * Returns the format of this audio sample.
     * @return OpenAL format-id
     */
    inline int get_format() { return _format; }

    /**
     * Set the frequency (in Herz) of this audio sample.
     * @param freq Frequency
     */
    inline void set_frequency( int freq ) { _freq = freq; }

    /**
     * Returns the frequency (in Herz) of this audio sample.
     * @return Frequency
     */
    inline int get_frequency() { return _freq; }

    /**
     * Sets the size (in bytes) of this audio sample.
     * @param size Data size
     */
    inline void set_size( size_t size ) { _size = size; }

    /**
     * Returns the size (in bytes) of this audio sample.
     * @return Data size
     */
    inline size_t get_size() const { return _size; }

    /**
     * Set the position of this sound relative to the base position.
     * This is in the same coordinate system as OpenGL; y=up, z=back, x=right.
     * @param pos Relative position of this sound
     */
    inline void set_relative_position( const SGVec3f& pos ) {
        _relative_pos = toVec3d(pos); _changed = true;
    }

    /**
     * Set the base position in Cartesian coordinates
     * @param pos position in Cartesian coordinates
     */
    inline void set_position( const SGVec3d& pos ) {
       _base_pos = pos; _changed = true;
    }

    /**
     * Get the absolute position of this sound.
     * This is in the same coordinate system as OpenGL; y=up, z=back, x=right.
     * @return Absolute position
     */
    SGVec3d& get_position() { return _absolute_pos; }

    /**
     * Set the orientation of this sound.
     * @param ori Quaternation containing the orientation information
     */
    inline void set_orientation( const SGQuatd& ori ) {
        _orientation = ori; _changed = true;
    }

    inline void set_rotation( const SGQuatd& ec2body ) {
        _rotation = ec2body; _changed = true;
    }

    /**
     * Set direction of this sound relative to the orientation.
     * This is in the same coordinate system as OpenGL; y=up, z=back, x=right
     * @param dir Sound emission direction
     */
    inline void set_direction( const SGVec3f& dir ) {
        _direction = toVec3d(dir); _static_changed = true;
    }

    /**
     * Define the audio cone parameters for directional audio.
     * Note: setting it to 2 degree will result in 1 degree to both sides.
     * @param inner Inner cone angle (0 - 360 degrees)
     * @param outer Outer cone angle (0 - 360 degrees)
     * @param gain Remaining gain at the edge of the outer cone (0.0 - 1.0)
     */
    void set_audio_cone( float inner, float outer, float gain ) {
        _inner_angle = inner; _outer_angle = outer; _outer_gain = gain;
        _static_changed = true;
    }

    /**
     * Get the orientation vector of this sound.
     * This is in the same coordinate system as OpenGL; y=up, z=back, x=right
     * @return Orientaton vector
     */
    SGVec3f& get_orientation() { return _orivec; }

    /**
     * Get the inner angle of the audio cone.
     * @return Inner angle in degrees
     */
    float get_innerangle() { return _inner_angle; }

    /**
     * Get the outer angle of the audio cone.
     * @return Outer angle in degrees
     */
    float get_outerangle() { return _outer_angle; }

    /**
     * Get the remaining gain at the edge of the outer cone.
     * @return Gain
     */
    float get_outergain() { return _outer_gain; }

    /**
     * Set the velocity vector (in meters per second) of this sound.
     * This is in the local frame coordinate system; x=north, y=east, z=down
     *
     * @param vel Velocity vector
     */
    inline void set_velocity( const SGVec3f& vel ) {
        _velocity = vel; _changed = true;
    }

    /**
     * Get velocity vector (in meters per second) of this sound.
     * This is in the same coordinate system as OpenGL; y=up, z=back, x=right
     * @return Velocity vector
     */
    SGVec3f& get_velocity() { return _velocity; }


    /**
     * Set reference distance (in meters) of this sound.
     * This is the distance where the gain will be half.
     * @param dist Reference distance
     */
    inline void set_reference_dist( float dist ) {
        _reference_dist = dist; _static_changed = true;
    }

    /**
     * Get reference distance ((in meters) of this sound.
     * This is the distance where the gain will be half.
     * @return Reference distance
     */
    inline float get_reference_dist() { return _reference_dist; }


    /**
     * Set maximum distance (in meters) of this sound.
     * This is the distance where this sound is no longer audible.
     * @param dist Maximum distance
     */
    inline void set_max_dist( float dist ) {
        _max_dist = dist; _static_changed = true;
    }

    /**
     * Get maximum distance (in meters) of this sound.
     * This is the distance where this sound is no longer audible.
     * @return dist Maximum distance
     */
    inline float get_max_dist() { return _max_dist; }

    /**
     * Get the reference name of this audio sample.
     * @return Sample name
     */
    inline std::string get_sample_name() const { return _refname; }

    inline virtual bool is_queue() const { return false; }

    void update_pos_and_orientation();

protected:
    int _format;
    size_t _size;
    int _freq;
    bool _changed;
    
    // Sources are points emitting sound.
    bool _valid_source;
    unsigned int _source;

private:

    // Position of the source sound.
    SGVec3d _absolute_pos;      // absolute position
    SGVec3d _relative_pos;      // position relative to the base position
    SGVec3d _direction;         // orientation offset
    SGVec3f _velocity;          // Velocity of the source sound.

    // The position and orientation of this sound
    SGQuatd _orientation;       // base orientation
    SGVec3f _orivec;		// orientation vector for OpenAL
    SGVec3d _base_pos;		// base position

    SGQuatd _rotation;

    std::string _refname;	// name or file path
    unsigned char* _data;

    // configuration values
    
    // Buffers hold sound data.
    bool _valid_buffer;
    unsigned int _buffer;

    // The orientation of this sound (direction and cut-off angles)
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
    bool _static_changed;
    bool _out_of_range;
    bool _is_file;

    std::string random_string();
};


#endif // _SG_SAMPLE_HXX


