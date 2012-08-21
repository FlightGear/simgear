#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x*1000)
#else
#include <unistd.h>	// sleep()
#endif

#if defined( __APPLE__ )
# define AL_ILLEGAL_ENUM AL_INVALID_ENUM
# define AL_ILLEGAL_COMMAND AL_INVALID_OPERATION
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#elif defined(OPENALSDK)
# include <al.h>
# include <alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

#define AUDIOFILE	SRC_DIR"/jet.wav"

#include <simgear/sound/readwav.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

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


int main( int argc, char *argv[] ) 
{
    sglog().setLogLevels( SG_ALL, SG_ALERT );
    
    // initialize OpenAL
    ALCdevice *dev = alcOpenDevice(NULL); 
    if (!dev) {
      SG_LOG( SG_GENERAL, SG_ALERT, "Audio device initialization failed!" );
      return EXIT_FAILURE;
    }
    
    ALCcontext *context = alcCreateContext(dev, NULL);
    if (!context) {
      SG_LOG( SG_GENERAL, SG_ALERT, "Audio context initialization failed!" );
      return EXIT_FAILURE;
    }
    
    alcMakeContextCurrent( context );

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
    ALboolean loop = false;

    source_pos[0] = 0.0; source_pos[1] = 0.0; source_pos[2] = 0.0;
    source_vel[0] = 0.0; source_vel[1] = 0.0; source_vel[2] = 0.0;

    // Load the sample file
      buffer = simgear::createBufferFromFile(SGPath(AUDIOFILE));
      if (buffer == AL_NONE) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Failed to buffer data.");
      }

    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        ALuint error = alGetError();
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
    
     alcMakeContextCurrent(NULL);
     alcDestroyContext(context);
     alcCloseDevice(dev);
     
    return 0;
}
