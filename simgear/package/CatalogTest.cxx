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

    SG_VERIFY(cat.valid());

    SG_CHECK_EQUAL(cat->id(), "org.flightgear.test.catalog1");
    SG_CHECK_EQUAL(cat->url(), "http://localhost:2000/catalogTest1/catalog.xml");
    SG_CHECK_EQUAL(cat->description(), "First test catalog");

// check the packages too
    SG_CHECK_EQUAL(cat->packages().size(), 4);

    pkg::PackageRef p1 = cat->packages().front();
    SG_CHECK_EQUAL(p1->catalog(), cat.ptr());

    SG_CHECK_EQUAL(p1->id(), "alpha");
    SG_CHECK_EQUAL(p1->qualifiedId(), "org.flightgear.test.catalog1.alpha");
    SG_CHECK_EQUAL(p1->name(), "Alpha package");
    SG_CHECK_EQUAL(p1->revision(), 8);
    SG_CHECK_EQUAL(p1->fileSizeBytes(), 593);


    pkg::PackageRef p2 = cat->getPackageById("c172p");
    SG_VERIFY(p2.valid());
    SG_CHECK_EQUAL(p2->qualifiedId(), "org.flightgear.test.catalog1.c172p");
    SG_CHECK_EQUAL(p2->description(), "A plane made by Cessna on Jupiter");

    pkg::Package::PreviewVec thumbs = p2->previewsForVariant(0);
    SG_CHECK_EQUAL(thumbs.size(), 3);

    auto index = std::find_if(thumbs.begin(), thumbs.end(), [](const pkg::Package::Preview& t)
                             { return (t.type == pkg::Package::Preview::Type::EXTERIOR); });
    SG_VERIFY(index != thumbs.end());
    SG_CHECK_EQUAL(index->path, "thumb-exterior.png");
    SG_CHECK_EQUAL(index->url, "http://foo.bar.com/thumb-exterior.png");
    SG_VERIFY(index->type == pkg::Package::Preview::Type::EXTERIOR);

    index = std::find_if(thumbs.begin(), thumbs.end(), [](const pkg::Package::Preview& t)
                        { return (t.type == pkg::Package::Preview::Type::PANEL); });
    SG_VERIFY(index != thumbs.end());
    SG_CHECK_EQUAL(index->path, "thumb-panel.png");
    SG_CHECK_EQUAL(index->url, "http://foo.bar.com/thumb-panel.png");
    SG_VERIFY(index->type == pkg::Package::Preview::Type::PANEL);

// old-style thumbnails
    string_list oldThumbUrls = p2->thumbnailUrls();
    SG_CHECK_EQUAL(oldThumbUrls.size(), 1);
    SG_CHECK_EQUAL(oldThumbUrls.at(0), "http://foo.bar.com/thumb-exterior.png");

    string_list oldThumbPaths = p2->thumbnails();
    SG_CHECK_EQUAL(oldThumbPaths.size(), 1);
    SG_CHECK_EQUAL(oldThumbPaths.at(0), "exterior.png");

// test variants
    try {
        p2->indexOfVariant("fofofo");
        SG_TEST_FAIL("lookup of non-existant variant did not throw");
    } catch (sg_exception& e) {
      // expected
    }

    unsigned int skisVariantFull = p2->indexOfVariant("org.flightgear.test.catalog1.c172p-skis");
    SG_VERIFY(skisVariantFull > 0);

    unsigned int skisVariant = p2->indexOfVariant("c172p-skis");
    SG_VERIFY(skisVariant > 0);

    SG_CHECK_EQUAL(skisVariant, skisVariantFull);

    SG_CHECK_EQUAL(p2->getLocalisedProp("description", skisVariant),
                   "A plane with skis");
    SG_CHECK_EQUAL(p2->getLocalisedProp("author", skisVariant),
                   "Standard author");

    unsigned int floatsVariant = p2->indexOfVariant("c172p-floats");
    SG_VERIFY(floatsVariant > 0);

    SG_CHECK_EQUAL(p2->getLocalisedProp("description", floatsVariant),
                   "A plane with floats");
    SG_CHECK_EQUAL(p2->getLocalisedProp("author", floatsVariant),
                   "Floats variant author");

    pkg::Package::PreviewVec thumbs2 = p2->previewsForVariant(skisVariant);
    SG_CHECK_EQUAL(thumbs2.size(), 2);

    index = std::find_if(thumbs2.begin(), thumbs2.end(), [](const pkg::Package::Preview& t)
                              { return (t.type == pkg::Package::Preview::Type::EXTERIOR); });
    SG_VERIFY(index != thumbs2.end());
    SG_CHECK_EQUAL(index->path, "thumb-exterior-skis.png");
    SG_CHECK_EQUAL(index->url, "http://foo.bar.com/thumb-exterior-skis.png");
    SG_VERIFY(index->type == pkg::Package::Preview::Type::EXTERIOR);


