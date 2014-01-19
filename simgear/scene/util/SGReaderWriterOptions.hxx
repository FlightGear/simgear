// Copyright (C) 2007 Tim Moore timoore@redhat.com
// Copyright (C) 2008 Till Busch buti@bux.at
// Copyright (C) 2011 Mathias Froehlich
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

#ifndef SGREADERWRITEROPTIONS_HXX
#define SGREADERWRITEROPTIONS_HXX 1

#include <osgDB/Options>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/material/matlib.hxx>

#include <simgear/props/props.hxx>

class SGPropertyNode;


namespace simgear
{

class SGReaderWriterOptions : public osgDB::Options {
public:
    SGReaderWriterOptions() :
        _materialLib(0),
        _load_panel(0),
        _model_data(0),
        _instantiateEffects(false)
    { }
    SGReaderWriterOptions(const std::string& str) :
        osgDB::Options(str),
        _materialLib(0),
        _load_panel(0),
        _model_data(0),
        _instantiateEffects(false)
    { }
    SGReaderWriterOptions(const osgDB::Options& options,
                          const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY) :
        osgDB::Options(options, copyop),
        _materialLib(0),
        _load_panel(0),
        _model_data(0),
        _instantiateEffects(false)
    { }
    SGReaderWriterOptions(const SGReaderWriterOptions& options,
                          const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY) :
        osgDB::Options(options, copyop),
        _propertyNode(options._propertyNode),
        _materialLib(options._materialLib),
        _load_panel(options._load_panel),
        _model_data(options._model_data),
        _instantiateEffects(options._instantiateEffects)
    { }

    META_Object(simgear, SGReaderWriterOptions);

    const SGSharedPtr<SGPropertyNode>& getPropertyNode() const
    { return _propertyNode; }
    void setPropertyNode(const SGSharedPtr<SGPropertyNode>& propertyNode)
    { _propertyNode = propertyNode; }

    SGMaterialLibPtr getMaterialLib() const
    { return _materialLib; }
    void setMaterialLib(SGMaterialLib* materialLib)
    { _materialLib = materialLib; }

    typedef osg::Node *(*panel_func)(SGPropertyNode *);

    panel_func getLoadPanel() const
    { return _load_panel; }
    void setLoadPanel(panel_func pf)
    { _load_panel=pf; }

    SGModelData *getModelData() const
    { return _model_data.get(); }
    void setModelData(SGModelData *modelData)
    { _model_data=modelData; }

    bool getInstantiateEffects() const
    { return _instantiateEffects; }
    void setInstantiateEffects(bool instantiateEffects)
    { _instantiateEffects = instantiateEffects; }

    static SGReaderWriterOptions* copyOrCreate(const osgDB::Options* options);
    static SGReaderWriterOptions* fromPath(const std::string& path);

protected:
    virtual ~SGReaderWriterOptions();

private:
    SGSharedPtr<SGPropertyNode> _propertyNode;
    SGSharedPtr<SGMaterialLib> _materialLib;
    osg::Node *(*_load_panel)(SGPropertyNode *);
    osg::ref_ptr<SGModelData> _model_data;
    bool _instantiateEffects;
};

}

#endif
