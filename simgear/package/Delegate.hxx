

#ifndef SG_PACKAGE_DELEGATE_HXX
#define SG_PACKAGE_DELEGATE_HXX

namespace simgear
{
        
namespace pkg
{
    
class Install;

class Delegate
{
public:
    virtual ~Delegate() { }
    
    virtual void refreshComplete() = 0;
    
    virtual void startInstall(Install* aInstall) = 0;
    virtual void installProgress(Install* aInstall, unsigned int aBytes, unsigned int aTotal) = 0;
    virtual void finishInstall(Install* aInstall) = 0;
    
    typedef enum {
        FAIL_UNKNOWN = 0,
        FAIL_CHECKSUM,
        FAIL_DOWNLOAD,
        FAIL_EXTRACT,
        FAIL_FILESYSTEM
    } FailureCode;
    
    virtual void failedInstall(Install* aInstall, FailureCode aReason) = 0;
    
};  
    
} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_DELEGATE_HXX
