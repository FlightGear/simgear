#include "StringTable.hxx"

#include <OpenThreads/ScopedLock>

namespace simgear
{
using namespace std;

const string* StringTable::insert(const string& str)
{
    using namespace OpenThreads;
    ScopedLock<Mutex> lock(_mutex);
    StringContainer::iterator it = _strings.insert(str).first;
    return &*it;
}
}
