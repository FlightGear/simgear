#include <simgear/structure/intern.hxx>

#include <simgear/structure/StringTable.hxx>
#include <simgear/structure/Singleton.hxx>

namespace
{
class GlobalStringTable : public StringTable,
                          public Singleton<GlobalStringTable>
{
};
}

namespace simgear
{
const std::string* intern(const std::string& str)
{
    return GlobalStringTable::instance()->insert(str);
}
}
