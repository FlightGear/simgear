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

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>

#include <simgear/constants.h>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>

#include "SGPagedLOD.hxx"
#include "SGReaderWriterXML.hxx"
#include "SGReaderWriterXMLOptions.hxx"

#include "modellib.hxx"

#include <simgear/math/SGMath.hxx>

using std::string;
using namespace simgear;

osgDB::RegisterReaderWriterProxy<SGReaderWriterXML> g_readerWriter_XML_Proxy;
ModelRegistryCallbackProxy<LoadOnlyCallback> g_xmlCallbackProxy("xml");

SGPropertyNode_ptr SGModelLib::static_propRoot;
SGModelLib::panel_func SGModelLib::static_panelFunc = NULL;
SGModelLib::resolve_func SGModelLib::static_resolver = NULL;

////////////////////////////////////////////////////////////////////////
// Implementation of SGModelLib.
////////////////////////////////////////////////////////////////////////
void SGModelLib::init(const string &root_dir)
{
    osgDB::Registry::instance()->getDataFilePathList().push_front(root_dir);
}

void SGModelLib::setPropRoot(SGPropertyNode* root)
{
  static_propRoot = root;
}
    
void SGModelLib::setPanelFunc(panel_func pf)
{
  static_panelFunc = pf;
}

void SGModelLib::setResolveFunc(resolve_func rf)
{
  static_resolver = rf;
}

std::string SGModelLib::findDataFile(const std::string& file, 
  const osgDB::ReaderWriter::Options* opts)
{
  if (static_resolver) {
    SGPath p = static_resolver(file);
    if (p.exists()) {
      return p.str();
    }
  }
  
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
osg::Node* loadFile(const string& path, SGReaderWriterXMLOptions* options)
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
                       SGModelData *data)
{
    osg::ref_ptr<SGReaderWriterXMLOptions> opt = new SGReaderWriterXMLOptions(*(osgDB::Registry::instance()->getOptions()));
    opt->setPropRoot(prop_root ? prop_root: static_propRoot.get());
    opt->setModelData(data);
    opt->setLoadPanel(static_panelFunc);
    
    osg::Node *n = loadFile(path, opt.get());
    if (n && n->getName().empty())
        n->setName("Direct loaded model \"" + path + "\"");
    return n;

}

osg::Node*
SGModelLib::loadPagedModel(const string &path, SGPropertyNode *prop_root,
                           SGModelData *data)
{
    SGPagedLOD *plod = new SGPagedLOD;
    plod->setName("Paged LOD for \"" + path + "\"");
    plod->setFileName(0, path);
    plod->setRange(0, 0.0, 50.0*SG_NM_TO_METER);

    osg::ref_ptr<SGReaderWriterXMLOptions> opt
        = new SGReaderWriterXMLOptions(*(osgDB::Registry::instance()
                                         ->getOptions()));
    opt->setPropRoot(prop_root ? prop_root: static_propRoot.get());
    opt->setModelData(data);
    opt->setLoadPanel(static_panelFunc);
    if (boost::iends_with(path, ".ac"))
        opt->setInstantiateEffects(true);
    plod->setReaderWriterOptions(opt.get());
    return plod;
}

// end of modellib.cxx
