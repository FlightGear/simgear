#include <simgear_config.h>

#include <cstdlib>

#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/test_macros.hxx>
#include "sg_dir.hxx"


void test_isNull()
{
    SG_VERIFY(simgear::Dir().isNull());
}

void test_setRemoveOnDestroy()
{
    SGPath path;
    {
        simgear::Dir d = simgear::Dir::tempDir("FlightGear");
        SG_VERIFY(!d.isNull() && d.exists() && d.isEmpty());
        d.setRemoveOnDestroy();

        path = d.path();        // keep a copy of the path
        SG_VERIFY(path.exists() && path.isDir());
    }

    SG_VERIFY(!path.exists());
}

void test_tempDir()
{
    simgear::Dir d = simgear::Dir::tempDir("FlightGear");
    SG_VERIFY(!d.isNull() && d.exists() && d.isEmpty());
    d.remove();
}

void test_isEmpty()
{
    simgear::Dir d = simgear::Dir::tempDir("FlightGear");
    SG_VERIFY(!d.isNull() && d.exists() && d.isEmpty());
    SGPath f = d.file("some file");

    { sg_ofstream file(f); }    // create and close the file
    SG_VERIFY(!d.isEmpty());

    f.remove();
    SG_VERIFY(d.isEmpty());

    simgear::Dir subDir{d.file("some subdir")};
    subDir.create(0777);
    SG_VERIFY(!d.isEmpty());

    subDir.remove();
    SG_VERIFY(d.isEmpty());

    d.remove();
    SG_VERIFY(d.isEmpty());     // eek, but that's how it is
}

int main(int argc, char **argv)
{
    test_isNull();
    test_setRemoveOnDestroy();
    test_tempDir();
    test_isEmpty();

    return EXIT_SUCCESS;
}
