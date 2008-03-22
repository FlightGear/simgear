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

#include <osg/PagedLOD>
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

    META_Node(osg, PagedLOD);

    // virtual void traverse(osg::NodeVisitor& nv);
    virtual void forceLoad(osgDB::DatabasePager* dbp);

    // reimplemented to notify the loading through ModelData
    bool addChild(osg::Node *child);

    void setReaderWriterOptions(osgDB::ReaderWriter::Options *o) {
        _readerWriterOptions=o;
        _readerWriterOptions->setObjectCacheHint(osgDB::ReaderWriter::Options::CACHE_NONE);
    }

    osgDB::ReaderWriter::Options * getReaderWriterOptions() {
        return _readerWriterOptions.get();
    }

protected:
    virtual ~SGPagedLOD();
    osg::ref_ptr<osgDB::ReaderWriter::Options> _readerWriterOptions;
    SGPropertyNode_ptr _props;
};
}
#endif
