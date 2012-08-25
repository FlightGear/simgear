// tileentry.cxx -- routines to handle a scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include "ReaderWriterSTG.hxx"

#include <osg/MatrixTransform>
#include <osg/ProxyNode>

#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>
#include <osgDB/ReadFile>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/tgdb/apt_signs.hxx>
#include <simgear/scene/tgdb/obj.hxx>

#include "SGOceanTile.hxx"

using namespace simgear;

/// Ok, this is a hack - we do not exactly know if it's an airport or not.
/// This feature might also vanish again later. This is currently to
/// support testing an external ai component that just loads the the airports
/// and supports ground queries on only these areas.
static bool isAirportBtg(const std::string& name)
{
    if (name.size() < 8)
        return false;
    if (name.substr(4, 8) != ".btg")
        return false;
    for (unsigned i = 0; i < 4; ++i) {
        if (name[i] < 'A' || 'Z' < name[i])
            continue;
        return true;
    }
    return false;
}

static SGBucket bucketIndexFromFileName(const std::string& fileName)
{
  // Extract the bucket from the filename
  std::istringstream ss(osgDB::getNameLessExtension(fileName));
  long index;
  ss >> index;
  if (ss.fail())
    return SGBucket();
  
  return SGBucket(index);
}

static bool hasOptionalValue(sg_gzifstream &in)
{
    while ( (in.peek() != '\n') && (in.peek() != '\r')
            && isspace(in.peek()) ) {
        in.get();
    }
    if ( isdigit(in.peek()) || (in.peek() == '-') ){
        return true;
    } else {
        return false;
    }
}

ReaderWriterSTG::ReaderWriterSTG()
{
    supportsExtension("stg", "SimGear stg database format");
}

ReaderWriterSTG::~ReaderWriterSTG()
{
}

const char* ReaderWriterSTG::className() const
{
    return "STG Database reader";
}

osgDB::ReaderWriter::ReadResult
ReaderWriterSTG::readNode(const std::string& fileName, const osgDB::Options* options) const
{
    // We treat 123.stg different than ./123.stg.
    // The difference is that ./123.stg as well as any absolute path
    // really loads the given stg file and only this.
    // In contrast 123.stg uses the search paths to load a set of stg
    // files spread across the scenery directories.
    if (osgDB::getSimpleFileName(fileName) != fileName)
        return readStgFile(fileName, options);

    // For stg meta files, we need options for the search path.
    if (!options)
        return ReadResult::FILE_NOT_FOUND;



    SG_LOG(SG_TERRAIN, SG_INFO, "Loading tile " << fileName);

    SGBucket bucket(bucketIndexFromFileName(fileName));
    std::string basePath = bucket.gen_base_path();

    osg::ref_ptr<osg::Group> group = new osg::Group;

    // Stop scanning once an object base is found
    bool foundBase = false;
    // This is considered a meta file, so apply the scenery path search
    const osgDB::FilePathList& filePathList = options->getDatabasePathList();
    for (osgDB::FilePathList::const_iterator i = filePathList.begin();
         i != filePathList.end() && !foundBase; ++i) {
        SGPath objects(*i);
        objects.append("Objects");
        objects.append(basePath);
        objects.append(fileName);
        if (readStgFile(objects.str(), bucket, *group, options))
            foundBase = true;
        
        SGPath terrain(*i);
        terrain.append("Terrain");
        terrain.append(basePath);
        terrain.append(fileName);
        if (readStgFile(terrain.str(), bucket, *group, options))
            foundBase = true;
    }
    
    // ... or generate an ocean tile on the fly
    if (!foundBase) {
        SG_LOG(SG_TERRAIN, SG_INFO, "  Generating ocean tile");
        
        osg::ref_ptr<SGReaderWriterOptions> opt;
        opt = SGReaderWriterOptions::copyOrCreate(options);
        osg::Node* node = SGOceanTile(bucket, opt->getMaterialLib());
        if ( node ) {
            group->addChild(node);
        } else {
            SG_LOG( SG_TERRAIN, SG_ALERT,
                    "Warning: failed to generate ocean tile!" );
        }
    }

    return group.get();
}

osgDB::ReaderWriter::ReadResult
ReaderWriterSTG::readStgFile(const std::string& fileName, const osgDB::Options* options) const
{
    // This is considered a real existing file.
    // We still apply the search path algorithms for relative files.
    osg::ref_ptr<osg::Group> group = new osg::Group;
    std::string path = osgDB::findDataFile(fileName, options);
    readStgFile(path, bucketIndexFromFileName(path), *group, options);

    return group.get();
}

