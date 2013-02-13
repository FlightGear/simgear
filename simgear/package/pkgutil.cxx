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
    } else if (!strcmp(argv[1], "uninstall")) {
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
    }
    
    while (http->hasActiveRequests()) {
        http->update();
    }

    return EXIT_SUCCESS;
}
