// Copyright (C) 2015  James Turner - zakalawe@mac.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/misc/test_macros.hxx>

#include <cstdlib>
#include <iostream>

#include <simgear/package/Catalog.hxx>
#include <simgear/package/Root.hxx>

#include <simgear/misc/sg_dir.hxx>

using namespace simgear;

int parseTest()
{
    SGPath rootPath = simgear::Dir::current().path();
    rootPath.append("testRoot");
    pkg::Root* root = new pkg::Root(rootPath, "8.1.12");
    pkg::CatalogRef cat = pkg::Catalog::createFromPath(root, SGPath(SRC_DIR "/catalogTest1"));

    VERIFY(cat.valid());

    COMPARE(cat->id(), "org.flightgear.test.catalog1");
    COMPARE(cat->url(), "http://download.flightgear.org/catalog1/catalog.xml");
    COMPARE(cat->description(), "First test catalog");

// check the packages too

    delete root;

    return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
    parseTest();
    std::cout << "Successfully passed all tests!" << std::endl;
    return EXIT_SUCCESS;
}