// test filtering / searching too
    string_set tags(p2->tags());
    SG_CHECK_EQUAL(tags.size(), 4);
    SG_VERIFY(tags.find("ifr") != tags.end());
    SG_VERIFY(tags.find("cessna") != tags.end());
    SG_VERIFY(tags.find("jet") == tags.end());


    SGPropertyNode_ptr queryA(new SGPropertyNode);
    queryA->setStringValue("tag", "ifr");
    SG_VERIFY(p2->matches(queryA.ptr()));

    SGPropertyNode_ptr queryB(new SGPropertyNode);
    queryB->setStringValue("name", "ces");
    SG_VERIFY(p2->matches(queryB.ptr()));

    SGPropertyNode_ptr queryC(new SGPropertyNode);
    queryC->setStringValue("name", "foo");
    SG_VERIFY(!p2->matches(queryC.ptr()));

    SGPropertyNode_ptr queryD(new SGPropertyNode);
    queryD->setIntValue("rating-FDM", 3);
    SG_VERIFY(p2->matches(queryD.ptr()));

    SGPropertyNode_ptr queryE(new SGPropertyNode);
    queryE->setIntValue("rating-model", 5);
    queryE->setStringValue("description", "cessna");
    SG_VERIFY(p2->matches(queryE.ptr()));

    {
        SGPropertyNode_ptr queryText(new SGPropertyNode);
        queryText->setStringValue("any-of/text", "jupiter");
        SG_VERIFY(p2->matches(queryText.ptr()));
    }

    {
        SGPropertyNode_ptr queryText(new SGPropertyNode);
        queryText->setStringValue("any-of/tag", "twin-engine");
        queryText->setStringValue("any-of/tag", "ga");
        SG_VERIFY(p2->matches(queryText.ptr()));
    }

    // match variant descriptions
    {
        SGPropertyNode_ptr queryText(new SGPropertyNode);
        queryText->setStringValue("any-of/description", "float");
        SG_VERIFY(p2->matches(queryText.ptr()));
    }
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
    SG_VERIFY(p.exists());
    SG_CHECK_EQUAL(root->allPackages().size(), 4);
    SG_CHECK_EQUAL(root->catalogs().size(), 1);

    pkg::PackageRef p1 = root->getPackageById("alpha");
    SG_CHECK_EQUAL(p1->id(), "alpha");

    pkg::PackageRef p2 = root->getPackageById("org.flightgear.test.catalog1.c172p");
    SG_CHECK_EQUAL(p2->id(), "c172p");
    SG_CHECK_EQUAL(p2->qualifiedId(), "org.flightgear.test.catalog1.c172p");

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

    SG_VERIFY(ins->isQueued());

    waitForUpdateComplete(cl, root);
    SG_VERIFY(p1->isInstalled());
    SG_VERIFY(p1->existingInstall() == ins);

    pkg::PackageRef commonDeps = root->getPackageById("common-sounds");
    SG_VERIFY(commonDeps->existingInstall());

    // verify on disk state
    SGPath p(rootPath);
    p.append("org.flightgear.test.catalog1");
    p.append("Aircraft");
    p.append("c172p");

    SG_CHECK_EQUAL(p, ins->path());

    p.append("c172p-floats-set.xml");
    SG_VERIFY(p.exists());

    SGPath p2(rootPath);
    p2.append("org.flightgear.test.catalog1");
    p2.append("Aircraft");
    p2.append("sounds");
    p2.append("sharedfile.txt");
    SG_VERIFY(p2.exists());
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

    SG_VERIFY(p1->isInstalled());

    ins->uninstall();

    SG_VERIFY(!ins->path().exists());
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

        SG_VERIFY(p1->isInstalled());
    }

    root->removeCatalogById("org.flightgear.test.catalog1");


    SGPath p2(rootPath);
    p2.append("org.flightgear.test.catalog1");
    SG_VERIFY(!p2.exists());

    SG_VERIFY(root->allPackages().empty());
    SG_VERIFY(root->catalogs().empty());

    pkg::CatalogRef c = root->getCatalogById("org.flightgear.test.catalog1");
    SG_CHECK_EQUAL(c, pkg::CatalogRef());
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

        SG_VERIFY(p1->isInstalled());
        SG_VERIFY(p2->isInstalled());
    }

    SG_VERIFY(root->packagesNeedingUpdate().empty());

    global_catalogVersion = 2;

    SG_VERIFY(!cl->hasActiveRequests());
    root->refresh();

    // should be a no-op due to catalog age testing
    SG_VERIFY(!cl->hasActiveRequests());

    // force it this time
    root->refresh(true);
    SG_VERIFY(cl->hasActiveRequests());
    waitForUpdateComplete(cl, root);

    pkg::CatalogRef c = root->getCatalogById("org.flightgear.test.catalog1");
    SG_CHECK_EQUAL(c->ageInSeconds(), 0);

    SG_VERIFY(root->getPackageById("dc3") != pkg::PackageRef());
    SG_CHECK_EQUAL(root->packagesNeedingUpdate().size(), 2);

    pkg::PackageList needingUpdate = root->packagesNeedingUpdate();
    SG_VERIFY(contains(needingUpdate, root->getPackageById("common-sounds")));
    SG_VERIFY(
      contains(needingUpdate,
               root->getPackageById("org.flightgear.test.catalog1.alpha")));

    pkg::InstallRef ins = root->getPackageById("alpha")->existingInstall();
    SG_VERIFY(ins->hasUpdate());

    for (pkg::PackageList::const_iterator it = needingUpdate.begin();
         it != needingUpdate.end(); ++it)
    {
        root->scheduleToUpdate((*it)->existingInstall());
    }

    waitForUpdateComplete(cl, root);

    SG_VERIFY(root->packagesNeedingUpdate().empty());
    SG_CHECK_EQUAL(root->getPackageById("common-sounds")->revision(), 11);
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
    SG_CHECK_EQUAL(p1->id(), "b737-NG");
    pkg::InstallRef ins = p1->install();

    SG_VERIFY(ins->isQueued());

    waitForUpdateComplete(cl, root);
    SG_VERIFY(p1->isInstalled());
    SG_VERIFY(p1->existingInstall() == ins);

    // verify on disk state
    SGPath p(rootPath);
    p.append("org.flightgear.test.catalog1");
    p.append("Aircraft");
    p.append("b737NG");

    SG_CHECK_EQUAL(p, ins->path());

    p.append("b737-900-set.xml");
    SG_VERIFY(p.exists());
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
