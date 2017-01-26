#include <cstdlib>

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

int main(int argc, char **argv)
{
    test_isNull();
    test_setRemoveOnDestroy();
    test_tempDir();

    return EXIT_SUCCESS;
}
