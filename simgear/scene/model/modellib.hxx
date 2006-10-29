// modellib.cxx - implement an SSG model library.

#ifndef _SG_MODEL_LIB_HXX
#define _SG_MODEL_LIB_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>	// for SG_USING_STD

#include <map>
#include STL_STRING

#include <osg/ref_ptr>
#include <osg/Node>

#include <simgear/props/props.hxx>
#include "model.hxx"

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

    virtual osg::Node *load_model( const string &fg_root,
                                   const string &path,
                                   SGPropertyNode *prop_root,
                                   double sim_time_sec,
                                   bool cache_object,
                                   SGModelData *data = 0 );
protected:

    map<string, osg::ref_ptr<osg::Node> > _table;
};


#endif // _SG_MODEL_LIB_HXX
