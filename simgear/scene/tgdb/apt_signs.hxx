// apt_signs.hxx -- build airport signs on the fly
//
// Written by Curtis Olson, started July 2001.
//
// Copyright (C) 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// $Id$


#ifndef _SG_APT_SIGNS_HXX
#define _SG_APT_SIGNS_HXX

#include <simgear/compiler.h>

#include <string>
#include <memory> // for auto-ptr

#include <osg/Node>

class SGMaterialLib;            // forward declaration
class SGGeod;

// Generate a generic sign
osg::Node* SGMakeSign( SGMaterialLib *matlib, const std::string& content );

namespace simgear
{

class AirportSignBuilder
{
public:
    AirportSignBuilder(SGMaterialLib* mats, const SGGeod& center);
    ~AirportSignBuilder();
    
    void addSign(const SGGeod& pos, double heading, const std::string& content, int size);
        
    osg::Node* getSignsGroup();
private:
    class AirportSignBuilderPrivate;
    std::auto_ptr<AirportSignBuilderPrivate> d;
};

} // of namespace simgear

#endif // _SG_APT_SIGNS_HXX
