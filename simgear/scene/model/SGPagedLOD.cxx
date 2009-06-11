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

#include <osgDB/ReadFile>

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/OSGVersion.hxx>

#include "modellib.hxx"
#include "SGReaderWriterXMLOptions.hxx"
#include "SGPagedLOD.hxx"

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
    //SG_LOG(SG_GENERAL, SG_ALERT, "SGPagedLOD::addChild(" << getFileName(getNumChildren()) << ")");
    if (!PagedLOD::addChild(child))
        return false;

    setRadius(getBound().radius());
    setCenter(getBound().center());
    SGReaderWriterXMLOptions* opts;
    opts = dynamic_cast<SGReaderWriterXMLOptions*>(_readerWriterOptions.get());
    if(opts)
    {
        osg::ref_ptr<SGModelData> d = opts->getModelData();
        if(d.valid())
            d->modelLoaded(getFileName(getNumChildren()-1),
                           d->getConfigProperties(), this);
    }
    return true;
}

void SGPagedLOD::forceLoad(osgDB::DatabasePager *dbp)
{
    //SG_LOG(SG_GENERAL, SG_ALERT, "SGPagedLOD::forceLoad(" <<
    //getFileName(getNumChildren()) << ")");
    unsigned childNum = getNumChildren();
    setTimeStamp(childNum, 0);
    double priority=1.0;
    dbp->requestNodeFile(getFileName(childNum),this,priority,0,
                         getDatabaseRequest(childNum),
                         _readerWriterOptions.get());
}

