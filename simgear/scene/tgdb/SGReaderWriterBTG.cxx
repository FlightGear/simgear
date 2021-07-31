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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <osgDB/FileNameUtils>
#include <osgDB/Registry>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/debug/Reporting.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/structure/exception.hxx>

#include "SGReaderWriterBTG.hxx"
#include "obj.hxx"

using namespace simgear;

SGReaderWriterBTG::SGReaderWriterBTG()
{
    supportsExtension("btg", "SimGear btg database format");
    // supportsExtension("btg.gz", "SimGear btg database format");
}

SGReaderWriterBTG::~SGReaderWriterBTG()
{
}

const char* SGReaderWriterBTG::className() const
{
    return "BTG Database reader";
}

bool
SGReaderWriterBTG::acceptsExtension(const std::string& extension) const
{
    // trick the osg extensions match algorithm to accept btg.gz files.
    if (osgDB::convertToLowerCase(extension) == "gz")
        return true;
    return osgDB::ReaderWriter::acceptsExtension(extension);
}

osgDB::ReaderWriter::ReadResult
SGReaderWriterBTG::readNode(const std::string& fileName,
                            const osgDB::Options* options) const
{
    const SGReaderWriterOptions* sgOptions;
    sgOptions = dynamic_cast<const SGReaderWriterOptions*>(options);
    osg::Node* result = NULL;
    simgear::ErrorReportContext ec{"btg", fileName};
    try {
        simgear::ReportBadAllocGuard g;
        result = SGLoadBTG(fileName, sgOptions);
        if (!result)
            return ReadResult::FILE_NOT_HANDLED;
    } catch (sg_exception& e) {
        simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::BTGLoad,
                               "Failed to load BTG file:" + e.getFormattedMessage(),
                               e.getLocation());
        return ReadResult::ERROR_IN_READING_FILE;
    } catch (std::bad_alloc&) {
        simgear::reportFailure(simgear::LoadFailure::OutOfMemory, simgear::ErrorCode::BTGLoad,
                               "Out of memory loading BTG:" + fileName, sg_location{fileName});
        return ReadResult::ERROR_IN_READING_FILE;
    }

    return result;
}


typedef ModelRegistryCallback<DefaultProcessPolicy, NoCachePolicy,
                              NoOptimizePolicy,
                              NoSubstitutePolicy, BuildGroupBVHPolicy>
BTGCallback;

namespace
{
ModelRegistryCallbackProxy<BTGCallback> g_btgCallbackProxy("btg");
}
