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

#ifndef SG_PACKAGE_INSTALL_HXX
#define SG_PACKAGE_INSTALL_HXX

#include <vector>

#include <simgear/misc/sg_path.hxx>
#include <simgear/package/Delegate.hxx>

namespace simgear
{
    
namespace pkg
{

// forward decls
class Package;
class Catalog;
    
/**
 *
 */
class Install
{
public:
    /**
     * create from a directory on disk, or fail.
     */
    static Install* createFromPath(const SGPath& aPath, Catalog* aCat);
    
    unsigned int revsion() const
        { return m_revision; }
    
    Package* package() const
        { return m_package; } 
    
    SGPath path() const
        { return m_path; }
    
    bool hasUpdate() const;
    
    void startUpdate();
    
    void uninstall();
    
// boost signals time?
    // failure
    // progress
    // completed
    
private:
    friend class Package;
    
    class PackageArchiveDownloader;
    friend class PackageArchiveDownloader;
    
    Install(Package* aPkg, const SGPath& aPath);
    
    void parseRevision();
    void writeRevisionFile();
    
    void installResult(Delegate::FailureCode aReason);
    void installProgress(unsigned int aBytes, unsigned int aTotal);
    
    Package* m_package;
    unsigned int m_revision; ///< revision on disk
    SGPath m_path; ///< installation point on disk
    
    PackageArchiveDownloader* m_download;
};
    
    
} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_CATALOG_HXX
