/* -*-c++-*-
 *
 * Copyright (C) 2008 Tim Moore
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

#include <osg/Drawable>
#include <osg/Geode>

#include "NodeAndDrawableVisitor.hxx"

namespace simgear
{
using namespace osg;

NodeAndDrawableVisitor::NodeAndDrawableVisitor(
  osg::NodeVisitor::TraversalMode tm
):
  NodeVisitor(tm)
{
}

NodeAndDrawableVisitor::NodeAndDrawableVisitor(
  osg::NodeVisitor::VisitorType type,
  osg::NodeVisitor::TraversalMode tm
):
  NodeVisitor(type, tm)
{
}

NodeAndDrawableVisitor::~NodeAndDrawableVisitor()
{
}

void NodeAndDrawableVisitor::apply(osg::Node& node)
{
    traverse(node);
}

void NodeAndDrawableVisitor::apply(osg::Drawable& Drawable)
{
}

void NodeAndDrawableVisitor::traverse(osg::Node& node)
{
    TraversalMode tm = getTraversalMode();
    if (tm == TRAVERSE_NONE) {
        return;
    } else if (tm == TRAVERSE_PARENTS) {
        NodeVisitor::traverse(node);
        return;
    }
    Geode* geode = dynamic_cast<Geode*>(&node);
    if (geode) {
        unsigned numDrawables = geode->getNumDrawables();
        for (unsigned i = 0; i < numDrawables; ++i)
            apply(*geode->getDrawable(i));
    } else {
        NodeVisitor::traverse(node);
    }
}
}
