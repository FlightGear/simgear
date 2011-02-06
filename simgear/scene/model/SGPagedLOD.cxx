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

SGPagedLOD::SGPagedLOD()
        : PagedLOD()
{
}

SGPagedLOD::~SGPagedLOD()
{
    //SG_LOG(SG_GENERAL, SG_ALERT, "SGPagedLOD::~SGPagedLOD(" << getFileName(0) << ")");
}

SGPagedLOD::SGPagedLOD(const SGPagedLOD& plod,const CopyOp& copyop)
        : osg::PagedLOD(plod, copyop),
        _readerWriterOptions(plod._readerWriterOptions)
{
}

bool SGPagedLOD::addChild(osg::Node *child)
{
    if (!PagedLOD::addChild(child))
        return false;

    setRadius(getBound().radius());
    setCenter(getBound().center());
    return true;
}

void SGPagedLOD::forceLoad(osgDB::DatabasePager *dbp, osg::FrameStamp* framestamp)
{
    //SG_LOG(SG_GENERAL, SG_ALERT, "SGPagedLOD::forceLoad(" <<
    //getFileName(getNumChildren()) << ")");
    unsigned childNum = getNumChildren();
    setTimeStamp(childNum, 0);
    double priority=1.0;
#if SG_OSG_MIN_VERSION_REQUIRED(2,9,11)
    // osgDB::DatabasePager::requestNodeFile changed with OSG 2.9.11 

    #ifdef __GNUC__
        #warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        #warning !! Your version of OpenSceneGraph is currently unsupported.
        #warning !! Use latest stable OSG (2.8.3) or a OSG developer release up to 2.9.10.
        #warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    #else
        // Alas! Why is MSVC different (again)? :)
        #pragma WARNING( Your version of OpenSceneGraph is currently unsupported. )
        #pragma WARNING( Use latest stable OSG (2.8.3) or a OSG developer release up to 2.9.10. )
    #endif

    //TODO We need to adapt to OSG 2.9.11 (and future releases). This doesn't work yet...
    dbp->requestNodeFile(getFileName(childNum),this,priority,framestamp,
                         getDatabaseRequest(childNum),
                         _readerWriterOptions.get());
#else
    // OSG revisions up to 2.9.10 
    dbp->requestNodeFile(getFileName(childNum),this,priority,framestamp,
                         getDatabaseRequest(childNum),
                         _readerWriterOptions.get());
#endif
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
