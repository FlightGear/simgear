#include <Cocoa/Cocoa.h>
#include <Foundation/NSAutoreleasePool.h>

#include <simgear/misc/sg_path.hxx>

namespace {
    
class CocoaAutoreleasePool
{
public:
    CocoaAutoreleasePool()
    {
        pool = [[NSAutoreleasePool alloc] init];
    }

    ~CocoaAutoreleasePool()
    {
        [pool release];
    }

private:
    NSAutoreleasePool* pool;
};

} // of anonyous namespace

SGPath appleSpecialFolder(int dirType, int domainMask, const SGPath& def)
{
    CocoaAutoreleasePool ap;
    NSFileManager* fm = [NSFileManager defaultManager];
    NSSearchPathDirectory d = static_cast<NSSearchPathDirectory>(dirType);
    NSURL* pathUrl = [fm URLForDirectory:d
                                inDomain:domainMask
                       appropriateForURL:Nil
                                  create:YES
                                   error:nil];
    if (!pathUrl) {
        return def;;
    }
    
    return SGPath([[pathUrl path] UTF8String], def.getPermissionChecker());
}