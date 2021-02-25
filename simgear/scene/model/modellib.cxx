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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <boost/algorithm/string.hpp>

#include <osg/Version>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>

#include <simgear/constants.h>
#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/misc/ResourceManager.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "SGReaderWriterXML.hxx"

#include "modellib.hxx"

using std::string;
using namespace simgear;

osgDB::RegisterReaderWriterProxy<SGReaderWriterXML> g_readerWriter_XML_Proxy;
ModelRegistryCallbackProxy<LoadOnlyCallback> g_xmlCallbackProxy("xml");

SGPropertyNode_ptr SGModelLib::static_propRoot;
SGModelLib::panel_func SGModelLib::static_panelFunc = NULL;

////////////////////////////////////////////////////////////////////////
// Implementation of SGModelLib.
////////////////////////////////////////////////////////////////////////
void SGModelLib::init(const string &root_dir, SGPropertyNode* root)
{
    osgDB::Registry::instance()->getDataFilePathList().push_front(root_dir);
    osgDB::Registry::instance()->getOptions()->getDatabasePathList().push_front(root_dir);
    static_propRoot = root;
}

void SGModelLib::resetPropertyRoot()
{
    static_propRoot.clear();
}

void SGModelLib::setPanelFunc(panel_func pf)
{
  static_panelFunc = pf;
}

std::string SGModelLib::findDataFile(const std::string& file,
  const osgDB::Options* opts,
  SGPath currentPath)
{
  if (file.empty())
    return file;
  SGPath p = ResourceManager::instance()->findPath(file, currentPath);
  if (p.exists()) {
    return p.utf8Str();
  }

  // finally hand on to standard OSG behaviour
  return osgDB::findDataFile(file, opts);
}

std::string SGModelLib::findDataFile(const SGPath& file,
  const osgDB::Options* opts,
  SGPath currentPath)
{
  SGPath p = ResourceManager::instance()->findPath(file.utf8Str(), currentPath);
  if (p.exists()) {
    return p.utf8Str();
  }

  // finally hand on to standard OSG behaviour
  return osgDB::findDataFile(file.utf8Str(), opts);
}


SGModelLib::SGModelLib()
{
}

SGModelLib::~SGModelLib()
{
}

namespace
{
osg::Node* loadFile(const string& path, SGReaderWriterOptions* options)
{
    using namespace osg;
    using namespace osgDB;
    if (boost::iends_with(path, ".ac") || boost::iends_with(path, ".obj")) {
        options->setInstantiateEffects(true);
    }

    ref_ptr<Node> model = readRefNodeFile(path, options);
    if (!model)
        return 0;
    else
     return model.release();
}
}

osg::Node*
SGModelLib::loadModel(const string &path,
                       SGPropertyNode *prop_root,
                       SGModelData *data,
                       bool load2DPanels,
                       bool autoTooltipsMaster,
                       int autoTooltipsMasterMax)
{
    osg::ref_ptr<SGReaderWriterOptions> opt;
    opt = SGReaderWriterOptions::copyOrCreate(osgDB::Registry::instance()->getOptions());
    opt->getDatabasePathList().push_front( osgDB::getFilePath(path) );
    opt->setPropertyNode(prop_root ? prop_root: static_propRoot.get());
    opt->setModelData(data);

    if (load2DPanels) {
       opt->setLoadPanel(static_panelFunc);
    }

    opt->setAutoTooltipsMaster(autoTooltipsMaster);
    opt->setAutoTooltipsMasterMax(autoTooltipsMasterMax);

    osg::Node *n = loadFile(path, opt.get());
    if (n && n->getName().empty())
        n->setName("Direct loaded model \"" + path + "\"");
    return n;

}

