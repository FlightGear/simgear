// Copyright (C) 2007 Tim Moore timoore@redhat.com
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

#include <osgDB/FileNameUtils>
#include <osgDB/Registry>

#include <simgear/scene/model/ModelRegistry.hxx>

#include "SGReaderWriterBTGOptions.hxx"
#include "SGReaderWriterBTG.hxx"
#include "obj.hxx"

using namespace simgear;

// SGReaderWriterBTGOptions static value here to avoid an additional,
// tiny source file.

std::string SGReaderWriterBTGOptions::defaultOptions;

const char* SGReaderWriterBTG::className() const
{
    return "BTG Database reader";
}

bool SGReaderWriterBTG::acceptsExtension(const std::string& extension) const
{
    return (osgDB::equalCaseInsensitive(extension, "gz")
            || osgDB::equalCaseInsensitive(extension, "btg"));
}

osgDB::ReaderWriter::ReadResult
SGReaderWriterBTG::readNode(const std::string& fileName,
                            const osgDB::ReaderWriter::Options* options) const
{
    std::string ext = osgDB::getLowerCaseFileExtension(fileName);
    if(!acceptsExtension(ext))
        return ReadResult::FILE_NOT_HANDLED;
    if (osgDB::equalCaseInsensitive(ext, "gz")) {
        std::string btgFileName = osgDB::getNameLessExtension(fileName);
        if (!acceptsExtension(
                osgDB::getLowerCaseFileExtension(btgFileName))) {
            return ReadResult::FILE_NOT_HANDLED;
        }
    }
    SGMaterialLib* matlib = 0;
    bool calcLights = false;
    bool useRandomObjects = false;
    bool useRandomVegetation = false;
    const SGReaderWriterBTGOptions* btgOptions
        = dynamic_cast<const SGReaderWriterBTGOptions*>(options);
    if (btgOptions) {
        matlib = btgOptions->getMatlib();
        calcLights = btgOptions->getCalcLights();
        useRandomObjects = btgOptions->getUseRandomObjects();
        useRandomVegetation = btgOptions->getUseRandomVegetation();
    }
    osg::Node* result = SGLoadBTG(fileName, matlib, calcLights,
                                  useRandomObjects,
                                  useRandomVegetation);
    if (result)
        return result;
    else
        return ReadResult::FILE_NOT_HANDLED;
}

namespace
{
ModelRegistryCallbackProxy<LoadOnlyCallback> g_btgCallbackProxy("btg");
}
