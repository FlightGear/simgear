#include "StringTable.hxx"

#include <simgear/threads/SGGuard.hxx>

namespace simgear
{
using namespace std;

const string* StringTable::insert(const string& str)
{
    SGGuard<SGMutex> lock(_mutex);
    StringContainer::iterator it = _strings.insert(str).first;
    return &*it;
}
}
