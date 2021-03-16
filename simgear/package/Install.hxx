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

#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/package/Delegate.hxx>
#include <simgear/package/PackageCommon.hxx>

#include <simgear/structure/function_list.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/io/HTTPRequest.hxx>

namespace simgear
{
    
namespace pkg
{

/**
 *
 */
class Install : public SGReferenced
{
public:
    virtual ~Install();
  
    typedef std::function<void(Install*)> Callback;
    typedef std::function<void(Install*, unsigned int, unsigned int)> ProgressCallback;

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

    bool uninstall();

    bool isDownloading() const;
    
    bool isQueued() const;
    
    int downloadedPercent() const;
    
    size_t downloadedBytes() const;
    
    Delegate::StatusCode status() const;
    
    /**
     * full path to the primary -set.xml file for this install
     */
    SGPath primarySetPath() const;
    
    /**
     * if a download is in progress, cancel it. If this is the first install
     * of the package (as opposed to an update), the install will be cleaned
     * up once the last reference is gone.
     */
    void cancelDownload();
    
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
      return done(std::bind(mem_func, instance, std::placeholders::_1));
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
      return fail(std::bind(mem_func, instance, std::placeholders::_1));
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
      return always(std::bind(mem_func, instance, std::placeholders::_1));
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
      return progress(std::bind(mem_func,
                                instance,
                                std::placeholders::_1,
                                std::placeholders::_2,
                                std::placeholders::_3));
    }

private:
    friend class Package;
    
    class PackageArchiveDownloader;
    friend class PackageArchiveDownloader;
    
    Install(PackageRef aPkg, const SGPath& aPath);
    
    void parseRevision();
    void writeRevisionFile();
    
    void installResult(Delegate::StatusCode aReason);
    void installProgress(unsigned int aBytes, unsigned int aTotal);
    void startDownload();
    
    PackageRef m_package;
    unsigned int m_revision; ///< revision on disk
    SGPath m_path; ///< installation point on disk
    
    HTTP::Request_ptr m_download;

    Delegate::StatusCode m_status;

    function_list<Callback>         _cb_done,
                                    _cb_fail,
                                    _cb_always;
    function_list<ProgressCallback> _cb_progress;
};
    
    
} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_CATALOG_HXX
