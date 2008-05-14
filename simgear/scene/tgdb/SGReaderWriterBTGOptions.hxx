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
#ifndef SGREADERWRITERBTGOPTIONS_HXX
#define SGREADERWRITERBTGOPTIONS_HXX

#include <osgDB/ReaderWriter>
#include <simgear/scene/tgdb/obj.hxx>
class SGReaderWriterBTGOptions : public osgDB::ReaderWriter::Options {
public:
    SGReaderWriterBTGOptions() {}
    SGReaderWriterBTGOptions(const std::string& str):
        osgDB::ReaderWriter::Options(str),
        _matlib(0), _calcLights(false),
        _useRandomObjects(false),
        _useRandomVegetation(false)
    {}

    SGReaderWriterBTGOptions(const SGReaderWriterBTGOptions& options,
            const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
        osgDB::ReaderWriter::Options(options, copyop),
        _matlib(options._matlib), _calcLights(options._calcLights),
        _useRandomObjects(options._useRandomObjects),
        _useRandomVegetation(options._useRandomVegetation)
    {
    }
    SGMaterialLib* getMatlib() const { return _matlib; }
    void setMatlib (SGMaterialLib* matlib) { _matlib = matlib; }
    bool getCalcLights() const { return _calcLights; }
    void setCalcLights(bool calcLights)  { _calcLights = calcLights; }
    bool getUseRandomObjects() const { return _useRandomObjects; }
    bool getUseRandomVegetation() const { return _useRandomVegetation; }
    void setUseRandomObjects(bool useRandomObjects)
    {
        _useRandomObjects = useRandomObjects;
    }
    void setUseRandomVegetation(bool useRandomVegetation)
    {
        _useRandomVegetation = useRandomVegetation;
    }

protected:
    virtual ~SGReaderWriterBTGOptions() {}
    SGMaterialLib* _matlib;
    bool _calcLights;
    bool _useRandomObjects;
    bool _useRandomVegetation;
};
#endif
