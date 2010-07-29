// Copyright (C) 2008 Till Busch buti@bux.at
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef _SG_MODEL_LIB_HXX
#define _SG_MODEL_LIB_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>	// for SG_USING_STD

#include <string>

#include <osg/Node>
#include <osgDB/ReaderWriter>

#include <simgear/props/props.hxx>
#include <simgear/misc/sg_path.hxx>

namespace simgear {

class SGModelData; // defined below

/**
 * Class for loading and managing models with XML wrappers.
 */
class SGModelLib
{
public:
    typedef osg::Node *(*panel_func)(SGPropertyNode *);

    typedef SGPath (*resolve_func)(const std::string& path);

    static void init(const std::string &root_dir);

    static void setPropRoot(SGPropertyNode* root);
    
    static void setPanelFunc(panel_func pf);
    
    static void setResolveFunc(resolve_func rf);

    // Load a 3D model (any format)
    // data->modelLoaded() will be called after the model is loaded
    static osg::Node* loadModel(const std::string &path,
                                SGPropertyNode *prop_root = NULL,
                                SGModelData *data=0);

    // Load a 3D model (any format) through the DatabasePager.
    // Most models should be loaded using this function!
    // This function will initially return an SGPagedLOD node.
    // data->modelLoaded() will be called after the model is loaded and
    // connected to the scene graph. See AIModelData on how to use this.
    // NOTE: AIModelData uses observer_ptr to avoid circular references.
    static osg::Node* loadPagedModel(const std::string &path,
                                     SGPropertyNode *prop_root = NULL,
                                     SGModelData *data=0);

    static std::string findDataFile(const std::string& file, const osgDB::ReaderWriter::Options* opts = NULL); 
protected:
    SGModelLib();
    ~SGModelLib ();
    
private:
  static SGPropertyNode_ptr static_propRoot;
  static panel_func static_panelFunc;
  static resolve_func static_resolver;
};


/**
 * Abstract class for adding data to the scene graph.  modelLoaded() is
 * called after the model was loaded, and the destructor when the branch
 * is removed from the scene graph.
 */
class SGModelData : public osg::Referenced {
public:
    virtual ~SGModelData() {}
    virtual void modelLoaded(const std::string& path, SGPropertyNode *prop,
                             osg::Node* branch) = 0;
};

}

#endif // _SG_MODEL_LIB_HXX
