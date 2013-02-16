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
    SGSampleQueue *squeue;
    SGSampleGroup *sgr;
    SGSoundMgr *smgr;
    SGGeod pos;

    smgr = new SGSoundMgr;

    smgr->bind();
    smgr->select_device("OSS Default");
    smgr->init();
    sgr = smgr->find("default", true);
    smgr->set_volume(0.9);
    smgr->activate();
    smgr->set_position( SGVec3d::fromGeod(SGGeod()), SGGeod() );

    void *data;
    size_t len;
    int freq, fmt;
    std::string file = SRC_DIR"/jet.wav";
    smgr->load(file, &data, &fmt, &len, &freq);

    squeue = new SGSampleQueue( freq, fmt );
    squeue->set_volume(1.0);
  
    sgr->add(squeue, "queue");

    squeue->add(  data, len );
    squeue->add(  data, len );
    squeue->play();
    printf("playing queue\n");

    smgr->update(1.0);
    sleep(10);
    smgr->update(10.0);

    printf("source at lat,lon = (10,-10), listener at (9.99,-9.99)\n");
    pos = SGGeod::fromDeg(9.99,-9.99);
    sgr->set_position_geod( SGGeod::fromDeg(10,-10) );
    smgr->set_position( SGVec3d::fromGeod(pos), pos );

    squeue->add(  data, len );
    squeue->add(  data, len );
    squeue->play( true ); // play looped
    printf("playing queue\n");

    smgr->update(1.0);
    sleep(10);
    smgr->update(10.0);

    squeue->stop();
    smgr->update(1.0);
    sleep(1);

    sgr->remove("queue");
    smgr->unbind();
    sleep(2);

    delete smgr;
}
