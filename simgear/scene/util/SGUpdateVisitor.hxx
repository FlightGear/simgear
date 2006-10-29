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

#ifndef SG_SCENE_UPDATEVISITOR_HXX
#define SG_SCENE_UPDATEVISITOR_HXX

#include <osg/NodeVisitor>
#include <osgUtil/UpdateVisitor>

#include <osg/io_utils>

class SGUpdateVisitor : public osgUtil::UpdateVisitor {
public:
  SGUpdateVisitor()
  {
//     setTraversalMode(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN);
  }
//   virtual void apply(osg::Transform& transform)
//   {
//     osg::Matrix matrix = mModelViewMatrix;
//     transform.computeLocalToWorldMatrix(mModelViewMatrix, this);

//     handle_callbacks_and_traverse(transform);

//     mModelViewMatrix = matrix;
//   }

//   virtual osg::Vec3 getEyePoint() const
//   {
//     osg::Matrix matrix;
//     matrix.invert(mModelViewMatrix);
//     return matrix.preMult(osg::Vec3(0, 0, 0));
//   }

// protected:
//   osg::Matrix mModelViewMatrix;
};

// #include <osg/NodeCallback>

// class SGNodeCallback : public osg::NodeCallback {
// public:
//   virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
//   {
//   }
// };

#endif
