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

#include <simgear/structure/function_list.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include <boost/bind.hpp>

namespace simgear
{
    
namespace pkg
{

// forward decls
class Package;
class Catalog;
class Install;
  
typedef SGSharedPtr<Package> PackageRef;
typedef SGSharedPtr<Catalog> CatalogRef;  
typedef SGSharedPtr<Install> InstallRef;
  
/**
 *
 */
class Install : public SGReferenced
{
public:
    virtual ~Install();
  
    typedef boost::function<void(Install*)> Callback;
    typedef boost::function<void(Install*, unsigned int, unsigned int)>
                                            ProgressCallback;

    /**
     * create from a directory on disk, or fail.
     */
    static InstallRef createFromPath(const SGPath& aPath, CatalogRef aCat);
    
    unsigned int revsion() const
        { return m_revision; }
    
    PackageRef package() const
        { return m_package; } 
    
    SGPath path() const
        { return m_path; }
    
    bool hasUpdate() const;
    
    void startUpdate();
    
    void uninstall();

    /**
     * Set the handler to be called when the installation successfully
     * completes.
     *
     * @note If the installation is already complete, the handler is called
     *       immediately.
     */
    Install* done(const Callback& cb);

    template<class C>
    Install* done(C* instance, void (C::*mem_func)(Install*))
    {
      return done(boost::bind(mem_func, instance, _1));
    }

    /**
     * Set the handler to be called when the installation fails or is aborted.
     *
     * @note If the installation has already failed, the handler is called
     *       immediately.
     */
    Install* fail(const Callback& cb);

    template<class C>
    Install* fail(C* instance, void (C::*mem_func)(Install*))
    {
      return fail(boost::bind(mem_func, instance, _1));
    }

    /**
     * Set the handler to be called when the installation either successfully
     * completes or fails.
     *
     * @note If the installation is already complete or has already failed, the
     *       handler is called immediately.
     */
    Install* always(const Callback& cb);

    template<class C>
    Install* always(C* instance, void (C::*mem_func)(Install*))
    {
      return always(boost::bind(mem_func, instance, _1));
    }
    
    /**
     * Set the handler to be called during downloading the installation file
     * indicating the progress of the download.
     *
     */
    Install* progress(const ProgressCallback& cb);

    template<class C>
    Install* progress(C* instance,
                      void (C::*mem_func)(Install*, unsigned int, unsigned int))
    {
      return progress(boost::bind(mem_func, instance, _1, _2, _3));
    }

private:
    friend class Package;
    
    class PackageArchiveDownloader;
    friend class PackageArchiveDownloader;
    
    Install(PackageRef aPkg, const SGPath& aPath);
    
    void parseRevision();
    void writeRevisionFile();
    
    void installResult(Delegate::FailureCode aReason);
    void installProgress(unsigned int aBytes, unsigned int aTotal);
    
    PackageRef m_package;
    unsigned int m_revision; ///< revision on disk
    SGPath m_path; ///< installation point on disk
    
    PackageArchiveDownloader* m_download;

    Delegate::FailureCode _status;

    function_list<Callback>         _cb_done,
                                    _cb_fail,
                                    _cb_always;
    function_list<ProgressCallback> _cb_progress;
};
    
    
} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_CATALOG_HXX
