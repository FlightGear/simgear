#include "StringTable.hxx"


namespace simgear
{
using namespace std;

const string* StringTable::insert(const string& str)
{
    std::lock_guard<std::mutex> lock(_mutex);
    StringContainer::iterator it = _strings.insert(str).first;
    return &*it;
}
}
