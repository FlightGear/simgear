// sample.cxx -- Sound sample encapsulation class
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


static void print_openal_error( ALuint error ) {
    if ( error == AL_INVALID_NAME ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "AL_INVALID_NAME" );
    } else if ( error == AL_ILLEGAL_ENUM ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "AL_ILLEGAL_ENUM" );
    } else if ( error == AL_INVALID_VALUE ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "AL_INVALID_VALUE" );
    } else if ( error == AL_ILLEGAL_COMMAND ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "AL_ILLEGAL_COMMAND" );
    } else if ( error == AL_OUT_OF_MEMORY ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "AL_OUT_OF_MEMORY" );
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Unhandled error code = " << error );
    }
}


// constructor
SGSoundSample::SGSoundSample( const char *path, const char *file ) :
    pitch(1.0),
    volume(1.0),
    loop(AL_FALSE)
{
    SGPath samplepath( path );
    if ( strlen(file) ) {
        samplepath.append( file );
    }
     
    SG_LOG( SG_GENERAL, SG_ALERT, "From file sounds sample = "
            << samplepath.str() );

    ALuint error;

    source_pos[0] = 0.0; source_pos[1] = 0.0; source_pos[2] = 0.0;
    source_vel[0] = 0.0; source_vel[1] = 0.0; source_vel[2] = 0.0;

    // clear errors from elsewhere?
    alGetError();

    // create an OpenAL buffer handle
    alGenBuffers(1, &buffer);
    error = alGetError();
    if ( error != AL_NO_ERROR ) {
        print_openal_error( error );
        throw sg_exception("Failed to gen OpenAL buffer.");
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Buffer created ok!" );
    }

    // Load the sample file
#if defined (__APPLE__)
    alutLoadWAVFile( (ALbyte *)samplepath.c_str(),
                     &format, &data, &size, &freq );
#else
    alutLoadWAVFile( (ALbyte *)samplepath.c_str(),
                     &format, &data, &size, &freq, &loop );
#endif
    if (alGetError() != AL_NO_ERROR) {
        throw sg_exception("Failed to load wav file.");
    }

    // Copy data to the internal OpenAL buffer
    alBufferData( buffer, format, data, size, freq );
    if (alGetError() != AL_NO_ERROR) {
        throw sg_exception("Failed to buffer data.");
    }

    alutUnloadWAV( format, data, size, freq );

    // Bind buffer with a source.
    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        throw sg_exception("Failed to gen source.");
    }

    alSourcei( source, AL_BUFFER, buffer );
    alSourcef( source, AL_PITCH, pitch );
    alSourcef( source, AL_GAIN, volume );
    alSourcefv( source, AL_POSITION, source_pos );
    alSourcefv( source, AL_VELOCITY, source_vel );
    alSourcei( source, AL_LOOPING, loop );
}


// constructor
SGSoundSample::SGSoundSample( unsigned char *_data, int len, int _freq ) :
    pitch(1.0),
    volume(1.0),
    loop(AL_FALSE)
{
    SG_LOG( SG_GENERAL, SG_ALERT, "In memory sounds sample" );

    source_pos[0] = 0.0; source_pos[1] = 0.0; source_pos[2] = 0.0;
    source_vel[0] = 0.0; source_vel[1] = 0.0; source_vel[2] = 0.0;

    // Load wav data into a buffer.
    alGenBuffers(1, &buffer);
    if (alGetError() != AL_NO_ERROR) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Error in alGenBuffers()" );
        return;
    }

    format = AL_FORMAT_MONO8;
    size = len;
    data = _data;
    freq = _freq;

    alBufferData( buffer, format, data, size, freq );

    // Bind buffer with a source.
    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        throw sg_exception("Failed to gen source.");
    }

    alSourcei( source, AL_BUFFER, buffer );
    alSourcef( source, AL_PITCH, pitch );
    alSourcef( source, AL_GAIN, volume );
    alSourcefv( source, AL_POSITION, source_pos );
    alSourcefv( source, AL_VELOCITY, source_vel );
    alSourcei( source, AL_LOOPING, loop );
}


// destructor
SGSoundSample::~SGSoundSample() {
    SG_LOG( SG_GENERAL, SG_ALERT, "Deleting a sample" );
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
}


// play the sample
void SGSoundSample::play( bool _loop ) {
    loop = _loop;
    
    // make sure sound isn't already playing
    alSourceStop( source );

    alSourcei( source, AL_LOOPING, loop );
    alSourcePlay( source );
}


// stop playing the sample
void SGSoundSample::stop() {
    alSourceStop( source );
}
