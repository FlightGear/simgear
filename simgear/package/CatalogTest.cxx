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
#include <fstream>

#include <simgear/package/Catalog.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Install.hxx>

#include <simgear/misc/test_macros.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/timing/timestamp.hxx>

#include <simgear/io/test_HTTP.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/structure/exception.hxx>

using namespace simgear;

std::string readFileIntoString(const SGPath& path)
{
    std::string contents;

    size_t readLen;
    SGBinaryFile f(path);
    if (!f.open(SG_IO_IN)) {
        throw sg_io_exception("Couldn't open file", path);
    }

    char buf[8192];
    while ((readLen = f.read(buf, 8192)) > 0) {
        contents += std::string(buf, readLen);
    }

    return contents;
}

SGPath global_serverFilesRoot;
unsigned int global_catalogVersion = 0;

class TestPackageChannel : public TestServerChannel
{
public:

    virtual void processRequestHeaders()
    {
        state = STATE_IDLE;
        SGPath localPath(global_serverFilesRoot);


        if (path == "/catalogTest1/catalog.xml") {
            if (global_catalogVersion > 0) {
                std::stringstream ss;
                ss << "/catalogTest1/catalog-v" << global_catalogVersion << ".xml";
                path = ss.str();
            }
        }

        localPath.append(path);

      //  SG_LOG(SG_IO, SG_INFO, "local path is:" << localPath.str());

        if (localPath.exists()) {
            std::string content = readFileIntoString(localPath);
            std::stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << content.size() << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << content;

            std::string ds(d.str());
            bufferSend(ds.data(), ds.size());
        } else {
            sendErrorResponse(404, false, "");
        }
    }
};

TestServer<TestPackageChannel> testServer;

void waitForUpdateComplete(HTTP::Client* cl, pkg::Root* root)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  10000) {
        cl->update();
        testServer.poll();

        if (!cl->hasActiveRequests()) {
            return;
        }

        SGTimeStamp::sleepForMSec(15);
    }

    std::cerr << "timed out" << std::endl;
}

