#ifndef __MODEL_LOADER_HXX
#define __MODEL_LOADER_HXX 1

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
 * Base class for loading and managing SSG things.
 */
class SGssgLoader
{
public:
    SGssgLoader ();
    virtual ~SGssgLoader ();
    virtual void flush ();
protected:
    std::map<string,ssgBase *> _table;
};


/**
 * Class for loading and managing models with XML wrappers.
 */
class SGModelLoader : public SGssgLoader
{
public:
    SGModelLoader ();
    virtual ~SGModelLoader ();

    virtual ssgEntity *load_model( const string &fg_root,
                                   const string &path,
                                   SGPropertyNode *prop_root,
                                   double sim_time_sec );
};


#endif
