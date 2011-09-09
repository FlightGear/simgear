#ifndef SIMGEAR_STRINGTABLE_HXX
#define SIMGEAR_STRINGTABLE_HXX 1

#include <string>

#include <simgear/threads/SGThread.hxx>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>

namespace simgear
{
typedef boost::multi_index_container<
    std::string,
    boost::multi_index::indexed_by<
        boost::multi_index::hashed_unique<
            boost::multi_index::identity<std::string> > > >
StringContainer;

class StringTable
{
    const std::string* insert(const std::string& str);
private:
    SGMutex _mutex;
    StringContainer _strings;
};
}
#endif
