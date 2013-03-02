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

#include <simgear/io/HTTPClient.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/misc/sg_dir.hxx>

#include <boost/foreach.hpp>
#include <iostream>
#include <cstring>

using namespace simgear; 
using namespace std;

bool keepRunning = true;

int main(int argc, char** argv)
{

    HTTP::Client* http = new HTTP::Client();
    pkg::Root* root = new pkg::Root(Dir::current().path());
    
    cout << "Package root is:" << Dir::current().path() << endl;
    cout << "have " << pkg::Catalog::allCatalogs().size() << " catalog(s)" << endl;
        
    root->setHTTPClient(http);
    
    if (!strcmp(argv[1], "add")) {
        std::string url(argv[2]);
        pkg::Catalog::createFromUrl(root, url);
    } else if (!strcmp(argv[1], "refresh")) {
        root->refresh();
    } else if (!strcmp(argv[1], "install")) {
        pkg::Package* pkg = root->getPackageById(argv[2]);
        if (!pkg) {
            cerr << "unknown package:" << argv[2] << endl;
            return EXIT_FAILURE;
        }
        
        if (pkg->isInstalled()) {
            cout << "package " << pkg->id() << " is already installed at " << pkg->install()->path() << endl;
            return EXIT_SUCCESS;
        }
        
        pkg::Catalog* catalog = pkg->catalog();
        cout << "Will install:" << pkg->id() << " from " << catalog->id() <<
                "(" << catalog->description() << ")" << endl;
        pkg->install();
    } else if (!strcmp(argv[1], "uninstall") || !strcmp(argv[1], "remove")) {
        pkg::Package* pkg = root->getPackageById(argv[2]);
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
        BOOST_FOREACH(pkg::Package* p, updates) {
            root->scheduleToUpdate(p->install());
        }
    } else if (!strcmp(argv[1], "list-updated")) {
        pkg::PackageList updates = root->packagesNeedingUpdate();
        if (updates.empty()) {
            cout << "no packages with updates" << endl;
            return EXIT_SUCCESS;
        }
        
        cout << updates.size() << " packages have updates" << endl;
        BOOST_FOREACH(pkg::Package* p, updates) {
            cout << "\t" << p->id() << " " << p->getLocalisedProp("name") << endl;
        }
    } else {
        cerr << "unknown command:" << argv[1] << endl;
        return EXIT_FAILURE;
    }
    
    while (http->hasActiveRequests()) {
        http->update();
    }

    return EXIT_SUCCESS;
}
