// sample.cxx -- Sound sample encapsulation class
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


#if defined( __APPLE__ )
# define AL_ILLEGAL_ENUM AL_INVALID_ENUM
# define AL_ILLEGAL_COMMAND AL_INVALID_OPERATION
# include <OpenAL/al.h>
# include <OpenAL/alut.h>
#else
# include <AL/al.h>
# include <AL/alut.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>

#include "sample_openal.hxx"


//
// SGSoundSample
//


static bool print_openal_error(const string &s = "unknown") {
    ALuint error = alGetError();
    if ( error == AL_NO_ERROR ) {
       return false;
    } else if ( error == AL_INVALID_NAME ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "OpenAL error (AL_INVALID_NAME): " << s );
    } else if ( error == AL_ILLEGAL_ENUM ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "OpenAL error (AL_ILLEGAL_ENUM): "  << s );
    } else if ( error == AL_INVALID_VALUE ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "OpenAL error (AL_INVALID_VALUE): " << s );
    } else if ( error == AL_ILLEGAL_COMMAND ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "OpenAL error (AL_ILLEGAL_COMMAND): " << s );
    } else if ( error == AL_OUT_OF_MEMORY ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "OpenAL error (AL_OUT_OF_MEMORY): " << s );
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Unhandled error code = " << error );
    }
    return error;
}

// empry constructor
SGSoundSample::SGSoundSample() :
    buffer(0),
    source(0),
    pitch(1.0),
    volume(1.0),
    reference_dist(500.0),
    max_dist(3000.),
    loop(AL_FALSE),
    playing(false)
{
}

// constructor
SGSoundSample::SGSoundSample( const char *path, const char *file) :
    buffer(0),
    source(0),
    pitch(1.0),
    volume(1.0),
    reference_dist(500.0),
    max_dist(3000.),
    loop(AL_FALSE),
    playing(false)
{
    SGPath samplepath( path );
    if ( strlen(file) ) {
        samplepath.append( file );
    }
    sample_name = samplepath.str();

    SG_LOG( SG_GENERAL, SG_DEBUG, "From file sounds sample = "
            << samplepath.str() );

    source_pos[0] = 0.0; source_pos[1] = 0.0; source_pos[2] = 0.0;
    offset_pos[0] = 0.0; offset_pos[1] = 0.0; offset_pos[2] = 0.0;
    source_vel[0] = 0.0; source_vel[1] = 0.0; source_vel[2] = 0.0;
    direction[0] = 0.0; direction[1] = 0.0; direction[2] = 0.0;
    inner = outer = 360.0; outergain = 0.0;

    // clear errors from elsewhere?
    alGetError();

    // create an OpenAL buffer handle
    alGenBuffers(1, &buffer);
    if ( print_openal_error("constructor (alGenBuffers)") ) {
        throw sg_exception("Failed to gen OpenAL buffer.");
    }

    // Load the sample file
#if defined(ALUT_API_MAJOR_VERSION) && ALUT_API_MAJOR_VERSION >= 1

  buffer = alutCreateBufferFromFile(samplepath.c_str());
  if (buffer == AL_NONE) {
     ALenum error = alutGetError ();
     print_openal_error("constructor (alutCreateBufferFromFile)");
     throw sg_exception("Failed to load wav file: "+string(alutGetErrorString (error)));
  }

#else
        //
	// pre 1.0 alut version
        //
    ALvoid* data = load_file(path, file);

    // Copy data to the internal OpenAL buffer
    alBufferData( buffer, format, data, size, freq );

    if ( print_openal_error("constructor (alBufferData)") ) {
        throw sg_exception("Failed to buffer data.");
    }

    alutUnloadWAV( format, data, size, freq );
#endif

    print_openal_error("constructor return");
}

// constructor
SGSoundSample::SGSoundSample( unsigned char *_data, int len, int _freq ) :
    buffer(0),
    source(0),
    pitch(1.0),
    volume(1.0),
    reference_dist(500.0),
    max_dist(3000.),
    loop(AL_FALSE),
    playing(false)
{
    SG_LOG( SG_GENERAL, SG_DEBUG, "In memory sounds sample" );

    sample_name = "unknown, generated from data";

    source_pos[0] = 0.0; source_pos[1] = 0.0; source_pos[2] = 0.0;
    offset_pos[0] = 0.0; offset_pos[1] = 0.0; offset_pos[2] = 0.0;
    source_vel[0] = 0.0; source_vel[1] = 0.0; source_vel[2] = 0.0;
    direction[0] = 0.0; direction[1] = 0.0; direction[2] = 0.0;
    inner = outer = 360.0; outergain = 0.0;

    // clear errors from elsewhere?
    alGetError();

    // Load wav data into a buffer.
    alGenBuffers(1, &buffer);
    if ( print_openal_error("constructor (alGenBuffers)") ) {
        throw sg_exception("Failed to gen buffer." );
    }

    format = AL_FORMAT_MONO8;
    size = len;
    freq = _freq;

    alBufferData( buffer, format, _data, size, freq );
    if ( print_openal_error("constructor (alBufferData)") ) {
        throw sg_exception("Failed to buffer data.");
    }

    print_openal_error("constructor return");
}


// destructor
SGSoundSample::~SGSoundSample() {
    SG_LOG( SG_GENERAL, SG_INFO, "Deleting a sample" );
    if (buffer)
        alDeleteBuffers(1, &buffer);
}


// play the sample
void SGSoundSample::play( bool _loop ) {

    if ( source ) {
        alSourceStop( source );
    }

    playing = bind_source();
    if ( playing ) {
        loop = _loop;
    
        alSourcei( source, AL_LOOPING, loop );
        alSourcePlay( source );

        print_openal_error("play (alSourcePlay)");
    }
}


