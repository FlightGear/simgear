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

namespace simgear
{
        
namespace pkg
{
    
class Install;
class Catalog;
    
/**
 * package delegate is the mechanism to discover progress / completion /
 * errors in packaging steps asynchronously.
 */
class Delegate
{
public:
    typedef enum {
        FAIL_SUCCESS = 0, ///< not a failure :)
        FAIL_UNKNOWN = 1,
        FAIL_IN_PROGRESS, ///< downloading/installation in progress (not a failure :P)
        FAIL_CHECKSUM,  ///< package MD5 verificstion failed
        FAIL_DOWNLOAD,  ///< network issue
        FAIL_EXTRACT,   ///< package archive failed to extract cleanly
        FAIL_FILESYSTEM,    ///< unknown filesystem error occurred
        FAIL_VERSION ///< version check mismatch
    } FailureCode;
    
    
    virtual ~Delegate() { }
    
    /**
     * invoked when all catalogs have finished refreshing - either successfully
     * or with a failure.
     */
    virtual void refreshComplete() = 0;
    
    /**
     * emitted when a catalog fails to refresh, due to a network issue or
     * some other failure.
     */
    virtual void failedRefresh(Catalog*, FailureCode aReason) = 0;
    
    virtual void startInstall(Install* aInstall) = 0;
    virtual void installProgress(Install* aInstall, unsigned int aBytes, unsigned int aTotal) = 0;
    virtual void finishInstall(Install* aInstall) = 0;
    
   
    virtual void failedInstall(Install* aInstall, FailureCode aReason) = 0;
    
};  
    
} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_DELEGATE_HXX
