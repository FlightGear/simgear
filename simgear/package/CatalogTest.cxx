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
#include <simgear/package/Package.hxx>

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
    COMPARE(cat->packages().size(), 3);

    pkg::PackageRef p1 = cat->packages().front();
    COMPARE(p1->catalog(), cat.ptr());

    COMPARE(p1->id(), "alpha");
    COMPARE(p1->qualifiedId(), "org.flightgear.test.catalog1.alpha");
    COMPARE(p1->name(), "Alpha package");
    COMPARE(p1->revision(), 8);
    COMPARE(p1->fileSizeBytes(), 1234567);


    pkg::PackageRef p2 = cat->getPackageById("c172p");
    VERIFY(p2.valid());
    COMPARE(p2->qualifiedId(), "org.flightgear.test.catalog1.c172p");
    COMPARE(p2->description(), "A plane made by Cessna");



// test filtering / searching too
    string_set tags(p2->tags());
    COMPARE(tags.size(), 4);
    VERIFY(tags.find("ifr") != tags.end());
    VERIFY(tags.find("cessna") != tags.end());
    VERIFY(tags.find("jet") == tags.end());


    SGPropertyNode_ptr queryA(new SGPropertyNode);
    queryA->setStringValue("tag", "ifr");
    VERIFY(p2->matches(queryA.ptr()));

    SGPropertyNode_ptr queryB(new SGPropertyNode);
    queryB->setStringValue("name", "ces");
    VERIFY(p2->matches(queryB.ptr()));

    SGPropertyNode_ptr queryC(new SGPropertyNode);
    queryC->setStringValue("name", "foo");
    VERIFY(!p2->matches(queryC.ptr()));

    SGPropertyNode_ptr queryD(new SGPropertyNode);
    queryD->setIntValue("rating-FDM", 3);
    VERIFY(p2->matches(queryD.ptr()));

    SGPropertyNode_ptr queryE(new SGPropertyNode);
    queryE->setIntValue("rating-model", 5);
    queryE->setStringValue("description", "cessna");
    VERIFY(p2->matches(queryE.ptr()));


    delete root;
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
    parseTest();
    std::cout << "Successfully passed all tests!" << std::endl;
    return EXIT_SUCCESS;
}
