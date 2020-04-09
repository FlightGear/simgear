// Copyright (C) 2013  James Turner - zakalawe@mac.com
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

#include <simgear/io/HTTPClient.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/misc/sg_dir.hxx>

#include <iostream>
#include <cstring>

using namespace simgear;
using namespace std;

bool keepRunning = true;

class MyDelegate : public pkg::Delegate
{
public:
    virtual void catalogRefreshed(pkg::CatalogRef aCatalog, StatusCode aReason)
    {
        if (aReason == STATUS_REFRESHED) {
            if (aCatalog.ptr() == NULL) {
                cout << "refreshed all catalogs" << endl;
            } else {
                cout << "refreshed catalog " << aCatalog->url() << endl;
            }
        } else if (aReason == STATUS_IN_PROGRESS) {
            cout << "started refresh of " << aCatalog->url() << endl;
        } else {
            cerr << "failed refresh of " << aCatalog->url() << ":" << aReason << endl;
        }
    }

    virtual void startInstall(pkg::InstallRef aInstall)
    {
        _lastPercent = 999;
        cout << "starting install of " << aInstall->package()->name() << endl;
    }

    virtual void installProgress(pkg::InstallRef aInstall, unsigned int bytes, unsigned int total)
    {
        size_t percent = (static_cast<size_t>(bytes) * 100) / total;
        if (percent == _lastPercent) {
            return;
        }

        _lastPercent = percent;
        cout << percent << "%" << endl;
    }

    virtual void finishInstall(pkg::InstallRef aInstall, StatusCode aReason)
    {
        if (aReason == STATUS_SUCCESS) {
            cout << "done install of " << aInstall->package()->name() << endl;
        } else {
            cerr << "failed install of " << aInstall->package()->name() << endl;
        }
    }

private:
    size_t _lastPercent;

};

void printRating(pkg::Package* pkg, const std::string& aRating, const std::string& aLabel)
{
    SGPropertyNode* ratings = pkg->properties()->getChild("rating");
    cout << "\t" << aLabel << ":" << ratings->getIntValue(aRating) << endl;
}

void printPackageInfo(pkg::Package* pkg)
{
    cout << "Package:" << pkg->catalog()->id() << "." << pkg->id() << endl;
    cout << "Revision:" << pkg->revision() << endl;
    cout << "Name:" << pkg->name() << endl;
    cout << "Description:" << pkg->description() << endl;
    cout << "Long description:\n" << pkg->getLocalisedProp("long-description") << endl << endl;

    if (pkg->properties()->hasChild("author")) {
        cout << "Authors:" << endl;
        for (auto author : pkg->properties()->getChildren("author")) {
            if (author->hasChild("name")) {
                cout << "\t" << author->getStringValue("name") << endl;

            } else {
                // simple author structure
                cout << "\t" << author->getStringValue() << endl;
            }


        }

        cout << endl;
    }

    cout << "Ratings:" << endl;
    printRating(pkg, "fdm",     "Flight-model    ");
    printRating(pkg, "cockpit", "Cockpit         ");
    printRating(pkg, "model",   "3D model        ");
    printRating(pkg, "systems", "Aircraft systems");
}

int main(int argc, char** argv)
{
    sglog().setLogLevels( SG_ALL, SG_INFO );
    
    HTTP::Client* http = new HTTP::Client();
    
    SGPath rootPath = SGPath::fromEnv("SG_PKG_ROOT", Dir::current().path());
    pkg::Root* root = new pkg::Root(rootPath, "2019.1.1");
    
    MyDelegate dlg;
    root->addDelegate(&dlg);

    cout << "Package root is:" << rootPath << endl;
    cout << "have " << root->catalogs().size() << " catalog(s)" << endl;

    root->setHTTPClient(http);

    if (!strcmp(argv[1], "add")) {
        std::string url(argv[2]);
        pkg::Catalog::createFromUrl(root, url);
    } else if (!strcmp(argv[1], "refresh")) {
        root->refresh(true);
    } else if (!strcmp(argv[1], "install")) {
        pkg::PackageRef pkg = root->getPackageById(argv[2]);
        if (!pkg) {
            cerr << "unknown package:" << argv[2] << endl;
            return EXIT_FAILURE;
        }

        if (pkg->isInstalled()) {
            cout << "package " << pkg->id() << " is already installed at " << pkg->install()->path() << endl;
            return EXIT_SUCCESS;
        }

        pkg::CatalogRef catalog = pkg->catalog();
        cout << "Will install:" << pkg->id() << " from " << catalog->id() <<
                "(" << catalog->description() << ")" << endl;
        pkg->install();
    } else if (!strcmp(argv[1], "uninstall") || !strcmp(argv[1], "remove")) {
        pkg::PackageRef pkg = root->getPackageById(argv[2]);
        if (!pkg) {
            cerr << "unknown package:" << argv[2] << endl;
            return EXIT_FAILURE;
        }

        if (!pkg->isInstalled()) {
            cerr << "package " << argv[2] << " not installed" << endl;
            return EXIT_FAILURE;
        }

        cout << "Will uninstall:" << pkg->id() << endl;
        pkg->install()->uninstall();
    } else if (!strcmp(argv[1], "update-all")) {
        pkg::PackageList updates = root->packagesNeedingUpdate();
        for (auto p : updates) {
            root->scheduleToUpdate(p->install());
        }
    } else if (!strcmp(argv[1], "list-updated")) {
        pkg::PackageList updates = root->packagesNeedingUpdate();
        if (updates.empty()) {
            cout << "no packages with updates" << endl;
            return EXIT_SUCCESS;
        }

        cout << updates.size() << " packages have updates" << endl;
        for (auto p : updates) {
            cout << "\t" << p->id() << " " << p->getLocalisedProp("name") << endl;
        }
    } else if (!strcmp(argv[1], "info")) {
        pkg::PackageRef pkg = root->getPackageById(argv[2]);
        if (!pkg) {
            cerr << "unknown package:" << argv[2] << endl;
            return EXIT_FAILURE;
        }

        printPackageInfo(pkg);
    } else {
        cerr << "unknown command:" << argv[1] << endl;
        return EXIT_FAILURE;
    }

    while (http->hasActiveRequests()) {
        http->update();
    }

    return EXIT_SUCCESS;
}
