#include <simgear_config.h>

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x*1000)
#else
#include <unistd.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/timing/timestamp.hxx>

#include "soundmgr.hxx"
#include "sample_group.hxx"
#include "sample.hxx"


int main( int argc, char *argv[] ) {
    SGSampleGroup *sgr;
    SGSoundMgr *smgr;

    smgr = new SGSoundMgr;

    smgr->bind();
    smgr->init();

    sgr = smgr->find("default", true);
    sgr->tie_to_listener();

    smgr->set_volume(0.9);
    smgr->activate();

    // prevent NaNs
    smgr->set_position( SGVec3d(0, 0, 0), SGGeod::fromDegFt(0, 0, 0) );
    smgr->set_orientation( SGQuatd::fromYawPitchRollDeg(0, 0, 0) );

    // Move the samples to the listener
    smgr->update(0.0);

    SGPath srcDir(SRC_DIR);

    SGSoundSample *sample1 = new SGSoundSample("jet_ulaw.wav", srcDir);
    sample1->set_volume(1.0);
    sample1->set_pitch(1.0);
    sample1->play_looped();
    sgr->add(sample1, "sound1");
    smgr->update(1.0);
    printf("playing sample1\n");
    sleep(1);

    SGSoundSample *sample2 = new SGSoundSample("jet_ulaw.wav", srcDir);
    sample2->set_volume(0.5);
    sample2->set_pitch(0.4);
    sample2->play_looped();
    sgr->add(sample2, "sound2");
    smgr->update(1.0);
    printf("playing sample2\n");
    sleep(1);

    printf("Note: OpenAL-Soft does not have native support for IMA4 encoded audio.\n");
    SGSoundSample *sample3 = new SGSoundSample("jet_ima4.wav", srcDir);
    sample3->set_volume(0.5);
    sample3->set_pitch(0.8);
    sample3->play_looped();
    sgr->add(sample3, "sound3");
    smgr->update(1.0);
    printf("playing sample3\n");
    sleep(1);

    SGSoundSample *sample4 = new SGSoundSample("jet.wav", srcDir);
    sample4->set_volume(0.5);
    sample4->set_pitch(1.2);
    sample4->play_looped();
    sgr->add(sample4, "sound4");
    smgr->update(1.0);
    printf("playing sample4\n");
    sleep(1);

    SGSoundSample *sample5 = new SGSoundSample("jet.wav", srcDir);
    sample5->set_volume(0.5);
    sample5->set_pitch(1.6);
    sample5->play_looped();
    sgr->add(sample5, "sound5");
    smgr->update(1.0);
    printf("playing sample5\n");
    sleep(1);

    SGSoundSample *sample6 = new SGSoundSample("jet.wav", srcDir);
    sample6->set_volume(0.5);
    sample6->set_pitch(2.0);
    sample6->play_looped();
    sgr->add(sample6, "sound6");
    smgr->update(1.0);
    printf("playing sample6\n");
    sleep(1);

    for (int i=0; i<10; i++) {
        sleep(1);
        smgr->update(1);
    }

    sgr->stop("sound1");
    sgr->stop("sound2");
    sgr->stop("sound3");
    SGTimeStamp::sleepForMSec(500);
    smgr->update(0.5);
    sgr->stop("sound4");
    sgr->stop("sound5");
    sgr->stop("sound6");
    smgr->update(1);
    sleep(1);

    smgr->unbind();
    sleep(2);
    delete smgr;
}
