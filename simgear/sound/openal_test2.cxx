#include <stdio.h>
#include <unistd.h>	// sleep()

#include "sample_openal.hxx"
#include "soundmgr_openal.hxx"


int main( int argc, char *argv[] ) {
    SGSoundMgr sm;

    SGSoundSample sample1( ".", "jet.wav", true );
    sample1.set_volume(0.5);
    sample1.set_volume(0.2);
    sample1.play_looped();
    sleep(1);

    SGSoundSample sample2( ".", "jet.wav", true );
    sample2.set_volume(0.5);
    sample2.set_pitch(0.4);
    sample2.play_looped();
    sleep(1);

    SGSoundSample sample3( ".", "jet.wav", true );
    sample3.set_volume(0.5);
    sample3.set_pitch(0.8);
    sample3.play_looped();
    sleep(1);

    SGSoundSample sample4( ".", "jet.wav", true );
    sample4.set_volume(0.5);
    sample4.set_pitch(1.2);
    sample4.play_looped();
    sleep(1);

    SGSoundSample sample5( ".", "jet.wav", true );
    sample5.set_volume(0.5);
    sample5.set_pitch(1.6);
    sample5.play_looped();
    sleep(1);

    SGSoundSample sample6( ".", "jet.wav", true );
    sample6.set_volume(0.5);
    sample6.set_pitch(2.0);
    sample6.play_looped();
    sleep(1);

    sleep(10);
}