int parseTest()
{
    SGPath rootPath = simgear::Dir::current().path();
    rootPath.append("testRoot");
    pkg::Root* root = new pkg::Root(rootPath, "8.1.12");
    pkg::CatalogRef cat = pkg::Catalog::createFromPath(root, SGPath(SRC_DIR "/catalogTest1"));

    VERIFY(cat.valid());

    COMPARE(cat->id(), "org.flightgear.test.catalog1");
    COMPARE(cat->url(), "http://localhost:2000/catalogTest1/catalog.xml");
    COMPARE(cat->description(), "First test catalog");

// check the packages too
    COMPARE(cat->packages().size(), 4);

    pkg::PackageRef p1 = cat->packages().front();
    COMPARE(p1->catalog(), cat.ptr());

    COMPARE(p1->id(), "alpha");
    COMPARE(p1->qualifiedId(), "org.flightgear.test.catalog1.alpha");
    COMPARE(p1->name(), "Alpha package");
    COMPARE(p1->revision(), 8);
    COMPARE(p1->fileSizeBytes(), 593);


    pkg::PackageRef p2 = cat->getPackageById("c172p");
    VERIFY(p2.valid());
    COMPARE(p2->qualifiedId(), "org.flightgear.test.catalog1.c172p");
    COMPARE(p2->description(), "A plane made by Cessna");

    pkg::Package::ThumbnailVec thumbs = p2->thumbnailsForVariant(0);
    COMPARE(thumbs.size(), 3);

    auto index = std::find_if(thumbs.begin(), thumbs.end(), [](const pkg::Package::Thumbnail& t)
                             { return (t.type == pkg::Package::Thumbnail::Type::EXTERIOR); });
    VERIFY(index != thumbs.end());
    COMPARE(index->path, "thumb-exterior.png");
    COMPARE(index->url, "http://foo.bar.com/thumb-exterior.png");
    VERIFY(index->type == pkg::Package::Thumbnail::Type::EXTERIOR);

    index = std::find_if(thumbs.begin(), thumbs.end(), [](const pkg::Package::Thumbnail& t)
                        { return (t.type == pkg::Package::Thumbnail::Type::PANEL); });
    VERIFY(index != thumbs.end());
    COMPARE(index->path, "thumb-panel.png");
    COMPARE(index->url, "http://foo.bar.com/thumb-panel.png");
    VERIFY(index->type == pkg::Package::Thumbnail::Type::PANEL);

// test variants
    try {
        p2->indexOfVariant("fofofo");
        SG_TEST_FAIL("lookup of non-existant variant did not throw");
    } catch (sg_exception& e) {
      // expected
    }

    unsigned int skisVariantFull = p2->indexOfVariant("org.flightgear.test.catalog1.c172p-skis");
    VERIFY(skisVariantFull > 0);

    unsigned int skisVariant = p2->indexOfVariant("c172p-skis");
    VERIFY(skisVariant > 0);

    COMPARE(skisVariant, skisVariantFull);

    pkg::Package::ThumbnailVec thumbs2 = p2->thumbnailsForVariant(skisVariant);
    COMPARE(thumbs2.size(), 2);

    index = std::find_if(thumbs2.begin(), thumbs2.end(), [](const pkg::Package::Thumbnail& t)
                              { return (t.type == pkg::Package::Thumbnail::Type::EXTERIOR); });
    VERIFY(index != thumbs2.end());
    COMPARE(index->path, "thumb-exterior-skis.png");
    COMPARE(index->url, "http://foo.bar.com/thumb-exterior-skis.png");
    VERIFY(index->type == pkg::Package::Thumbnail::Type::EXTERIOR);


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

void testAddCatalog(HTTP::Client* cl)
{
// erase dir
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("pkg_add_catalog");
    simgear::Dir pd(rootPath);
    pd.removeChildren();


    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    // specify a test dir
    root->setHTTPClient(cl);

    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");

    waitForUpdateComplete(cl, root);

// verify on disk state
    SGPath p(rootPath);
    p.append("org.flightgear.test.catalog1");
    p.append("catalog.xml");
    VERIFY(p.exists());
    COMPARE(root->allPackages().size(), 4);
    COMPARE(root->catalogs().size(), 1);

    pkg::PackageRef p1 = root->getPackageById("alpha");
    COMPARE(p1->id(), "alpha");

    pkg::PackageRef p2 = root->getPackageById("org.flightgear.test.catalog1.c172p");
    COMPARE(p2->id(), "c172p");
    COMPARE(p2->qualifiedId(), "org.flightgear.test.catalog1.c172p");

}

void testInstallPackage(HTTP::Client* cl)
{
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("pkg_install_with_dep");
    simgear::Dir pd(rootPath);
    pd.removeChildren();

    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    // specify a test dir
    root->setHTTPClient(cl);

    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
    waitForUpdateComplete(cl, root);

    pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.c172p");
    pkg::InstallRef ins = p1->install();

    VERIFY(ins->isQueued());

    waitForUpdateComplete(cl, root);
    VERIFY(p1->isInstalled());
    VERIFY(p1->existingInstall() == ins);

    pkg::PackageRef commonDeps = root->getPackageById("common-sounds");
    VERIFY(commonDeps->existingInstall());

    // verify on disk state
    SGPath p(rootPath);
    p.append("org.flightgear.test.catalog1");
    p.append("Aircraft");
    p.append("c172p");

    COMPARE(p, ins->path());

    p.append("c172p-floats-set.xml");
    VERIFY(p.exists());

    SGPath p2(rootPath);
    p2.append("org.flightgear.test.catalog1");
    p2.append("Aircraft");
    p2.append("sounds");
    p2.append("sharedfile.txt");
    VERIFY(p2.exists());
}

void testUninstall(HTTP::Client* cl)
{
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("pkg_uninstall");
    simgear::Dir pd(rootPath);
    pd.removeChildren();

    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    root->setHTTPClient(cl);

    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
    waitForUpdateComplete(cl, root);

    pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.c172p");
    pkg::InstallRef ins = p1->install();

    waitForUpdateComplete(cl, root);

    VERIFY(p1->isInstalled());

    ins->uninstall();

    VERIFY(!ins->path().exists());
}

void testRemoveCatalog(HTTP::Client* cl)
{
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("pkg_catalog_remove");
    simgear::Dir pd(rootPath);
    pd.removeChildren();

    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    root->setHTTPClient(cl);

    {
        pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
        waitForUpdateComplete(cl, root);
    }

    {
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.c172p");
        pkg::InstallRef ins = p1->install();

        waitForUpdateComplete(cl, root);

        VERIFY(p1->isInstalled());
    }

    root->removeCatalogById("org.flightgear.test.catalog1");


    SGPath p2(rootPath);
    p2.append("org.flightgear.test.catalog1");
    VERIFY(!p2.exists());

    VERIFY(root->allPackages().empty());
    VERIFY(root->catalogs().empty());

    pkg::CatalogRef c = root->getCatalogById("org.flightgear.test.catalog1");
    COMPARE(c, pkg::CatalogRef());
}

template <class T>
bool contains(const std::vector<T>& vec, const T& item)
{
    typename std::vector<T>::const_iterator it = std::find(vec.begin(), vec.end(), item);
    return (it != vec.end());
}

void testRefreshCatalog(HTTP::Client* cl)
{
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("pkg_catalog_refresh_update");
    simgear::Dir pd(rootPath);
    pd.removeChildren();

    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    root->setHTTPClient(cl);

    {
        pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
        waitForUpdateComplete(cl, root);
    }

    {
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.c172p");
        pkg::InstallRef ins = p1->install();

        pkg::PackageRef p2 = root->getPackageById("org.flightgear.test.catalog1.alpha");
        pkg::InstallRef ins2 = p2->install();

        waitForUpdateComplete(cl, root);

        VERIFY(p1->isInstalled());
        VERIFY(p2->isInstalled());
    }

    VERIFY(root->packagesNeedingUpdate().empty());

    global_catalogVersion = 2;

    VERIFY(!cl->hasActiveRequests());
    root->refresh();

    // should be a no-op due to catalog age testing
    VERIFY(!cl->hasActiveRequests());

    // force it this time
    root->refresh(true);
    VERIFY(cl->hasActiveRequests());
    waitForUpdateComplete(cl, root);

    pkg::CatalogRef c = root->getCatalogById("org.flightgear.test.catalog1");
    COMPARE(c->ageInSeconds(), 0);

    VERIFY(root->getPackageById("dc3") != pkg::PackageRef());
    COMPARE(root->packagesNeedingUpdate().size(), 2);

    pkg::PackageList needingUpdate = root->packagesNeedingUpdate();
    VERIFY(contains(needingUpdate, root->getPackageById("common-sounds")));
    VERIFY(contains(needingUpdate, root->getPackageById("org.flightgear.test.catalog1.alpha")));

    pkg::InstallRef ins = root->getPackageById("alpha")->existingInstall();
    VERIFY(ins->hasUpdate());

    for (pkg::PackageList::const_iterator it = needingUpdate.begin();
         it != needingUpdate.end(); ++it)
    {
        root->scheduleToUpdate((*it)->existingInstall());
    }

    waitForUpdateComplete(cl, root);

    VERIFY(root->packagesNeedingUpdate().empty());
    COMPARE(root->getPackageById("common-sounds")->revision(), 11);
}

void testInstallTarPackage(HTTP::Client* cl)
{
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("pkg_install_tar");
    simgear::Dir pd(rootPath);
    pd.removeChildren();

    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    // specify a test dir
    root->setHTTPClient(cl);

    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
    waitForUpdateComplete(cl, root);

    pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
    COMPARE(p1->id(), "b737-NG");
    pkg::InstallRef ins = p1->install();

    VERIFY(ins->isQueued());

    waitForUpdateComplete(cl, root);
    VERIFY(p1->isInstalled());
    VERIFY(p1->existingInstall() == ins);

    // verify on disk state
    SGPath p(rootPath);
    p.append("org.flightgear.test.catalog1");
    p.append("Aircraft");
    p.append("b737NG");

    COMPARE(p, ins->path());

    p.append("b737-900-set.xml");
    VERIFY(p.exists());
}


int main(int argc, char* argv[])
{
    sglog().setLogLevels( SG_ALL, SG_DEBUG );

    HTTP::Client cl;
    cl.setMaxConnections(1);

    global_serverFilesRoot = SGPath(SRC_DIR);

    testAddCatalog(&cl);

    parseTest();

    testInstallPackage(&cl);

    testUninstall(&cl);

    testRemoveCatalog(&cl);

    testRefreshCatalog(&cl);

    testInstallTarPackage(&cl);

    std::cout << "Successfully passed all tests!" << std::endl;
    return EXIT_SUCCESS;
}
