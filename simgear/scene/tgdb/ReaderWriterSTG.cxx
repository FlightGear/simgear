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
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/tgdb/apt_signs.hxx>
#include <simgear/scene/tgdb/obj.hxx>

#include "SGOceanTile.hxx"

using namespace simgear;

static SGBucket getBucketFromFileName(const std::string& fileName)
{
    std::istringstream ss(osgDB::getNameLessExtension(fileName));
    long index;
    ss >> index;
    if (ss.fail())
        return SGBucket();
    return SGBucket(index);
}

static bool
loadStgFile(const std::string& absoluteFileName, osg::Group& group, const osgDB::Options* options)
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
            node = osgDB::readRefNodeFile(path.str(),
                                          staticOptions.get());
                
            if (!node.valid()) {
                SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                        << ": Failed to load OBJECT_BASE '"
                        << name << "'" );
            }
            
        } else if ( token == "OBJECT" ) {
            node = osgDB::readRefNodeFile(path.str(),
                                          staticOptions.get());
                
            if (!node.valid()) {
                SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                        << ": Failed to load OBJECT '"
                        << name << "'" );
            }
            
        } else {
            double lon, lat, elev, hdg;
            in >> lon >> lat >> elev >> hdg;
            
            // Always OK to load
            if ( token == "OBJECT_STATIC" ) {
                /// Hmm, the findDataFile should happen downstream
                std::string absName = osgDB::findDataFile(name,
                                                          staticOptions.get());
                osg::ref_ptr<SGReaderWriterOptions> opt;
                opt = new SGReaderWriterOptions(*staticOptions);
                if (SGPath(absName).lower_extension() == "ac")
                    opt->setInstantiateEffects(true);
                else
                    opt->setInstantiateEffects(false);
                node = osgDB::readRefNodeFile(absName, opt.get());
                
                if (!node.valid()) {
                    SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                            << ": Failed to load OBJECT_STATIC '"
                            << name << "'" );
                }
                
            } else if ( token == "OBJECT_SHARED" ) {
                osg::ref_ptr<SGReaderWriterOptions> opt;
                opt = new SGReaderWriterOptions(*sharedOptions);
                
                /// Hmm, the findDataFile should happen in the downstream readers
                std::string absName = osgDB::findDataFile(name, opt.get());
                
                osg::ProxyNode* proxyNode = new osg::ProxyNode;
                proxyNode->setLoadingExternalReferenceMode(osg::ProxyNode::DEFER_LOADING_TO_DATABASE_PAGER);
                proxyNode->setFileName(0, absName);
                if (SGPath(absName).lower_extension() == "ac")
                    opt->setInstantiateEffects(true);
                else
                    opt->setInstantiateEffects(false);
                proxyNode->setDatabaseOptions(opt.get());
                node = proxyNode;
                
                if (!node.valid()) {
                    SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                            << ": Failed to load OBJECT_SHARED '"
                            << name << "'" );
                }
                
            } else if ( token == "OBJECT_SIGN" ) {
                node = SGMakeSign(staticOptions->getMaterialLib(), name);
                
            } else if ( token == "OBJECT_RUNWAY_SIGN" ) {
                node = SGMakeRunwaySign(staticOptions->getMaterialLib(), name);
                
            } else {
                SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                        << ": Unknown token '" << token << "'" );
            }
            
            if (node.valid()) {
                osg::Matrix matrix;
                matrix = makeZUpFrame(SGGeod::fromDegM(lon, lat, elev));
                matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(hdg),
                                               osg::Vec3(0, 0, 1)));
                
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

    return has_base;
}
    
static osg::Node*
loadTileByFileName(const string& fileName, const osgDB::Options* options)
{
    SG_LOG(SG_TERRAIN, SG_INFO, "Loading tile " << fileName);

    // We treat 123.stg different than ./123.stg.
    // The difference is that ./123.stg as well as any absolute path
    // really loads the given stg file and only this.
    // In contrast 123.stg uses the search paths to load a set of stg
    // files spread across the scenery directories.
    std::string simpleFileName = osgDB::getSimpleFileName(fileName);
    SGBucket bucket = getBucketFromFileName(simpleFileName);
    osg::ref_ptr<osg::Group> group = new osg::Group;
    if (simpleFileName != fileName || !options) {
        // This is considered a real existing file.
        // We still apply the search path algorithms for relative files.
        loadStgFile(osgDB::findDataFile(fileName, options), *group, options);
        return group.release();
    }

    // This is considered a meta file, so apply the scenery path search
    const osgDB::FilePathList& filePathList = options->getDatabasePathList();
    std::string basePath = bucket.gen_base_path();
    // Stop scanning once an object base is found
    bool foundBase = false;
    for (osgDB::FilePathList::const_iterator i = filePathList.begin();
         i != filePathList.end() && !foundBase; ++i) {
        SGPath objects(*i);
        objects.append("Objects");
        objects.append(basePath);
        objects.append(simpleFileName);
        if (loadStgFile(objects.str(), *group, options))
            foundBase = true;
        
        SGPath terrain(*i);
        terrain.append("Terrain");
        terrain.append(basePath);
        terrain.append(simpleFileName);
        if (loadStgFile(terrain.str(), *group, options))
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
    return group.release();
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

//#define SLOW_PAGER 1
#ifdef SLOW_PAGER
#include <unistd.h>
#endif

osgDB::ReaderWriter::ReadResult
ReaderWriterSTG::readNode(const std::string& fileName,
                          const osgDB::Options* options) const
{
    osg::Node* result = loadTileByFileName(fileName, options);
    // For debugging race conditions
#ifdef SLOW_PAGER
    sleep(5);
#endif
    if (result)
        return result;
    else
        return ReadResult::FILE_NOT_HANDLED;
}

