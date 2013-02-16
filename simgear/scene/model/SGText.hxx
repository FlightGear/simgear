// SGText.hxx - Manage text in the scene graph
// Copyright (C) 2009 Torsten Dreyer Torsten (_at_) t3r *dot* de
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

#ifndef _SGTEXT_HXX
#define _SGTEXT_HXX 1

#include <osgDB/ReaderWriter>
#include <osg/Group>

#include <simgear/props/props.hxx>

class SGText : public osg::NodeCallback 
{
public:
  static osg::Node * appendText(const SGPropertyNode* configNode, SGPropertyNode* modelRoot, const osgDB::Options* options);
private:
  class UpdateCallback;
};

#endif

