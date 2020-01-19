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

#ifndef SG_PACKAGE_DELEGATE_HXX
#define SG_PACKAGE_DELEGATE_HXX

#include <string>
#include <simgear/misc/stdint.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

namespace simgear
{

namespace pkg
{

class Install;
class Catalog;
class Package;

typedef SGSharedPtr<Catalog> CatalogRef;
typedef SGSharedPtr<Install> InstallRef;
typedef SGSharedPtr<Package> PackageRef;

/**
 * package delegate is the mechanism to discover progress / completion /
 * errors in packaging steps asynchronously.
 */
class Delegate
{
public:
    typedef enum {
        STATUS_SUCCESS = 0,
        FAIL_UNKNOWN = 1,
        STATUS_IN_PROGRESS, ///< downloading/installation in progress
        FAIL_CHECKSUM,      ///< package MD5 verificstion failed
        FAIL_DOWNLOAD,      ///< network issue
        FAIL_EXTRACT,       ///< package archive failed to extract cleanly
        FAIL_FILESYSTEM,    ///< unknown filesystem error occurred
        FAIL_VERSION,       ///< version check mismatch
        FAIL_NOT_FOUND,     ///< package URL returned a 404
        FAIL_HTTP_FORBIDDEN, ///< URL returned a 403. Marked specially to catch rate-limiting
        FAIL_VALIDATION,    ///< catalog or package failed to validate
        STATUS_REFRESHED,
        USER_CANCELLED,
        USER_DISABLED
    } StatusCode;


    virtual ~Delegate() { }


    /**
     * emitted when a catalog refesh completes, either success or failure
     * If catalog is null, this means /all/ catalogs have been refreshed
     */
    virtual void catalogRefreshed(CatalogRef, StatusCode aReason) = 0;

    virtual void startInstall(InstallRef aInstall) = 0;
    virtual void installProgress(InstallRef aInstall, unsigned int aBytes, unsigned int aTotal) = 0;
    virtual void finishInstall(InstallRef aInstall, StatusCode aReason) = 0;

    virtual void finishUninstall(const PackageRef& aPackage) {};

    /**
     * Notification when catalogs/packages are added or removed
     */
    virtual void availablePackagesChanged() {}
    
    /**
     * More general purpose notification when install is queued / cancelled / started
     * stopped. Reason value is only in certain cases.
     */
    virtual void installStatusChanged(InstallRef aInstall, StatusCode aReason);

	virtual void dataForThumbnail(const std::string& aThumbnailUrl,
		size_t lenth, const uint8_t* bytes);
};

} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_DELEGATE_HXX
