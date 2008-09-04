// Copyright (C) 2007 Tim Moore timoore@redhat.com
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
//
#ifndef SGREADERWRITERXMLOPTIONS_HXX
#define SGREADERWRITERXMLOPTIONS_HXX 1

#include <osgDB/ReaderWriter>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/props/props.hxx>

class SGPropertyNode;

namespace simgear
{
class SGModelData;

class SGReaderWriterXMLOptions : public osgDB::ReaderWriter::Options
{
public:
    typedef osg::Node *(*panel_func)(SGPropertyNode *);

    SGReaderWriterXMLOptions():
            osgDB::ReaderWriter::Options(),
            _prop_root(0),
            _load_panel(0),
            _model_data(0) {}

    SGReaderWriterXMLOptions(const std::string& str):
            osgDB::ReaderWriter::Options(str),
            _prop_root(0),
            _load_panel(0),
            _model_data(0) {}

    SGReaderWriterXMLOptions(const SGReaderWriterXMLOptions& options,
                             const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
            osgDB::ReaderWriter::Options(options, copyop),
            _prop_root(options._prop_root),
            _load_panel(options._load_panel),
            _model_data(options._model_data) {}

    SGReaderWriterXMLOptions(const osgDB::ReaderWriter::Options& options,
                             const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
            osgDB::ReaderWriter::Options(options, copyop),
            _prop_root(0),
            _load_panel(0),
            _model_data(0) {}

    SGPropertyNode *getPropRoot() const {
        return _prop_root;
    }
    panel_func getLoadPanel() const {
        return _load_panel;
    }
    SGModelData *getModelData() const {
        return _model_data.get();
    }

    void setPropRoot(SGPropertyNode *p) {
        _prop_root=p;
    }
    void setLoadPanel(panel_func pf) {
        _load_panel=pf;
    }
    void setModelData(SGModelData *d) {
        _model_data=d;
    }

protected:
    virtual ~SGReaderWriterXMLOptions() {}

    SGPropertyNode_ptr _prop_root;
    osg::Node *(*_load_panel)(SGPropertyNode *);
    osg::ref_ptr<SGModelData> _model_data;
};

}
#endif
