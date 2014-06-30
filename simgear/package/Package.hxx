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

#ifndef SG_PACKAGE_PACKAGE_HXX
#define SG_PACKAGE_PACKAGE_HXX

#include <set>
#include <vector>

#include <simgear/props/props.hxx>
#include <simgear/misc/strutils.hxx>

#include <simgear/structure/function_list.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

typedef std::set<std::string> string_set;

namespace simgear
{
    
namespace pkg
{

// forward decls
class Install;
class Catalog;
class Package;
  
typedef SGSharedPtr<Package> PackageRef;
typedef SGSharedPtr<Catalog> CatalogRef;
typedef SGSharedPtr<Install> InstallRef;

typedef std::vector<PackageRef> PackageList;
    
  class Package : public SGReferenced
{
public:

    typedef boost::function<void(Package*, Install*)> InstallCallback;

    /**
     * get or create an install for the package
     */
    InstallRef install();

    InstallRef
    existingInstall(const InstallCallback& cb = InstallCallback()) const;

    bool isInstalled() const;
    
    std::string id() const;

    /**
     * Variant IDs. Note the primary ID will always be included as 
     * variants()[0], to simplify enumerating all variants
     */
    string_list variants() const;

    /**
     * Fully-qualified ID, including our catalog'd ID
     */
    std::string qualifiedId() const;
    
    /**
     * human-readable name - note this is probably not localised,
     * although this is not ruled out for the future.
     */
    std::string name() const;

    /**
     * Human readable name of a variant
     */
    std::string nameForVariant(const std::string& vid) const;

    /**
     * syntactic sugar to get the localised description
     */
    std::string description() const;
    
    /**
     * access the raw property data in the package
     */
    SGPropertyNode* properties() const;
    
    /**
     * hex-encoded MD5 sum of the download files
     */
    std::string md5() const;
    
    std::string getLocalisedProp(const std::string& aName) const;

    unsigned int revision() const;
  
    size_t fileSizeBytes() const;
  
    CatalogRef catalog() const
        { return m_catalog; }
    
    bool matches(const SGPropertyNode* aFilter) const;
    
    /**
     * download URLs for the package
     */
    string_list downloadUrls() const;
    
    string_list thumbnailUrls() const;
    
    /**
     * Packages we depend upon.
     * If the dependency list cannot be satisifed for some reason,
     * this will raise an sg_exception.
     */
    PackageList dependencies() const;
private:
    SGPath pathOnDisk() const;

    friend class Catalog;
    
    Package(const SGPropertyNode* aProps, CatalogRef aCatalog);
    
    void initWithProps(const SGPropertyNode* aProps);

    void updateFromProps(const SGPropertyNode* aProps);

    std::string getLocalisedString(const SGPropertyNode* aRoot, const char* aName) const;
    
    SGPropertyNode_ptr m_props;
    std::string m_id;
    string_set m_tags;
    CatalogRef m_catalog;

    mutable function_list<InstallCallback> _install_cb;
};




} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_PACKAGE_HXX

