// modellib.cxx - implement an SSG model library.

#ifndef _SG_MODEL_LIB_HXX
#define _SG_MODEL_LIB_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>	// for SG_USING_STD

#include <map>
#include STL_STRING

#include <plib/ssg.h>

#include <simgear/props/props.hxx>

SG_USING_STD(map);
SG_USING_STD(string);


/**
 * Class for loading and managing models with XML wrappers.
 */
class SGModelLib
{

public:

    SGModelLib ();
    virtual ~SGModelLib ();
    virtual void flush1();

    virtual ssgEntity *load_model( const string &fg_root,
                                   const string &path,
                                   SGPropertyNode *prop_root,
                                   double sim_time_sec );
protected:

    map<string,ssgBase *> _table;
};


#endif // _SG_MODEL_LIB_HXX
