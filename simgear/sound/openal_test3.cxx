#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x*1000)
#else
#include <unistd.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include "soundmgr_openal.hxx"
#include "sample_group.hxx"
#include "sample_openal.hxx"

int main( int argc, char *argv[] ) {
    SGSampleGroup *sgr;
    SGSoundMgr *smgr;
    SGGeod pos;

    smgr = new SGSoundMgr;

    smgr->bind();
    smgr->init();
    sgr = smgr->find("default", true);
    smgr->set_volume(0.9);
    smgr->activate();

    SGPath srcDir(SRC_DIR);

    printf("default position and orientation\n");
    SGSoundSample *sample1 = new SGSoundSample("jet.wav", srcDir);
    sample1->set_volume(1.0);
    sample1->set_pitch(1.0);
    sample1->play_looped();
    sgr->add(sample1, "sound1");
    smgr->update(1.0);
    printf("playing sample\n");
    sleep(3);
    sample1->stop();
    smgr->update(3.0);
    sleep(1);

    printf("source at lat,lon = (10,-10), listener at (9.99,-9.99)\n");
    pos = SGGeod::fromDeg(9.99,-9.99);
    sample1->set_position( SGVec3d::fromGeod(SGGeod::fromDeg(10,-10)) );
    smgr->set_position( SGVec3d::fromGeod(pos), pos );
    sample1->play_looped();
    smgr->update(1.0);
    printf("playing sample\n");
    sleep(3);
    sample1->stop();
    smgr->update(3.0);
    sleep(1);

    sgr->remove("sound1");
    smgr->unbind();
    sleep(2);
    delete smgr;
}
