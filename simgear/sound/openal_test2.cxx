#include <stdio.h>
#ifdef __MINGW32__
// This is broken, but allows the file to compile without a POSIX
// environment.
static unsigned int sleep(unsigned int secs) { return 0; }
#else
#include <unistd.h>	// sleep()
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include "soundmgr_openal.hxx"


int main( int argc, char *argv[] ) {
    SGSampleGroup *sgr;
    SGSoundMgr *smgr;

    smgr = new SGSoundMgr;

    smgr->bind();
    smgr->init();
    sgr = smgr->find("default", true);
    smgr->set_volume(0.9);
    smgr->activate();

    SGSoundSample *sample1 = new SGSoundSample( SRC_DIR, "jet.wav" );
    sample1->set_volume(1.0);
    sample1->set_pitch(1.0);
    sample1->play_looped();
    sgr->add(sample1, "sound1");
    smgr->update_late(1.0);
    printf("playing sample1\n");
    sleep(1);

    SGSoundSample *sample2 = new SGSoundSample( SRC_DIR, "jet.wav" );
    sample2->set_volume(0.5);
    sample2->set_pitch(0.4);
    sample2->play_looped();
    sgr->add(sample2, "sound2");
    smgr->update_late(1.0);
    printf("playing sample2\n");
    sleep(1);

    SGSoundSample *sample3 = new SGSoundSample( SRC_DIR, "jet.wav" );
    sample3->set_volume(0.5);
    sample3->set_pitch(0.8);
    sample3->play_looped();
    sgr->add(sample3, "sound3");
    smgr->update_late(1.0);
    printf("playing sample3\n");
    sleep(1);

    SGSoundSample *sample4 = new SGSoundSample( SRC_DIR, "jet.wav" );
    sample4->set_volume(0.5);
    sample4->set_pitch(1.2);
    sample4->play_looped();
    sgr->add(sample4, "sound4");
    smgr->update_late(1.0);
    printf("playing sample4\n");
    sleep(1);

    SGSoundSample *sample5 = new SGSoundSample( SRC_DIR, "jet.wav" );
    sample5->set_volume(0.5);
    sample5->set_pitch(1.6);
    sample5->play_looped();
    sgr->add(sample5, "sound5");
    smgr->update_late(1.0);
    printf("playing sample5\n");
    sleep(1);

    SGSoundSample *sample6 = new SGSoundSample( SRC_DIR, "jet.wav" );
    sample6->set_volume(0.5);
    sample6->set_pitch(2.0);
    sample6->play_looped();
    sgr->add(sample6, "sound6");
    smgr->update_late(1.0);
    printf("playing sample6\n");
    sleep(1);

    for (int i=0; i<10; i++) {
        sleep(1);
        smgr->update_late(1);
    }

    sgr->stop("sound1");
    sgr->stop("sound2");
    sgr->stop("sound3");
    sleep(0.5);
    smgr->update_late(0.5);
    sgr->stop("sound4");
    sgr->stop("sound5");
    sgr->stop("sound6");
    smgr->update_late(1);
    sleep(1);

    smgr->unbind();
    sleep(2);
    delete smgr;
}
