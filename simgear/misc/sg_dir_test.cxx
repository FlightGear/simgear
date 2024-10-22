// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2010 James Turner
// SPDX-FileCopyrightText: 2010 Curtis L. Olson

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
    SG_VERIFY(!d.exists());
    SG_VERIFY(d.isEmpty());     // eek, but that's how it is
}

void test_hiddenChildren()
{
    simgear::Dir d = simgear::Dir::tempDir("FlightGear");
    SG_VERIFY(!d.isNull() && d.exists() && d.isEmpty());

    {
        sg_ofstream file(d.file(".hiddenFile"));
    }
    {
        sg_ofstream file(d.file("regularFile"));
    }


    const auto c1 = d.children();
    SG_VERIFY(c1.size() == 1);
    SG_VERIFY(c1.front() == d.file("regularFile"));

    const auto c2 = d.children(simgear::Dir::INCLUDE_HIDDEN | simgear::Dir::TYPE_FILE | simgear::Dir::TYPE_DIR | simgear::Dir::NO_DOT_OR_DOTDOT);
    SG_VERIFY(c2.size() == 2);

    const auto c3 = d.children(simgear::Dir::INCLUDE_HIDDEN | simgear::Dir::TYPE_FILE | simgear::Dir::TYPE_DIR);
    SG_VERIFY(c3.size() == 4);
}

int main(int argc, char **argv)
{
    test_isNull();
    test_setRemoveOnDestroy();
    test_tempDir();
    test_isEmpty();
    test_hiddenChildren();

    return EXIT_SUCCESS;
}


