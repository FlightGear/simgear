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

#ifdef ENABLE_GDAL
#include <simgear/scene/dem/SGDem.hxx>
#endif

class SGPropertyNode;
class SGPath;

typedef std::vector < std::string > string_list;

namespace simgear
{

class SGReaderWriterOptions : public osgDB::Options {
public:
    enum LoadOriginHint
    {
        ORIGIN_MODEL,
        ORIGIN_EFFECTS,
        ORIGIN_EFFECTS_NORMALIZED,
        ORIGIN_SPLASH_SCREEN,
        ORIGIN_CANVAS,
    };

    //SGReaderWriterOptions* cloneOptions(const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY) const { return static_cast<SGReaderWriterOptions*>(clone(copyop)); }

    SGReaderWriterOptions() :
        _materialLib(0),
        _load_panel(0),
        _instantiateEffects(false),
        _instantiateMaterialEffects(false),
        _LoadOriginHint(ORIGIN_MODEL)
    { }
    SGReaderWriterOptions(const std::string& str) :
        osgDB::Options(str),
        _materialLib(0),
        _load_panel(0),
        _instantiateEffects(false),
        _instantiateMaterialEffects(false),
        _LoadOriginHint(ORIGIN_MODEL)
    { }
    SGReaderWriterOptions(const osgDB::Options& options,
                          const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY) :
        osgDB::Options(options, copyop),
        _materialLib(0),
        _load_panel(0),
        _instantiateEffects(false),
        _instantiateMaterialEffects(false),
        _LoadOriginHint(ORIGIN_MODEL)
    { }
    SGReaderWriterOptions(const SGReaderWriterOptions& options,
                          const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY) : osgDB::Options(options, copyop),
                                                                                   _propertyNode(options._propertyNode),
                                                                                   _materialLib(options._materialLib),
#ifdef ENABLE_GDAL
                                                                                   _dem(options._dem),
#endif
        _load_panel(options._load_panel),
        _model_data(options._model_data),
        _instantiateEffects(options._instantiateEffects),
        _instantiateMaterialEffects(options._instantiateMaterialEffects),
        _materialName(options._materialName),
        _sceneryPathSuffixes(options._sceneryPathSuffixes),
        _LoadOriginHint(ORIGIN_MODEL),
        _errorContext(options._errorContext)
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

#ifdef ENABLE_GDAL
    SGDemPtr getDem() const
    { return _dem; }
    void setDem(SGDem* dem)
    { _dem = dem; }
#endif

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

    bool getInstantiateMaterialEffects() const
    { return _instantiateMaterialEffects; }
    void setInstantiateMaterialEffects(bool instantiateMaterialEffects)
    { _instantiateMaterialEffects = instantiateMaterialEffects; }

    string getMaterialName() const
    { return _materialName; }
    void setMaterialName(string materialName)
    { _materialName = materialName; }

    const string_list& getSceneryPathSuffixes() const
    { return _sceneryPathSuffixes; }

    void setSceneryPathSuffixes(const string_list& suffixes)
    { _sceneryPathSuffixes = suffixes; }

    static SGReaderWriterOptions* copyOrCreate(const osgDB::Options* options);
    static SGReaderWriterOptions* fromPath(const SGPath& path);

    void setLocation(double lon, double lat)
    { _geod = SGGeod::fromDeg(lon, lat); }

    const SGGeod& getLocation() const
    { return _geod; }

    // the load origin defines where the load request has come from.
    // example usage; to allow the DDS Texture Cache (DTC) to ignore 
    // any texture that is used in a shader, as these often have special values
    // encoded into the channels that aren't suitable for conversion.
    void setLoadOriginHint(LoadOriginHint _v) const { _LoadOriginHint = _v; } 
    LoadOriginHint getLoadOriginHint() const { return _LoadOriginHint; }

    using ErrorContext = std::map<std::string, std::string>;

    void addErrorContext(const std::string& key, const std::string& value);

    ErrorContext getErrorContext() const
    {
        return _errorContext;
    }

protected:
    virtual ~SGReaderWriterOptions();

private:
    SGSharedPtr<SGPropertyNode> _propertyNode;
    SGSharedPtr<SGMaterialLib> _materialLib;
#ifdef ENABLE_GDAL
    SGSharedPtr<SGDem> _dem;
#endif

    osg::Node *(*_load_panel)(SGPropertyNode *);
    osg::ref_ptr<SGModelData> _model_data;
    
    bool _instantiateEffects;
    bool _instantiateMaterialEffects;
    string _materialName;
    string_list _sceneryPathSuffixes;
    SGGeod _geod;
    mutable LoadOriginHint _LoadOriginHint;
    ErrorContext _errorContext;
};

}

#endif
