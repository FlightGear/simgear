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


int main( int argc, char *argv[] ) {
    SGSampleGroup *sgr;
    SGSoundMgr *smgr;

    smgr = new SGSoundMgr;

    smgr->bind();
    smgr->init();
    sgr = smgr->find("default", true);
    smgr->set_volume(0.9);
    smgr->set_position( SGVec3d::fromGeod(SGGeod::fromDeg(0,0)) );
    smgr->activate();

    printf("default position and orientation\n");
    SGSoundSample *sample1 = new SGSoundSample( SRC_DIR, "jet.wav" );
    sample1->set_volume(1.0);
    sample1->set_pitch(1.0);
    sample1->play_looped();
    sgr->add(sample1, "sound1");
    smgr->update(1.0);
    printf("playing sample\n");
    sleep(3);
    sgr->stop("sound1");
    smgr->update(3.0);
    sleep(1);

    printf("source at lat,lon = (10,-10), listener at (0.999,-0.999)\n");
    sample1->set_position( SGGeod::fromDeg(10,-10) );
    smgr->set_position( SGVec3d::fromGeod(SGGeod::fromDeg(9.99,-9.99)) );
    sample1->play_looped();
    smgr->update(1.0);
    printf("playing sample\n");
    sleep(3);
    sgr->stop("sound1");
    smgr->update(3.0);
    sleep(1);

    smgr->unbind();
    sleep(2);
    delete smgr;
}
