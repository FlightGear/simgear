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

#ifndef SG_PACKAGE_CATALOG_HXX
#define SG_PACKAGE_CATALOG_HXX

#include <vector>
#include <ctime>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>

#include <simgear/package/Delegate.hxx>

namespace simgear
{
    
namespace HTTP { class Client; }
    
namespace pkg
{

// forward decls
class Package;
class Catalog;
class Root;

typedef std::vector<Package*> PackageList;
typedef std::vector<Catalog*> CatalogList;

class Catalog
{
public:
    virtual ~Catalog();
    
    static Catalog* createFromUrl(Root* aRoot, const std::string& aUrl);
    
    static Catalog* createFromPath(Root* aRoot, const SGPath& aPath);
    
    static CatalogList allCatalogs();
    
    Root* root() const
        { return m_root;};
    
    /**
     * perform a refresh of the catalog contents
     */
    void refresh();
    /**
     * retrieve packages in this catalog matching a filter.
     * filter consists of required / minimum values, AND-ed together.
     */
    PackageList packagesMatching(const SGPropertyNode* aFilter) const;
    
    /**
     * retrieve all the packages in the catalog which are installed
     * and have a pendig update
     */ 
    PackageList packagesNeedingUpdate() const;
     
    SGPath installRoot() const
         { return m_installRoot; }
    
    std::string id() const;
    
    std::string url() const;
    
    std::string description() const;
    
    Package* getPackageById(const std::string& aId) const;
    
    /**
     * test if the catalog data was retrieved longer ago than the
     * maximum permitted age for this catalog.
     */
    bool needsRefresh() const;
    
    unsigned int ageInSeconds() const;
    
    /**
     * access the raw property data in the catalog
     */
    SGPropertyNode* properties() const;
private:
    Catalog(Root* aRoot);
    
    class Downloader;
    friend class Downloader;
    
    void parseProps(const SGPropertyNode* aProps);
    
    void refreshComplete(Delegate::FailureCode aReason);
    
    void parseTimestamp();
    void writeTimestamp();
    
    std::string getLocalisedString(const SGPropertyNode* aRoot, const char* aName) const;
    
    Root* m_root;
    SGPropertyNode_ptr m_props;
    SGPath m_installRoot;
    
    PackageList m_packages;
    time_t m_retrievedTime;
};  
    
} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_CATALOG_HXX