osg::Node*
SGModelLib::loadDeferredModel(const string &path, SGPropertyNode *prop_root,
                             SGModelData *data)
{
    osg::ProxyNode* proxyNode = new osg::ProxyNode;
    proxyNode->setLoadingExternalReferenceMode(osg::ProxyNode::DEFER_LOADING_TO_DATABASE_PAGER);
    proxyNode->setFileName(0, path);

    osg::ref_ptr<SGReaderWriterOptions> opt;
    opt = SGReaderWriterOptions::copyOrCreate(osgDB::Registry::instance()->getOptions());
    opt->getDatabasePathList().push_front( osgDB::getFilePath(path) );
    opt->setPropertyNode(prop_root ? prop_root: static_propRoot.get());
    opt->setModelData(data);
    opt->setLoadPanel(static_panelFunc);
    std::string lext = SGPath(path).lower_extension();
    if ((lext == "ac") || (lext == "obj")) {
        opt->setInstantiateEffects(true);
    }

    if (!prop_root || prop_root->getBoolValue("/sim/rendering/cache", true))
        opt->setObjectCacheHint(osgDB::Options::CACHE_ALL);
    else
        opt->setObjectCacheHint(osgDB::Options::CACHE_NONE);
    proxyNode->setDatabaseOptions(opt.get());

    return proxyNode;
}


/*
 * Load a set of models at different LOD range_nearest
 *
 */
osg::PagedLOD*
SGModelLib::loadPagedModel(SGPropertyNode *prop_root, SGModelData *data, SGModelLOD model_lods)
{
    unsigned int simple_models = 0;
    osg::PagedLOD *plod = new osg::PagedLOD;

    osg::ref_ptr<SGReaderWriterOptions> opt;
    opt = SGReaderWriterOptions::copyOrCreate(osgDB::Registry::instance()->getOptions());
    opt->setPropertyNode(prop_root ? prop_root: static_propRoot.get());
    opt->setModelData(data);
    opt->setLoadPanel(static_panelFunc);
    if (!prop_root || prop_root->getBoolValue("/sim/rendering/cache", true))
        opt->setObjectCacheHint(osgDB::Options::CACHE_ALL);
    else
        opt->setObjectCacheHint(osgDB::Options::CACHE_NONE);

    for(unsigned int i = 0; i < model_lods.getNumLODs(); i++) {
      SGModelLOD::ModelLOD lod = model_lods.getModelLOD(i);
      plod->setName("Paged LOD for \"" + lod.path + "\"");
      plod->setFileName(i, lod.path);
      plod->setRange(i, lod.min_range, lod.max_range);
      plod->setMinimumExpiryTime(i, prop_root->getDoubleValue("/sim/rendering/plod-minimum-expiry-time-secs", 180.0 ) );

      std::string lext = SGPath(lod.path).lower_extension();
      if ((lext == "ac") || (lext == "obj")) {
        simple_models++;
      }
    }

    // If all we have are simple models, then we can instantiate effects in
    // the loader.
    if (simple_models == model_lods.getNumLODs()) opt->setInstantiateEffects(true);

    plod->setDatabaseOptions(opt.get());

    return plod;
}

osg::PagedLOD*
SGModelLib::loadPagedModel(const string &path, SGPropertyNode *prop_root,
                           SGModelData *data)
{
    SGModelLOD model_lods;
    model_lods.insert(path, 0.0, 50.0*SG_NM_TO_METER);
    return SGModelLib::loadPagedModel(prop_root, data, model_lods);
}

osg::PagedLOD*
SGModelLib::loadPagedModel(std::vector<string> paths, SGPropertyNode *prop_root,
                           SGModelData *data)
{
    SGModelLOD model_lods;
    for(unsigned int i = 0; i < paths.size(); i++) {
      // We don't have any range data, so simply set them all up to full range.
      // Some other code will update the LoD ranges later.  (AIBase::updateLOD)
      model_lods.insert(paths[i], 0.0, 50.0*SG_NM_TO_METER);
    }
    return SGModelLib::loadPagedModel(prop_root, data, model_lods);
}

// end of modellib.cxx
