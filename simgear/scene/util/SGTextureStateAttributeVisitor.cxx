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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "SGTextureStateAttributeVisitor.hxx"

SGTextureStateAttributeVisitor::SGTextureStateAttributeVisitor() :
  osg::NodeVisitor(osg::NodeVisitor::NODE_VISITOR,
                   osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}

void
SGTextureStateAttributeVisitor::apply(int textureUnit, osg::StateSet::RefAttributePair& refAttr)
{
}

void
SGTextureStateAttributeVisitor::apply(int textureUnit, osg::StateSet::AttributeList& attrList)
{
  osg::StateSet::AttributeList::iterator i;
  i = attrList.begin();
  while (i != attrList.end()) {
    apply(textureUnit, i->second);
    ++i;
  }
}

void
SGTextureStateAttributeVisitor::apply(osg::StateSet::TextureAttributeList& attrList)
{
  for (unsigned i = 0; i < attrList.size(); ++i)
    apply(i, attrList[i]);
}

void
SGTextureStateAttributeVisitor::apply(osg::StateSet* stateSet)
{
  if (!stateSet)
    return;
  apply(stateSet->getTextureAttributeList());
}

void
SGTextureStateAttributeVisitor::apply(osg::Node& node)
{
  apply(node.getStateSet());
  traverse(node);
}

void
SGTextureStateAttributeVisitor::apply(osg::Geode& node)
{
  unsigned nDrawables = node.getNumDrawables();
  for (unsigned i = 0; i < nDrawables; ++i)
    apply(node.getDrawable(i)->getStateSet());
  apply(node.getStateSet());
  traverse(node);
}
