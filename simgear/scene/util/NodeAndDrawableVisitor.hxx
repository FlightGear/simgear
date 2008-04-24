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
#ifndef SIMGEAR_NODEANDDRAWABLEVISITOR_HXX
#define SIMGEAR_NODEANDDRAWABLEVISITOR_HXX 1

#include <osg/Drawable>
#include <osg/NodeVisitor>

namespace simgear
{
/** A node visitor that descends into Drawables too.
 */
class NodeAndDrawableVisitor : public osg::NodeVisitor
{
public:
    NodeAndDrawableVisitor(osg::NodeVisitor::TraversalMode tm);
    NodeAndDrawableVisitor(osg::NodeVisitor::VisitorType type,
                           osg::NodeVisitor::TraversalMode tm);
    virtual ~NodeAndDrawableVisitor();
    using osg::NodeVisitor::apply;
    virtual void apply(osg::Node& node);
    /** Visit a Drawable node. Note that you cannot write an apply()
    method with an argument that is a subclass of Drawable and expect
    it to be called, because this visitor can't add the double dispatch
    machinery of NodeVisitor to the existing OSG Drawable subclasses.
    */
    virtual void apply(osg::Drawable& drawable);
    // hides NodeVisitor::traverse
    void traverse(osg::Node& node);
};

}
#endif
