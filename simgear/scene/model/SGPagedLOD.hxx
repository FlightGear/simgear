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

#ifndef SGPAGEDLOD_HXX
#define SGPAGEDLOD_HXX 1

#include <simgear/structure/OSGVersion.hxx>

#define SG_PAGEDLOD_HAS_OPTIONS \
    (SG_OSG_VERSION >= 29005 \
     || (SG_OSG_VERSION < 29000 && SG_OSG_VERSION >= 28003))

#include <osg/PagedLOD>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>
#include <simgear/props/props.hxx>

namespace osgDB {
class DatabasePager;
}


namespace simgear
{

class SGPagedLOD : public osg::PagedLOD
{
public:
    SGPagedLOD();

    SGPagedLOD(const SGPagedLOD&,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

    META_Node(simgear, SGPagedLOD);

    // virtual void traverse(osg::NodeVisitor& nv);
    virtual void forceLoad(osgDB::DatabasePager* dbp,
                           osg::FrameStamp* framestamp,
                           osg::NodePath& path);

    // reimplemented to notify the loading through ModelData
    bool addChild(osg::Node *child);

    void setReaderWriterOptions(osgDB::ReaderWriter::Options *options) {
        options->setObjectCacheHint(osgDB::ReaderWriter::Options::CACHE_NONE);
#if SG_PAGEDLOD_HAS_OPTIONS
        setDatabaseOptions(options);
#else
        _readerWriterOptions = options;
#endif
    }

    osgDB::ReaderWriter::Options* getReaderWriterOptions() {
#if SG_PAGEDLOD_HAS_OPTIONS
        return static_cast<osgDB::ReaderWriter::Options*>(getDatabaseOptions());
#else
        return _readerWriterOptions.get();
#endif
    }

protected:
    virtual ~SGPagedLOD();
#if SG_PAGEDLOD_HAS_OPTIONS
    osg::ref_ptr<osgDB::ReaderWriter::Options> _readerWriterOptions;
#endif
};
}
#endif
