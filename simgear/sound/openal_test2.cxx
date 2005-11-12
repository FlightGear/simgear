#include <stdio.h>
#ifdef __MINGW32__
// This is broken, but allows the file to compile without a POSIX
// environment.
static unsigned int sleep(unsigned int secs) { return 0; }
#else
#include <unistd.h>	// sleep()
#endif

#include "sample_openal.hxx"
#include "soundmgr_openal.hxx"


int main( int argc, char *argv[] ) {
    SGSoundMgr sm;

    SGSoundSample sample1( ".", "jet.wav" );
    sample1.set_volume(0.5);
    sample1.set_volume(0.2);
    sample1.play_looped();
    sleep(1);

    SGSoundSample sample2( ".", "jet.wav" );
    sample2.set_volume(0.5);
    sample2.set_pitch(0.4);
    sample2.play_looped();
    sleep(1);

    SGSoundSample sample3( ".", "jet.wav" );
    sample3.set_volume(0.5);
    sample3.set_pitch(0.8);
    sample3.play_looped();
    sleep(1);

    SGSoundSample sample4( ".", "jet.wav" );
    sample4.set_volume(0.5);
    sample4.set_pitch(1.2);
    sample4.play_looped();
    sleep(1);

    SGSoundSample sample5( ".", "jet.wav" );
    sample5.set_volume(0.5);
    sample5.set_pitch(1.6);
    sample5.play_looped();
    sleep(1);

    SGSoundSample sample6( ".", "jet.wav" );
    sample6.set_volume(0.5);
    sample6.set_pitch(2.0);
    sample6.play_looped();
    sleep(1);

    sleep(10);
}