// stop playing the sample
void SGSoundSample::stop() {
    if (playing) {
        alSourceStop( source );
        alDeleteSources(1, &source);
        source = 0;
        print_openal_error("stop (alDeleteSources)");
    }
    playing = false;
}

// Generate sound source
bool
SGSoundSample::bind_source() {

    if ( playing ) {
        return true;
    }
    if ( buffer == 0 ) {
        return false;
    }

    // Bind buffer with a source.
    alGetError();
    alGenSources(1, &source);
    if ( print_openal_error("bind_source (alGenSources)") ) {
        // No biggy, better luck next time.
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to generate audio source.");
        // SG_LOG( SG_GENERAL, SG_ALERT, "Please update your sound driver and try again.");
        return false;
    }

    alSourcei( source, AL_BUFFER, buffer );
    alSourcef( source, AL_PITCH, pitch );
    alSourcef( source, AL_GAIN, volume );
    alSourcefv( source, AL_POSITION, source_pos );
    alSourcefv( source, AL_DIRECTION, direction );
    alSourcef( source, AL_CONE_INNER_ANGLE, inner );
    alSourcef( source, AL_CONE_OUTER_ANGLE, outer );
    alSourcef( source, AL_CONE_OUTER_GAIN, outergain);
    alSourcefv( source, AL_VELOCITY, source_vel );
    alSourcei( source, AL_LOOPING, loop );

    alSourcei( source, AL_SOURCE_RELATIVE, AL_TRUE );
    alSourcef( source, AL_REFERENCE_DISTANCE, reference_dist );
    alSourcef( source, AL_MAX_DISTANCE, max_dist );

    print_openal_error("bind_sources return");

    return true;
}

void
SGSoundSample::set_pitch( double p ) {
    // clamp in the range of 0.01 to 2.0
    if ( p < 0.01 ) { p = 0.01; }
    if ( p > 2.0 ) { p = 2.0; }
    pitch = p;
    if (playing) {
        alSourcef( source, AL_PITCH, pitch );
        print_openal_error("set_pitch");
    }
}

void
SGSoundSample::set_volume( double v ) {
    volume = v;
    if (playing) {
        alSourcef( source, AL_GAIN, volume );
        print_openal_error("set_volume");
    }
}


bool
SGSoundSample::is_playing( ) {
    if (playing) {
        ALint result;
        alGetSourcei( source, AL_SOURCE_STATE, &result );
        if ( alGetError() != AL_NO_ERROR) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                "Oops AL error in sample is_playing(): " << sample_name );
        }
        return (result == AL_PLAYING) ;
    } else
        return false;
}

void
SGSoundSample::set_source_pos( ALfloat *pos ) {
    source_pos[0] = pos[0];
    source_pos[1] = pos[1];
    source_pos[2] = pos[2];

    if (playing) {
        sgVec3 final_pos;
        sgAddVec3( final_pos, source_pos, offset_pos );

        alSourcefv( source, AL_POSITION, final_pos );
    }
}

void
SGSoundSample::set_offset_pos( ALfloat *pos ) {
    offset_pos[0] = pos[0];
    offset_pos[1] = pos[1];
    offset_pos[2] = pos[2];

    if (playing) {
        sgVec3 final_pos;
        sgAddVec3( final_pos, source_pos, offset_pos );

        alSourcefv( source, AL_POSITION, final_pos );
    }
}

void
SGSoundSample::set_orientation( ALfloat *dir, ALfloat inner_angle,
                                           ALfloat outer_angle,
                                           ALfloat outer_gain)
{
    inner = inner_angle;
    outer = outer_angle;
    outergain = outer_gain;
    if (playing) {
        alSourcefv( source, AL_DIRECTION, dir);
        alSourcef( source, AL_CONE_INNER_ANGLE, inner );
        alSourcef( source, AL_CONE_OUTER_ANGLE, outer );
        alSourcef( source, AL_CONE_OUTER_GAIN, outergain );
    }
}

void
SGSoundSample::set_source_vel( ALfloat *vel ) {
    source_vel[0] = vel[0];
    source_vel[1] = vel[1];
    source_vel[2] = vel[2];
    if (playing) {
        alSourcefv( source, AL_VELOCITY, source_vel );
    }
}

void
SGSoundSample::set_reference_dist( ALfloat dist ) {
    reference_dist = dist;
    if (playing) {
        alSourcef( source, AL_REFERENCE_DISTANCE, reference_dist );
    }
}


void
SGSoundSample::set_max_dist( ALfloat dist ) {
    max_dist = dist;
    if (playing) {
        alSourcef( source, AL_MAX_DISTANCE, max_dist );
    }
}

ALvoid *
SGSoundSample::load_file(const char *path, const char *file)
{
    ALvoid* data = 0;

    SGPath samplepath( path );
    if ( strlen(file) ) {
        samplepath.append( file );
    }

#if defined(ALUT_API_MAJOR_VERSION) && ALUT_API_MAJOR_VERSION >= 1
    ALfloat freqf;
    data = alutLoadMemoryFromFile(samplepath.c_str(), &format, &size, &freqf );
    if (data == NULL) {
        throw sg_exception("Failed to load wav file.");
    }
    freq = (ALsizei)freqf;
#else
# if defined (__APPLE__)
    alutLoadWAVFile( (ALbyte *)samplepath.c_str(),
                     &format, &data, &size, &freq );
# else
    alutLoadWAVFile( (ALbyte *)samplepath.c_str(),
                     &format, &data, &size, &freq, &loop );
# endif
    if ( print_openal_error("constructor (alutLoadWAVFile)") ) {
        throw sg_exception("Failed to load wav file.");
    }
#endif

    return data;
}

