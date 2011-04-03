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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <osgDB/ReadFile>
#include <osgDB/Input>
#include <osgDB/ParameterOutput>
#include <osgDB/DatabasePager>

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/OSGVersion.hxx>

#include "modellib.hxx"
#include "SGReaderWriterXMLOptions.hxx"
#include "SGPagedLOD.hxx"

#include <simgear/math/SGMath.hxx>

using namespace osg;
using namespace simgear;

bool SGPagedLOD::_cache = true;

SGPagedLOD::SGPagedLOD()
        : PagedLOD()
{
}

SGPagedLOD::~SGPagedLOD()
{
    //SG_LOG(SG_GENERAL, SG_ALERT, "SGPagedLOD::~SGPagedLOD(" << getFileName(0) << ")");
}

SGPagedLOD::SGPagedLOD(const SGPagedLOD& plod,const CopyOp& copyop)
        : osg::PagedLOD(plod, copyop)
#if !SG_PAGEDLOD_HAS_OPTIONS
        ,  _readerWriterOptions(plod._readerWriterOptions)
#endif
{
}

void
SGPagedLOD::setReaderWriterOptions(osgDB::ReaderWriter::Options *options)
{
    if (_cache)
        options->setObjectCacheHint(osgDB::ReaderWriter::Options::CACHE_ALL);
    else
        options->setObjectCacheHint(osgDB::ReaderWriter::Options::CACHE_NONE);
#if SG_PAGEDLOD_HAS_OPTIONS
    setDatabaseOptions(options);
#else
    _readerWriterOptions = options;
#endif
}

bool SGPagedLOD::addChild(osg::Node *child)
{
    if (!PagedLOD::addChild(child))
        return false;

    setRadius(getBound().radius());
    setCenter(getBound().center());
    return true;
}

// Work around interface change in osgDB::DatabasePager::requestNodeFile
struct NodePathProxy
{
    NodePathProxy(NodePath& nodePath)
        : _nodePath(nodePath)
    {
    }
    operator Group* () { return static_cast<Group*>(_nodePath.back()); }
    operator NodePath& () { return _nodePath; }
    NodePath& _nodePath;
};

void SGPagedLOD::forceLoad(osgDB::DatabasePager *dbp, FrameStamp* framestamp,
                           NodePath& path)
{
    //SG_LOG(SG_GENERAL, SG_ALERT, "SGPagedLOD::forceLoad(" <<
    //getFileName(getNumChildren()) << ")");
    unsigned childNum = getNumChildren();
    setTimeStamp(childNum, 0);
    double priority=1.0;
    dbp->requestNodeFile(getFileName(childNum), NodePathProxy(path),
                         priority, framestamp,
                         getDatabaseRequest(childNum),
                         getReaderWriterOptions());
}

bool SGPagedLOD_writeLocalData(const Object& obj, osgDB::Output& fw)
{
    return true;
}

namespace
{
osgDB::RegisterDotOsgWrapperProxy sgPagedLODProxy
(
    new SGPagedLOD,
    "simgear::SGPagedLOD",
    "Object Node LOD PagedLOD SGPagedLOD Group",
    0,
    &SGPagedLOD_writeLocalData
    );
}
