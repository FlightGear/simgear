#include <stdio.h>

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


int main( int argc, char *argv[] ) {
    // initialize OpenAL
    alutInit( 0, NULL );
    alGetError();
    if ( alGetError() != AL_NO_ERROR) {
	SG_LOG( SG_GENERAL, SG_ALERT, "Audio initialization failed!" );
    }

    // Position of the listener.
    ALfloat listener_pos[3];

    // Velocity of the listener.
    ALfloat listener_vel[3];

    // Orientation of the listener. (first 3 elements are "at", second
    // 3 are "up")
    ALfloat listener_ori[6];

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
    ALboolean loop;

    source_pos[0] = 0.0; source_pos[1] = 0.0; source_pos[2] = 0.0;
    source_vel[0] = 0.0; source_vel[1] = 0.0; source_vel[2] = 0.0;

    // create an OpenAL buffer handle
    alGenBuffers(1, &buffer);
    ALuint error = alGetError();
    if ( error != AL_NO_ERROR ) {
        print_openal_error( error );
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to gen OpenAL buffer." );
    } else {
        SG_LOG( SG_GENERAL, SG_ALERT, "Buffer created ok!" );
    }

    // Load the sample file
#if defined (__APPLE__)
    alutLoadWAVFile( (ALbyte *)"jet.wav", &format, &data, &size, &freq );
#else
    alutLoadWAVFile( (ALbyte *)"jet.wav", &format, &data, &size, &freq, &loop );
#endif
    if (alGetError() != AL_NO_ERROR) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to load wav file.");
    }

    // Copy data to the internal OpenAL buffer
    alBufferData( buffer, format, data, size, freq );
    if (alGetError() != AL_NO_ERROR) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to buffer data.");
    }

    alutUnloadWAV( format, data, size, freq );

    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        print_openal_error( error );
    }

    alSourcei( source, AL_BUFFER, buffer );
    alSourcef( source, AL_PITCH, 1.0 );
    alSourcef( source, AL_GAIN, 1.0 );
    alSourcefv( source, AL_POSITION, source_pos );
    alSourcefv( source, AL_VELOCITY, source_vel );
    alSourcei( source, AL_LOOPING, loop );

    alSourcePlay( source );

    sleep(10);

    return 0;
}