bool
ReaderWriterSTG::readStgFile(const std::string& absoluteFileName,
                             const SGBucket& bucket,
                             osg::Group& group, const osgDB::Options* options) const
{
    if (absoluteFileName.empty())
        return false;

    sg_gzifstream in( absoluteFileName );
    if ( !in.is_open() )
        return false;

    SG_LOG(SG_TERRAIN, SG_INFO, "Loading stg file " << absoluteFileName);
    
    std::string filePath = osgDB::getFilePath(absoluteFileName);

    osg::ref_ptr<SGReaderWriterOptions> staticOptions;
    staticOptions = SGReaderWriterOptions::copyOrCreate(options);
    staticOptions->getDatabasePathList().clear();
    staticOptions->getDatabasePathList().push_back(filePath);
    staticOptions->setObjectCacheHint(osgDB::Options::CACHE_NONE);
    
    osg::ref_ptr<SGReaderWriterOptions> sharedOptions;
    sharedOptions = SGReaderWriterOptions::copyOrCreate(options);
    sharedOptions->getDatabasePathList().clear();
    
    SGPath path = filePath;
    path.append(".."); path.append(".."); path.append("..");
    sharedOptions->getDatabasePathList().push_back(path.str());
    std::string fg_root = options->getPluginStringData("SimGear::FG_ROOT");
    sharedOptions->getDatabasePathList().push_back(fg_root);

    // do only load airport btg files.
    bool onlyAirports = options->getPluginStringData("SimGear::FG_ONLY_AIRPORTS") == "ON";
    // do only load terrain btg files
    bool onlyTerrain = options->getPluginStringData("SimGear::FG_ONLY_TERRAIN") == "ON";

    simgear::AirportSignBuilder signBuilder(staticOptions->getMaterialLib(), bucket.get_center());
  
    bool has_base = false;
    while ( ! in.eof() ) {
        std::string token;
        in >> token;
        
        // No comment
        if ( token.empty() || token[0] == '#' ) {
            in >> ::skipeol;
            continue;
        }
        
        // Then there is always a name
        std::string name;
        in >> name;
        
        SGPath path = filePath;
        path.append(name);
        
        osg::ref_ptr<osg::Node> node;
        if ( token == "OBJECT_BASE" ) {
            // Load only once (first found)
            SG_LOG( SG_TERRAIN, SG_BULK, "    " << token << " " << name );
            
            has_base = true;

            if (!onlyAirports || isAirportBtg(name)) {
                node = osgDB::readRefNodeFile(path.str(),
                                              staticOptions.get());
                
                if (!node.valid()) {
                    SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                            << ": Failed to load OBJECT_BASE '"
                            << name << "'" );
                }
            }
            
        } else if ( token == "OBJECT" ) {
            if (!onlyAirports || isAirportBtg(name)) {
                node = osgDB::readRefNodeFile(path.str(),
                                              staticOptions.get());
                
                if (!node.valid()) {
                    SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                            << ": Failed to load OBJECT '"
                            << name << "'" );
                }
            }
            
        } else {
            double lon, lat, elev, hdg;
            in >> lon >> lat >> elev >> hdg;
            
            // Always OK to load
            if ( token == "OBJECT_STATIC" ) {
                if (!onlyTerrain) {
                    osg::ref_ptr<SGReaderWriterOptions> opt;
                    opt = new SGReaderWriterOptions(*staticOptions);
                    osg::ProxyNode* proxyNode = new osg::ProxyNode;
                    proxyNode->setLoadingExternalReferenceMode(osg::ProxyNode::DEFER_LOADING_TO_DATABASE_PAGER);
                    /// Hmm, the findDataFile should happen downstream
                    std::string absName = osgDB::findDataFile(name, opt.get());
                    proxyNode->setFileName(0, absName);
                    if (SGPath(absName).lower_extension() == "ac")
                    {
                        proxyNode->setNodeMask( ~simgear::MODELLIGHT_BIT );
                        opt->setInstantiateEffects(true);
                    }
                    else
                        opt->setInstantiateEffects(false);
                    proxyNode->setDatabaseOptions(opt.get());
                    node = proxyNode;
                    
                    if (!node.valid()) {
                        SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                                << ": Failed to load OBJECT_STATIC '"
                                << name << "'" );
                    }
                }
                
            } else if ( token == "OBJECT_SHARED" ) {
                if (!onlyTerrain) {
                    osg::ref_ptr<SGReaderWriterOptions> opt;
                    opt = new SGReaderWriterOptions(*sharedOptions);
                    /// Hmm, the findDataFile should happen in the downstream readers
                    std::string absName = osgDB::findDataFile(name, opt.get());
                    if (SGPath(absName).lower_extension() == "ac")
                        opt->setInstantiateEffects(true);
                    else
                        opt->setInstantiateEffects(false);
                    node = osgDB::readRefNodeFile(absName, opt.get());
                    if (SGPath(absName).lower_extension() == "ac")
                        node->setNodeMask( ~simgear::MODELLIGHT_BIT );
                    
                    if (!node.valid()) {
                        SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                                << ": Failed to load OBJECT_SHARED '"
                                << name << "'" );
                    }
                }
                
            } else if ( token == "OBJECT_SIGN" ) {
                int size(-1);

                if ( hasOptionalValue(in) ){
                    in >> size;
                }
                if (!onlyTerrain)
                    signBuilder.addSign(SGGeod::fromDegM(lon, lat, elev), hdg, name, size);
            } else {
                SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                        << ": Unknown token '" << token << "'" );
            }
            
            if (node.valid()) {
                osg::Matrix matrix;
                matrix = makeZUpFrame(SGGeod::fromDegM(lon, lat, elev));
                matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(hdg),
                                               osg::Vec3(0, 0, 1)));

                if ( hasOptionalValue(in) ){
                    double pitch(0.0), roll(0.0);
                    in >> pitch >> roll;

                    matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(pitch),
                                                   osg::Vec3(0, 1, 0)));
                    matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(roll),
                                                   osg::Vec3(1, 0, 0)));
                }

                osg::MatrixTransform* matrixTransform;
                matrixTransform = new osg::MatrixTransform(matrix);
                matrixTransform->setDataVariance(osg::Object::STATIC);
                matrixTransform->addChild(node.get());
                node = matrixTransform;
            }
        }
        
        if (node.valid())
            group.addChild(node.get());
        
        in >> ::skipeol;
    }
  
    if (signBuilder.getSignsGroup())
        group.addChild(signBuilder.getSignsGroup());

    return has_base;
}
