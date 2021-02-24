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

#include <simgear/compiler.h>	// for SG_USING_STD

#include <map>
#include <string>

#include <osg/Node>
#include <osgDB/ReaderWriter>

#include <simgear/props/props.hxx>
#include <simgear/misc/sg_path.hxx>

namespace osg {
    class PagedLOD;
}

namespace simgear {

class SGModelData; // defined below
class SGModelLOD;  // defined below

/**
 * Class for loading and managing models with XML wrappers.
 */
class SGModelLib
{
public:
    typedef osg::Node *(*panel_func)(SGPropertyNode *);

    static void init(const std::string &root_dir, SGPropertyNode* root);

    static void resetPropertyRoot();

    static void setPanelFunc(panel_func pf);

    // Load a 3D model (any format)
    // data->modelLoaded() will be called after the model is loaded
    static osg::Node* loadModel(const std::string &path,
                                SGPropertyNode *prop_root = NULL,
                                SGModelData *data=0, bool load2DPanels=false,
                                bool autoTooltipsMaster=false,
                                int autoTooltipsMasterMax=0);

    // Load a 3D model (any format) through the DatabasePager.
    // This function initially just returns a proxy node that refers to
    // the model file. Once the viewer steps onto that node the
    // model will be loaded.
    static osg::Node* loadDeferredModel(const std::string &path,
                                        SGPropertyNode *prop_root = NULL,
                                        SGModelData *data=0);
    // Load a 3D model (any format) through the DatabasePager.
    // This function initially just returns a PagedLOD node that refers to
    // the model file. Once the viewer steps onto that node the
    // model will be loaded. When the viewer does no longer reference this
    // node for a long time the node is unloaded again.
    static osg::PagedLOD* loadPagedModel(SGPropertyNode *prop_root,
                                      SGModelData *data,
                                      SGModelLOD model_lods);
    static osg::PagedLOD* loadPagedModel(const std::string &path,
                                     SGPropertyNode *prop_root = NULL,
                                     SGModelData *data=0);

   static osg::PagedLOD* loadPagedModel(std::vector<string> paths,
                                    SGPropertyNode *prop_root = NULL,
                                    SGModelData *data=0);

    static std::string findDataFile(const std::string& file,
      const osgDB::Options* opts = nullptr,
      SGPath currentDir = SGPath());

    static std::string findDataFile(const SGPath& file,
      const osgDB::Options* opts = nullptr,
      SGPath currentDir = SGPath());
protected:
    SGModelLib();
    ~SGModelLib ();

private:
  static SGPropertyNode_ptr static_propRoot;
  static panel_func static_panelFunc;
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
    virtual SGModelData* clone() const = 0;

    using ErrorContext = std::map<std::string, std::string>;

    virtual ErrorContext getErrorContext() const = 0;
};

/*
 * Data for a model with multiple LoD versions
 */

class SGModelLOD {
public:
  struct ModelLOD {
    ModelLOD(const string &p, float minrange, float maxrange) :
      path(p), min_range(minrange), max_range(maxrange)
      { }
    const string &path;
    float min_range;
    float max_range;
  };
typedef std::vector<ModelLOD> ModelLODList;

void insert(const ModelLOD& model)
{
  _models.push_back(model);
}

void insert(const string &p, float minrange, float maxrange)
{
  insert(ModelLOD(p, minrange, maxrange));
}

unsigned getNumLODs() const { return _models.size(); }
const ModelLOD& getModelLOD(unsigned i) const { return _models[i]; }

private:
  ModelLODList _models;
};

}

#endif // _SG_MODEL_LIB_HXX
