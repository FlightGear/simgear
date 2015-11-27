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

#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>

#include <simgear/constants.h>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/misc/ResourceManager.hxx>

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
    return p.str();
  }

  // finally hand on to standard OSG behaviour
  return osgDB::findDataFile(file, opts);
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
    if (boost::iends_with(path, ".ac"))
        options->setInstantiateEffects(true);
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
                       bool load2DPanels)
{
    osg::ref_ptr<SGReaderWriterOptions> opt;
    opt = SGReaderWriterOptions::copyOrCreate(osgDB::Registry::instance()->getOptions());
    opt->getDatabasePathList().push_front( osgDB::getFilePath(path) );
    opt->setPropertyNode(prop_root ? prop_root: static_propRoot.get());
    opt->setModelData(data);
    
    if (load2DPanels) {
       opt->setLoadPanel(static_panelFunc);
    }
    
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
    if (SGPath(path).lower_extension() == "ac")
        opt->setInstantiateEffects(true);
    if (!prop_root || prop_root->getBoolValue("/sim/rendering/cache", true))
        opt->setObjectCacheHint(osgDB::Options::CACHE_ALL);
    else
        opt->setObjectCacheHint(osgDB::Options::CACHE_NONE);
    proxyNode->setDatabaseOptions(opt.get());

    return proxyNode;
}

osg::PagedLOD*
SGModelLib::loadPagedModel(const string &path, SGPropertyNode *prop_root,
                           SGModelData *data)
{
    osg::PagedLOD *plod = new osg::PagedLOD;
    plod->setName("Paged LOD for \"" + path + "\"");
    plod->setFileName(0, path);
    plod->setRange(0, 0.0, 50.0*SG_NM_TO_METER);
    plod->setMinimumExpiryTime( 0, prop_root->getDoubleValue("/sim/rendering/plod-minimum-expiry-time-secs", 180.0 ) );

    osg::ref_ptr<SGReaderWriterOptions> opt;
    opt = SGReaderWriterOptions::copyOrCreate(osgDB::Registry::instance()->getOptions());
    opt->setPropertyNode(prop_root ? prop_root: static_propRoot.get());
    opt->setModelData(data);
    opt->setLoadPanel(static_panelFunc);
    if (SGPath(path).lower_extension() == "ac")
        opt->setInstantiateEffects(true);
    if (!prop_root || prop_root->getBoolValue("/sim/rendering/cache", true))
        opt->setObjectCacheHint(osgDB::Options::CACHE_ALL);
    else
        opt->setObjectCacheHint(osgDB::Options::CACHE_NONE);
    plod->setDatabaseOptions(opt.get());
    return plod;
}

// end of modellib.cxx
