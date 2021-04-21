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

#include <simgear_config.h>

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
bool global_failRequests = false;
bool global_fail747Request = true;

int global_737RequestsToFailCount = 0;

class TestPackageChannel : public TestServerChannel
{
public:

    virtual void processRequestHeaders()
    {
        state = STATE_IDLE;
        SGPath localPath(global_serverFilesRoot);

        if (global_failRequests) {
            closeWhenDone();
            return;
        }

        if (path == "/catalogTest1/catalog.xml") {
            if (global_catalogVersion > 0) {
                std::stringstream ss;
                ss << "/catalogTest1/catalog-v" << global_catalogVersion << ".xml";
                path = ss.str();
            }
        }
        
        if (path == "/catalogTestInvalid/catalog.xml") {
            if (global_catalogVersion > 0) {
                std::stringstream ss;
                ss << "/catalogTestInvalid/catalog-v" << global_catalogVersion << ".xml";
                path = ss.str();
            }
        }
        
        // return zip data for this computed URL
        if (path.find("/catalogTest1/movies") == 0) {
            path = "/catalogTest1/movies-data.zip";
        }

        if (path == "/catalogTest1/b747.tar.gz") {
            if (global_fail747Request) {
                sendErrorResponse(403, false, "Bad URL");
                return;
            } else {
                path = "/catalogTest1/b747.tar.gz"; // valid path
            }
        }
        
        if ((path.find("/mirror") == 0) && (path.find("/b737.tar.gz") == 8)) {
            if (global_737RequestsToFailCount > 0) {
                sendErrorResponse(404, false, "Mirror failure");
                --global_737RequestsToFailCount;
                return;
            }
            path = "/catalogTest1/b737.tar.gz";
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

template<class T>
bool vectorContains(const std::vector<T>& vec, const T value)
{
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

int parseTest()
{
    SGPath rootPath = simgear::Dir::current().path();
    rootPath.append("testRoot");

    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    root->setLocale("de");
    pkg::CatalogRef cat = pkg::Catalog::createFromPath(root, SGPath(SRC_DIR "/catalogTest1"));

    SG_VERIFY(cat.valid());

    SG_CHECK_EQUAL(cat->id(), "org.flightgear.test.catalog1");
    SG_CHECK_EQUAL(cat->url(), "http://localhost:2000/catalogTest1/catalog.xml");
    SG_CHECK_EQUAL(cat->description(), "First test catalog");

// check the packages too
    SG_CHECK_EQUAL(cat->packages().size(), 4);
    SG_CHECK_EQUAL(cat->packages(simgear::pkg::LibraryPackage).size(), 2);
    SG_CHECK_EQUAL(cat->packages(simgear::pkg::AIModelPackage).size(), 1);

    pkg::PackageRef p1 = cat->packages().front();
    SG_CHECK_EQUAL(p1->catalog(), cat.ptr());

    SG_CHECK_EQUAL(p1->id(), "alpha");
    SG_CHECK_EQUAL(p1->qualifiedId(), "org.flightgear.test.catalog1.alpha");
    SG_CHECK_EQUAL(p1->name(), "Alpha package");
    SG_CHECK_EQUAL(p1->revision(), 8);
    SG_CHECK_EQUAL(p1->fileSizeBytes(), 593);
    SG_CHECK_EQUAL(p1->type(), simgear::pkg::AircraftPackage);


    pkg::PackageRef p2 = cat->getPackageById("c172p");
    SG_VERIFY(p2.valid());
    SG_CHECK_EQUAL(p2->qualifiedId(), "org.flightgear.test.catalog1.c172p");
    SG_CHECK_EQUAL(p2->description(), "German description of C172");

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

// thumbnails
    const pkg::Package::Thumbnail& thumb = p2->thumbnailForVariant(0);
    SG_CHECK_EQUAL(thumb.url, "http://foo.bar.com/thumb-exterior.png");
    SG_CHECK_EQUAL(thumb.path, "exterior.png");

// test variants
    SG_CHECK_EQUAL(p2->parentIdForVariant(0), std::string());

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

    SG_CHECK_EQUAL(p2->parentIdForVariant(skisVariantFull), "c172p");

    SG_CHECK_EQUAL(skisVariant, skisVariantFull);

    SG_CHECK_EQUAL(p2->getLocalisedProp("description", skisVariant),
                   "German description of C172 with skis");
    SG_CHECK_EQUAL(p2->getLocalisedProp("author", skisVariant),
                   "Standard author");

    unsigned int floatsVariant = p2->indexOfVariant("c172p-floats");
    SG_VERIFY(floatsVariant > 0);
    SG_CHECK_EQUAL(p2->parentIdForVariant(floatsVariant), "c172p");

    // no DE localisation description provided for the floats variant
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

    const pkg::Package::Thumbnail& thumb2 = p2->thumbnailForVariant(floatsVariant);
    SG_CHECK_EQUAL(thumb2.url, "http://foo.bar.com/thumb-floats.png");
    SG_CHECK_EQUAL(thumb2.path, "thumb-floats.png");

// test multiple primary
    unsigned int rVariant = p2->indexOfVariant("c172r");
    SG_VERIFY(rVariant > 0);

    SG_CHECK_EQUAL(p2->parentIdForVariant(rVariant), std::string());

    unsigned int rFloatVariant = p2->indexOfVariant("c172r-floats");
    SG_VERIFY(rFloatVariant > 0);
    SG_CHECK_EQUAL(p2->parentIdForVariant(rFloatVariant), std::string("c172r"));

    string_list primaries = {"c172p", "c172r"};
    SG_VERIFY(p2->primaryVariants() == primaries);

///////////
    pkg::PackageRef p3 = cat->getPackageById("b737-NG");
    SG_VERIFY(p3.valid());
    SG_CHECK_EQUAL(p3->description(), "German description of B737NG XYZ");
    
    
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
    
    // match localized variant descriptions
    {
        SGPropertyNode_ptr queryText(new SGPropertyNode);
        queryText->setStringValue("any-of/description", "XYZ");
        SG_VERIFY(p3->matches(queryText.ptr()));
    }
    
    string_list urls = p3->downloadUrls();
    SG_CHECK_EQUAL(urls.size(), 3);
    SG_CHECK_EQUAL(urls.at(1), "http://localhost:2000/mirrorB/b737.tar.gz");

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
    SG_CHECK_EQUAL(ins->status(), pkg::Delegate::STATUS_SUCCESS);

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

    SG_CHECK_EQUAL(ins->status(), pkg::Delegate::STATUS_SUCCESS);
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

void testInstallArchiveType(HTTP::Client* cl)
{
    global_catalogVersion = 0;
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("pkg_install_archive_type");
    simgear::Dir pd(rootPath);
    pd.removeChildren();
    
    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    // specify a test dir
    root->setHTTPClient(cl);
    
    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
    waitForUpdateComplete(cl, root);
    
    pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.movies");
    SG_CHECK_EQUAL(p1->id(), "movies");
    pkg::InstallRef ins = p1->install();
    
    SG_VERIFY(ins->isQueued());
    
    waitForUpdateComplete(cl, root);
    SG_VERIFY(p1->isInstalled());
    SG_VERIFY(p1->existingInstall() == ins);
    
    // verify on disk state
    SGPath p(rootPath);
    p.append("org.flightgear.test.catalog1");
    p.append("Aircraft");
    p.append("movies");
    
    SG_CHECK_EQUAL(p, ins->path());
    
    p.append("movie-list.json");
    SG_VERIFY(p.exists());
}

void testDisableDueToVersion(HTTP::Client* cl)
{
    global_catalogVersion = 0;
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("cat_disable_at_version");
    simgear::Dir pd(rootPath);
    pd.removeChildren();
    
    {
        pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
        root->setHTTPClient(cl);
        
        pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
        waitForUpdateComplete(cl, root);
        SG_VERIFY(c->isEnabled());

        // install a package
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
        SG_CHECK_EQUAL(p1->id(), "b737-NG");
        pkg::InstallRef ins = p1->install();
        SG_VERIFY(ins->isQueued());
        waitForUpdateComplete(cl, root);
        SG_VERIFY(p1->isInstalled());
    }
    
    // bump version and refresh
    {
        pkg::RootRef root(new pkg::Root(rootPath, "9.1.2"));
        pkg::CatalogRef cat = root->getCatalogById("org.flightgear.test.catalog1");
        SG_CHECK_EQUAL(root->allCatalogs().size(), 1);
        SG_VERIFY(!cat->isEnabled());
        SG_CHECK_EQUAL(root->catalogs().size(), 0);

        root->setHTTPClient(cl);
        root->refresh();
        waitForUpdateComplete(cl, root);
        SG_CHECK_EQUAL(root->allCatalogs().size(), 1);

        
        SG_CHECK_EQUAL(cat->status(), pkg::Delegate::FAIL_VERSION);
        SG_VERIFY(!cat->isEnabled());
        SG_CHECK_EQUAL(cat->id(), "org.flightgear.test.catalog1");
        
        auto enabledCats = root->catalogs();
        auto it = std::find(enabledCats.begin(), enabledCats.end(), cat);
        SG_VERIFY(it == enabledCats.end());
        SG_CHECK_EQUAL(enabledCats.size(), 0);
        
        auto allCats = root->allCatalogs();
        auto j = std::find(allCats.begin(), allCats.end(), cat);
        SG_VERIFY(j != allCats.end());

        SG_CHECK_EQUAL(allCats.size(), 1);

        // ensure existing package is still installed but not directly list
        
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
        SG_VERIFY(p1 !=  pkg::PackageRef());
        SG_CHECK_EQUAL(p1->id(), "b737-NG");

        auto packs = root->allPackages();
        auto k = std::find(packs.begin(), packs.end(), p1);
        SG_VERIFY(k == packs.end());
    }
}

void testVersionMigrate(HTTP::Client* cl)
{
    global_catalogVersion = 2; // version which has migration info
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("cat_migrate_version");
    simgear::Dir pd(rootPath);
    pd.removeChildren();
    
    {
        pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
        root->setHTTPClient(cl);
        
        pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
        waitForUpdateComplete(cl, root);
        SG_VERIFY(c->isEnabled());
        
        // install a package
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
        SG_CHECK_EQUAL(p1->id(), "b737-NG");
        pkg::InstallRef ins = p1->install();
        SG_VERIFY(ins->isQueued());
        waitForUpdateComplete(cl, root);
        SG_VERIFY(p1->isInstalled());
    }
    
    // bump version and refresh
    {
        pkg::RootRef root(new pkg::Root(rootPath, "10.1.2"));
        root->setHTTPClient(cl);
        
        // this should cause auto-migration
        root->refresh(true);
        waitForUpdateComplete(cl, root);
        
        pkg::CatalogRef cat = root->getCatalogById("org.flightgear.test.catalog1");
        SG_VERIFY(cat->isEnabled());
        SG_CHECK_EQUAL(cat->status(), pkg::Delegate::STATUS_REFRESHED);
        SG_CHECK_EQUAL(cat->id(), "org.flightgear.test.catalog1");
        SG_CHECK_EQUAL(cat->url(), "http://localhost:2000/catalogTest1/catalog-v10.xml");

        auto enabledCats = root->catalogs();
        auto it = std::find(enabledCats.begin(), enabledCats.end(), cat);
        SG_VERIFY(it != enabledCats.end());
        
        // ensure existing package is still installed
        
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
        SG_VERIFY(p1 !=  pkg::PackageRef());
        SG_CHECK_EQUAL(p1->id(), "b737-NG");
        
        auto packs = root->allPackages();
        auto k = std::find(packs.begin(), packs.end(), p1);
        SG_VERIFY(k != packs.end());
    }
}

void testVersionMigrateToId(HTTP::Client* cl)
{
    global_catalogVersion = 2; // version which has migration info
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("cat_migrate_version_id");
    simgear::Dir pd(rootPath);
    pd.removeChildren();
    
    {
        pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
        root->setHTTPClient(cl);
        
        pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
        waitForUpdateComplete(cl, root);
        SG_VERIFY(c->isEnabled());
        
        // install a package
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
        SG_CHECK_EQUAL(p1->id(), "b737-NG");
        pkg::InstallRef ins = p1->install();
        SG_VERIFY(ins->isQueued());
        waitForUpdateComplete(cl, root);
        SG_VERIFY(p1->isInstalled());
    }
    
    // change version to an alternate one
    {
        pkg::RootRef root(new pkg::Root(rootPath, "7.5"));
        root->setHTTPClient(cl);
        
        // this should cause the alternate package to be loaded
        root->refresh(true);
        waitForUpdateComplete(cl, root);
        
        pkg::CatalogRef cat = root->getCatalogById("org.flightgear.test.catalog1");
        SG_VERIFY(!cat->isEnabled());
        SG_CHECK_EQUAL(cat->status(), pkg::Delegate::FAIL_VERSION);
        SG_CHECK_EQUAL(cat->id(), "org.flightgear.test.catalog1");
        SG_CHECK_EQUAL(cat->url(), "http://localhost:2000/catalogTest1/catalog.xml");
        
        auto enabledCats = root->catalogs();
        auto it = std::find(enabledCats.begin(), enabledCats.end(), cat);
        SG_VERIFY(it == enabledCats.end());
                
        // ensure existing package is still installed
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
        SG_VERIFY(p1 !=  pkg::PackageRef());
        SG_CHECK_EQUAL(p1->id(), "b737-NG");
        
        // but not listed
        auto packs = root->allPackages();
        auto k = std::find(packs.begin(), packs.end(), p1);
        SG_VERIFY(k == packs.end());
        
        // check the new catalog
        auto altCat = root->getCatalogById("org.flightgear.test.catalog-alt");
        SG_VERIFY(altCat->isEnabled());
        SG_CHECK_EQUAL(altCat->status(), pkg::Delegate::STATUS_REFRESHED);
        SG_CHECK_EQUAL(altCat->id(), "org.flightgear.test.catalog-alt");
        SG_CHECK_EQUAL(altCat->url(), "http://localhost:2000/catalogTest1/catalog-alt.xml");
        
        it = std::find(enabledCats.begin(), enabledCats.end(), altCat);
        SG_VERIFY(it != enabledCats.end());

        SG_CHECK_EQUAL(altCat->packagesNeedingUpdate().size(),
                       1); // should be the 737

        // install a parallel package from the new catalog
        pkg::PackageRef p2 = root->getPackageById("org.flightgear.test.catalog-alt.b737-NG");
        SG_CHECK_EQUAL(p2->id(), "b737-NG");
        pkg::InstallRef ins = p2->install();
        SG_VERIFY(ins->isQueued());
        waitForUpdateComplete(cl, root);
        SG_VERIFY(p2->isInstalled());
        
        // do a non-scoped lookup, we should get the new one
        pkg::PackageRef p3 = root->getPackageById("b737-NG");
        SG_CHECK_EQUAL(p2, p3);
    }

    // test that re-init-ing doesn't migrate again
    {
        pkg::RootRef root(new pkg::Root(rootPath, "7.5"));
        root->setHTTPClient(cl);
        
        root->refresh(true);
        waitForUpdateComplete(cl, root);
        
        pkg::CatalogRef cat = root->getCatalogById("org.flightgear.test.catalog1");
        SG_VERIFY(!cat->isEnabled());
        SG_CHECK_EQUAL(cat->status(), pkg::Delegate::FAIL_VERSION);
        
        auto altCat = root->getCatalogById("org.flightgear.test.catalog-alt");
        SG_VERIFY(altCat->isEnabled());
        
        auto packs = root->allPackages();
        SG_CHECK_EQUAL(packs.size(), 4);
    }
    
    // and now switch back to the older version
    {
        pkg::RootRef root(new pkg::Root(rootPath, "8.1.0"));
        root->setHTTPClient(cl);
        root->refresh(true);
        waitForUpdateComplete(cl, root);
        
        pkg::CatalogRef cat = root->getCatalogById("org.flightgear.test.catalog1");
        SG_VERIFY(cat->isEnabled());
        SG_CHECK_EQUAL(cat->status(), pkg::Delegate::STATUS_REFRESHED);
        
        auto altCat = root->getCatalogById("org.flightgear.test.catalog-alt");
        SG_VERIFY(!altCat->isEnabled());

        // verify the original aircraft is still installed and available
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
        SG_VERIFY(p1 !=  pkg::PackageRef());
        SG_CHECK_EQUAL(p1->id(), "b737-NG");
        SG_VERIFY(p1->isInstalled());
        
        // verify the alt package is still installed,
        pkg::PackageRef p2 = root->getPackageById("org.flightgear.test.catalog-alt.b737-NG");
        SG_VERIFY(p2 !=  pkg::PackageRef());
        SG_CHECK_EQUAL(p2->id(), "b737-NG");
        SG_VERIFY(p2->isInstalled());
    }
}

void testOfflineMode(HTTP::Client* cl)
{
    global_catalogVersion = 0;
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("cat_offline_mode");
    simgear::Dir pd(rootPath);
    pd.removeChildren();
    
    {
        pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
        root->setHTTPClient(cl);
        
        pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
        waitForUpdateComplete(cl, root);
        SG_VERIFY(c->isEnabled());
        
        // install a package
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
        SG_CHECK_EQUAL(p1->id(), "b737-NG");
        pkg::InstallRef ins = p1->install();
        SG_VERIFY(ins->isQueued());
        waitForUpdateComplete(cl, root);
        SG_VERIFY(p1->isInstalled());
    }
    
    global_failRequests = true;
    
    {
        pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
        SG_CHECK_EQUAL(root->catalogs().size(), 1);

        root->setHTTPClient(cl);
        root->refresh(true);
        waitForUpdateComplete(cl, root);
        SG_CHECK_EQUAL(root->catalogs().size(), 1);

        pkg::CatalogRef cat = root->getCatalogById("org.flightgear.test.catalog1");
        SG_VERIFY(cat->isEnabled());
        SG_CHECK_EQUAL(cat->status(), pkg::Delegate::FAIL_DOWNLOAD);
        SG_CHECK_EQUAL(cat->id(), "org.flightgear.test.catalog1");
        
        auto enabledCats = root->catalogs();
        auto it = std::find(enabledCats.begin(), enabledCats.end(), cat);
        SG_VERIFY(it != enabledCats.end());
        
        // ensure existing package is still installed
        pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
        SG_VERIFY(p1 !=  pkg::PackageRef());
        SG_CHECK_EQUAL(p1->id(), "b737-NG");
        
        auto packs = root->allPackages();
        auto k = std::find(packs.begin(), packs.end(), p1);
        SG_VERIFY(k != packs.end());
    }
    
    global_failRequests = false;
}

int parseInvalidTest()
{
    SGPath rootPath = simgear::Dir::current().path();
    rootPath.append("testRoot");
    pkg::RootRef root(new pkg::Root(rootPath, "8.1.12"));
    pkg::CatalogRef cat = pkg::Catalog::createFromPath(root, SGPath(SRC_DIR "/catalogTestInvalid"));
    SG_VERIFY(cat.valid());

    SG_CHECK_EQUAL(cat->status(), pkg::Delegate::FAIL_VALIDATION);
 
    return 0;
}

void removeInvalidCatalog(HTTP::Client* cl)
{
    global_catalogVersion = 0; // fetch the good version
    
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("cat_remove_invalid");
    simgear::Dir pd(rootPath);
    pd.removeChildren();
    
    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    root->setHTTPClient(cl);
    
    // another catalog so the dicts are non-empty
    pkg::CatalogRef anotherCat = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");

    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTestInvalid/catalog.xml");
    waitForUpdateComplete(cl, root);
    SG_VERIFY(!c->isEnabled());
    SG_VERIFY(c->status() == pkg::Delegate::FAIL_VALIDATION);
    SG_VERIFY(!vectorContains(root->catalogs(), c));
    SG_VERIFY(vectorContains(root->allCatalogs(), c));
 
    // now remove it
    root->removeCatalog(c);
    SG_VERIFY(!vectorContains(root->catalogs(), c));
    SG_VERIFY(!vectorContains(root->allCatalogs(), c));
    c.clear(); // drop the catalog

    // re-add it again, and remove it again
    {
        pkg::CatalogRef c2 = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTestInvalid/catalog.xml");
        waitForUpdateComplete(cl, root);
        SG_VERIFY(!c2->isEnabled());
        SG_VERIFY(c2->status() == pkg::Delegate::FAIL_VALIDATION);
        SG_VERIFY(!vectorContains(root->catalogs(), c2));
        SG_VERIFY(vectorContains(root->allCatalogs(), c2));
        
        // now remove it
        root->removeCatalog(c2);
        SG_VERIFY(!vectorContains(root->catalogs(), c2));
        SG_VERIFY(!vectorContains(root->allCatalogs(), c2));
    }
    
    // only the other catalog (testCatalog should be left)
    SG_VERIFY(root->allCatalogs().size() == 1);
    SG_VERIFY(root->catalogs().size() == 1);
    
    SG_LOG(SG_GENERAL, SG_INFO, "Remove invalid catalog test passeed");
}

void updateInvalidToValid(HTTP::Client* cl)
{
    global_catalogVersion = 0;
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("cat_update_invalid_to_valid");
    simgear::Dir pd(rootPath);
    pd.removeChildren();
    
// first, sync the invalid version
    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    root->setHTTPClient(cl);
    
    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTestInvalid/catalog.xml");
    waitForUpdateComplete(cl, root);
    SG_VERIFY(!c->isEnabled());
    
    SG_VERIFY(c->status() == pkg::Delegate::FAIL_VALIDATION);
    SG_VERIFY(!vectorContains(root->catalogs(), c));
    SG_VERIFY(vectorContains(root->allCatalogs(), c));

// now refrsh the good one
    global_catalogVersion = 2;
    c->refresh();
    waitForUpdateComplete(cl, root);
    SG_VERIFY(c->isEnabled());
    SG_VERIFY(c->status() == pkg::Delegate::STATUS_REFRESHED);
    SG_VERIFY(vectorContains(root->catalogs(), c));

}

void updateValidToInvalid(HTTP::Client* cl)
{
    global_catalogVersion = 2; // fetch the good version
    
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("cat_update_valid_to_invalid");
    simgear::Dir pd(rootPath);
    pd.removeChildren();
    
    // first, sync the invalid version
    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    root->setHTTPClient(cl);
    
    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTestInvalid/catalog.xml");
    waitForUpdateComplete(cl, root);
    SG_VERIFY(c->isEnabled());
    SG_VERIFY(c->status() == pkg::Delegate::STATUS_REFRESHED);
    SG_VERIFY(vectorContains(root->catalogs(), c));
    SG_VERIFY(vectorContains(root->allCatalogs(), c));
    
    // now refrsh the bad one
    global_catalogVersion = 3;
    c->refresh();
    waitForUpdateComplete(cl, root);
    SG_VERIFY(!c->isEnabled());
    SG_VERIFY(c->status() == pkg::Delegate::FAIL_VALIDATION);
    SG_VERIFY(!vectorContains(root->catalogs(), c));
}

void updateInvalidToInvalid(HTTP::Client* cl)
{
    global_catalogVersion = 0;
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("cat_update_invalid_to_inalid");
    simgear::Dir pd(rootPath);
    pd.removeChildren();
    
    // first, sync the invalid version
    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    root->setHTTPClient(cl);
    
    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTestInvalid/catalog.xml");
    waitForUpdateComplete(cl, root);
    SG_VERIFY(!c->isEnabled());
    
    SG_VERIFY(c->status() == pkg::Delegate::FAIL_VALIDATION);
    SG_VERIFY(!vectorContains(root->catalogs(), c));
    SG_VERIFY(vectorContains(root->allCatalogs(), c));
    
    // now refresh to a different, but still bad one
    global_catalogVersion = 3;
    c->refresh();
    waitForUpdateComplete(cl, root);
    SG_VERIFY(!c->isEnabled());
    SG_VERIFY(c->status() == pkg::Delegate::FAIL_VALIDATION);
    SG_VERIFY(!vectorContains(root->catalogs(), c));
    
}

void testInstallBadPackage(HTTP::Client* cl)
{
    global_catalogVersion = 0;
    global_fail747Request = true;
    
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("pkg_install_bad_pkg");
    simgear::Dir pd(rootPath);
    pd.removeChildren();

    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    // specify a test dir
    root->setHTTPClient(cl);

    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
    waitForUpdateComplete(cl, root);

    pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b747-400");
    pkg::InstallRef ins = p1->install();
    
    bool didFail = false;
    ins->fail([&didFail, &ins](pkg::Install* ourInstall) {
        SG_CHECK_EQUAL(ins, ourInstall);
        didFail = true;
    });
    
    SG_VERIFY(ins->isQueued());

    waitForUpdateComplete(cl, root);
    SG_VERIFY(!p1->isInstalled());
    SG_VERIFY(didFail);
    SG_VERIFY(p1->existingInstall() == ins);
    SG_CHECK_EQUAL(ins->status(), pkg::Delegate::FAIL_DOWNLOAD);
    SG_CHECK_EQUAL(ins->path(), rootPath / "org.flightgear.test.catalog1" / "Aircraft" / "b744");
    
    // now retry, it should work
    global_fail747Request = false;
    
    auto ins2 = p1->install();
    SG_CHECK_EQUAL(ins2, p1->existingInstall());
    root->scheduleToUpdate(ins2);
    SG_VERIFY(ins2->isQueued());

    didFail = false;
    waitForUpdateComplete(cl, root);
    SG_VERIFY(p1->isInstalled());
    SG_VERIFY(!didFail);
    SG_CHECK_EQUAL(ins->status(), pkg::Delegate::STATUS_SUCCESS);
    SG_CHECK_EQUAL(ins->path(), rootPath / "org.flightgear.test.catalog1" / "Aircraft" / "b744");
}

void testMirrorsFailure(HTTP::Client* cl)
{
    global_catalogVersion = 0;
    // there's three mirrors defined, so se tthe first twom attempts to fail
    global_737RequestsToFailCount = 2;
    
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("pkg_install_mirror_fail");
    simgear::Dir pd(rootPath);
    pd.removeChildren();

    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    // specify a test dir
    root->setHTTPClient(cl);

    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
    waitForUpdateComplete(cl, root);

    pkg::PackageRef p1 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
    pkg::InstallRef ins = p1->install();
    
    bool didFail = false;
    ins->fail([&didFail, &ins](pkg::Install* ourInstall) {
        SG_CHECK_EQUAL(ins, ourInstall);
        didFail = true;
    });
    
    waitForUpdateComplete(cl, root);
    SG_VERIFY(p1->isInstalled());
    SG_VERIFY(!didFail);
    SG_VERIFY(p1->existingInstall() == ins);
    SG_CHECK_EQUAL(ins->status(), pkg::Delegate::STATUS_SUCCESS);
    
}

void testMigrateInstalled(HTTP::Client *cl) {
  SGPath rootPath(simgear::Dir::current().path());
  rootPath.append("pkg_migrate_installed");
  simgear::Dir pd(rootPath);
  pd.removeChildren();

  pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
  root->setHTTPClient(cl);

  pkg::CatalogRef oldCatalog, newCatalog;

  {
    oldCatalog = pkg::Catalog::createFromUrl(
        root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
    waitForUpdateComplete(cl, root);

    pkg::PackageRef p1 =
        root->getPackageById("org.flightgear.test.catalog1.b747-400");
    p1->install();
    auto p2 = root->getPackageById("org.flightgear.test.catalog1.c172p");
    p2->install();
    auto p3 = root->getPackageById("org.flightgear.test.catalog1.b737-NG");
    p3->install();
    waitForUpdateComplete(cl, root);
  }

  {
    newCatalog = pkg::Catalog::createFromUrl(
        root.ptr(), "http://localhost:2000/catalogTest2/catalog.xml");
    waitForUpdateComplete(cl, root);

    string_list existing;
    for (const auto& pack : oldCatalog->installedPackages(simgear::pkg::AnyPackageType)) {
        existing.push_back(pack->id());
    }

    SG_CHECK_EQUAL(4, existing.size());

    int result = newCatalog->markPackagesForInstallation(existing);
    SG_CHECK_EQUAL(2, result);
    SG_CHECK_EQUAL(2, newCatalog->packagesNeedingUpdate().size());

    auto p1 = root->getPackageById("org.flightgear.test.catalog2.b737-NG");
    auto ins = p1->existingInstall();
    SG_CHECK_EQUAL(0, ins->revsion());
  }

  {
    root->scheduleAllUpdates();
    waitForUpdateComplete(cl, root);

    SG_CHECK_EQUAL(0, newCatalog->packagesNeedingUpdate().size());

    auto p1 = root->getPackageById("org.flightgear.test.catalog2.b737-NG");
    auto ins = p1->existingInstall();
    SG_CHECK_EQUAL(ins->revsion(), p1->revision());
  }
}

void testDontMigrateRemoved(HTTP::Client *cl) {
  global_catalogVersion = 2; // version which has migration info
  SGPath rootPath(simgear::Dir::current().path());
  rootPath.append("cat_dont_migrate_id");
  simgear::Dir pd(rootPath);
  pd.removeChildren();

  // install and mnaully remove the alt catalog

  {
    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    root->setHTTPClient(cl);

    pkg::CatalogRef c = pkg::Catalog::createFromUrl(
        root.ptr(), "http://localhost:2000/catalogTest1/catalog-alt.xml");
    waitForUpdateComplete(cl, root);

    root->removeCatalogById("org.flightgear.test.catalog-alt");
  }

  // install the migration catalog
  {
    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    root->setHTTPClient(cl);

    pkg::CatalogRef c = pkg::Catalog::createFromUrl(
        root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
    waitForUpdateComplete(cl, root);
    SG_VERIFY(c->isEnabled());
  }

  // change version to an alternate one
  {
    pkg::RootRef root(new pkg::Root(rootPath, "7.5"));

    auto removed = root->explicitlyRemovedCatalogs();
    auto j = std::find(removed.begin(), removed.end(),
                       "org.flightgear.test.catalog-alt");
    SG_VERIFY(j != removed.end());

    root->setHTTPClient(cl);

    // this would tirgger migration, but we blocked it
    root->refresh(true);
    waitForUpdateComplete(cl, root);

    pkg::CatalogRef cat = root->getCatalogById("org.flightgear.test.catalog1");
    SG_VERIFY(!cat->isEnabled());
    SG_CHECK_EQUAL(cat->status(), pkg::Delegate::FAIL_VERSION);
    SG_CHECK_EQUAL(cat->id(), "org.flightgear.test.catalog1");
    SG_CHECK_EQUAL(cat->url(),
                   "http://localhost:2000/catalogTest1/catalog.xml");

    auto enabledCats = root->catalogs();
    auto it = std::find(enabledCats.begin(), enabledCats.end(), cat);
    SG_VERIFY(it == enabledCats.end());

    // check the new catalog
    auto altCat = root->getCatalogById("org.flightgear.test.catalog-alt");
    SG_VERIFY(altCat.get() == nullptr);
  }
}

void testProvides(HTTP::Client* cl)
{
    SGPath rootPath(simgear::Dir::current().path());
    rootPath.append("pkg_check_provides");
    simgear::Dir pd(rootPath);
    pd.removeChildren();
    global_catalogVersion = 0;

    pkg::RootRef root(new pkg::Root(rootPath, "8.1.2"));
    // specify a test dir
    root->setHTTPClient(cl);

    pkg::CatalogRef c = pkg::Catalog::createFromUrl(root.ptr(), "http://localhost:2000/catalogTest1/catalog.xml");
    waitForUpdateComplete(cl, root);

    // query
    auto packages = root->packagesProviding("Aircraft/b744", false);
    SG_CHECK_EQUAL(packages.size(), 1);
    SG_CHECK_EQUAL(packages.front()->qualifiedId(), "org.flightgear.test.catalog1.b747-400");

    packages = root->packagesProviding("movies/Foo/intro.mov", false);
    SG_CHECK_EQUAL(packages.size(), 1);
    SG_CHECK_EQUAL(packages.front()->qualifiedId(), "org.flightgear.test.catalog1.movies");
}

int main(int argc, char* argv[])
{
    sglog().setLogLevels( SG_ALL, SG_WARN );

    HTTP::Client cl;
    cl.setMaxConnections(1); 

    global_serverFilesRoot = SGPath(SRC_DIR);

    testAddCatalog(&cl);

    parseTest();

    parseInvalidTest();

    testInstallPackage(&cl);

    testUninstall(&cl);

    testRemoveCatalog(&cl);

    testRefreshCatalog(&cl);

    testInstallTarPackage(&cl);

    testInstallArchiveType(&cl);

    testDisableDueToVersion(&cl);

    testOfflineMode(&cl);

    testVersionMigrate(&cl);

    updateInvalidToValid(&cl);
    updateValidToInvalid(&cl);
    updateInvalidToInvalid(&cl);

    removeInvalidCatalog(&cl);

    testVersionMigrateToId(&cl);

    testInstallBadPackage(&cl);

    testMirrorsFailure(&cl);

    testMigrateInstalled(&cl);

    testDontMigrateRemoved(&cl);

    testProvides(&cl);

    cerr << "Successfully passed all tests!" << endl;
    return EXIT_SUCCESS;
}
