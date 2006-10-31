/* -*-c++-*-
 *
 * Copyright (C) 2006 Mathias Froehlich 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef SG_SCENE_TEXTURESTATEATTRIBUTEVISITOR_HXX
#define SG_SCENE_TEXTURESTATEATTRIBUTEVISITOR_HXX

#include <osg/Geode>
#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/StateSet>

class SGTextureStateAttributeVisitor : public osg::NodeVisitor {
public:
  SGTextureStateAttributeVisitor();

  virtual void apply(int textureUnit, osg::StateSet::RefAttributePair& refAttr);
  virtual void apply(int textureUnit, osg::StateSet::AttributeList& attrList);
  virtual void apply(osg::StateSet::TextureAttributeList& attrList);
  virtual void apply(osg::StateSet* stateSet);
  virtual void apply(osg::Node& node);
  virtual void apply(osg::Geode& node);
};

#endif
