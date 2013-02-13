#ifndef SG_PACKAGE_INSTALL_HXX
#define SG_PACKAGE_INSTALL_HXX

#include <vector>

#include <simgear/misc/sg_path.hxx>

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
    
    Package* m_package;
    unsigned int m_revision; ///< revision on disk
    SGPath m_path; ///< installation point on disk
    
    PackageArchiveDownloader* m_download;
};
    
    
} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_CATALOG_HXX
