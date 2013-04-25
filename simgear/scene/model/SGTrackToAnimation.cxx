// TrackTo animation
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include "SGRotateTransform.hxx"
#include "SGTrackToAnimation.hxx"

#include <simgear/scene/util/OsgMath.hxx>
#include <osg/Transform>
#include <cassert>

/**
 *
 */
static osg::NodePath requireNodePath( osg::Node* node,
                                      osg::Node* haltTraversalAtNode = 0 )
{
  const osg::NodePathList node_paths =
    node->getParentalNodePaths(haltTraversalAtNode);
  return node_paths.at(0);
}

/**
 * Get a subpath of an osg::NodePath
 *
 * @param path  Path to extract subpath from
 * @param start Number of elements to skip from start of #path
 *
 * @return Subpath starting with node at position #start
 */
static osg::NodePath subPath( const osg::NodePath& path,
                              size_t start )
{
  if( start >= path.size() )
    return osg::NodePath();

  osg::NodePath np(path.size() - start);
  for(size_t i = start; i < path.size(); ++i)
    np[i - start] = path[i];

  return np;
}

/**
 * Visitor to find a group by its name.
 */
class FindGroupVisitor:
  public osg::NodeVisitor
{
  public:

    FindGroupVisitor(const std::string& name):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _name(name),
        _group(0)
    {
      if( name.empty() )
        SG_LOG(SG_IO, SG_WARN, "FindGroupVisitor: empty name provided");
    }

    osg::Group* getGroup() const
    {
      return _group;
    }

    virtual void apply(osg::Group& group)
    {
      if( _name != group.getName() )
        return traverse(group);

      if( !_group )
        _group = &group;
      else
        SG_LOG(SG_IO, SG_WARN, "FindGroupVisitor: name not unique");
    }

  protected:

    std::string _name;
    osg::Group *_group;
};

//------------------------------------------------------------------------------
class SGTrackToAnimation::UpdateCallback:
  public osg::NodeCallback
{
  public:
    UpdateCallback( osg::Group* target,
                    const SGTrackToAnimation* anim ):
      _target(target)
    {
      setName("SGTrackToAnimation::UpdateCallback");

      _node_center = toOsg( anim->readVec3("center", "-m") );
      _target_center = toOsg( anim->readVec3("target-center", "-m") );
      _lock_axis = toOsg( anim->readVec3("lock-axis") );
      _track_axis = toOsg( anim->readVec3("track-axis") );

      if( _lock_axis.normalize() == 0.0 )
      {
        anim->log(SG_WARN, "invalid lock-axis");
        _lock_axis.set(0, 1, 0);
      }

      for(;;)
      {
        float proj = _lock_axis * _track_axis;
        if( proj != 0.0 )
        {
          anim->log(SG_WARN, "track-axis not perpendicular to lock-axis");

          // Make tracking axis perpendicular to locked axis
          _track_axis -= _lock_axis * proj;
        }

        if( _track_axis.normalize() == 0.0 )
        {
          anim->log(SG_WARN, "invalid track-axis");
          if( std::fabs(_lock_axis.x()) < 0.1 )
            _track_axis.set(1, 0, 0);
          else
            _track_axis.set(0, 1, 0);
        }
        else
          break;
      }

      _up_axis = _lock_axis ^ _track_axis;
    }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
      SGRotateTransform* tf = static_cast<SGRotateTransform*>(node);

      if( _target )
      {
        // Get path to animated node and calculated simplified paths to the
        // nearest common parent node of both the animated node and the target
        // node

                                  // start at parent to not get false results by
                                  // also including animation transformation
        osg::NodePath node_path = requireNodePath(node->getParent(0)),
                      target_path = requireNodePath(_target);
        size_t tp_size = target_path.size(),
               np_size = node_path.size();
        size_t common_parents = 0;

        for(; common_parents < std::min(tp_size, np_size); ++common_parents)
        {
          if( target_path[common_parents] != node_path[common_parents] )
            break;
        }

        _node_path = subPath(node_path, common_parents);
        _target_path = subPath(target_path, common_parents);
        _target = 0;

        tf->setCenter( toSG(_node_center) );
        tf->setAxis( toSG(_lock_axis) );
      }

      osg::Vec3 target_pos = ( osg::computeLocalToWorld(_target_path)
                             * osg::computeWorldToLocal(_node_path)
                             ).preMult(_target_center),
                       dir = target_pos - _node_center;

      // Ensure direction is perpendicular to lock axis
      float proj = _lock_axis * dir;
      if( proj != 0.0 )
        dir -= _lock_axis * proj;

      float x = dir * _track_axis,
            y = dir * _up_axis;
      tf->setAngleRad( std::atan2(y, x) );

      traverse(node, nv);
    }
  protected:
    osg::Vec3       _node_center,
                    _target_center,
                    _lock_axis,
                    _track_axis,
                    _up_axis;
    osg::Group     *_target;
    osg::NodePath   _node_path,
                    _target_path;
};

//------------------------------------------------------------------------------
SGTrackToAnimation::SGTrackToAnimation( osg::Node* node,
                                        const SGPropertyNode* configNode,
                                        SGPropertyNode* modelRoot ):
  SGAnimation(configNode, modelRoot),
  _target_group(0)
{
  std::string target = configNode->getStringValue("target-name");
  FindGroupVisitor target_finder(target);
  node->accept(target_finder);

  if( !(_target_group = target_finder.getGroup()) )
    log(SG_ALERT, "target not found: '" + target + '\'');
}

//------------------------------------------------------------------------------
osg::Group* SGTrackToAnimation::createAnimationGroup(osg::Group& parent)
{
  if( !_target_group )
    return 0;

  SGRotateTransform* transform = new SGRotateTransform;
  transform->setName("locked-track animation");
  transform->setUpdateCallback(new UpdateCallback(_target_group, this));
  parent.addChild(transform);
  return transform;
}

//------------------------------------------------------------------------------
void SGTrackToAnimation::log(sgDebugPriority p, const std::string& msg) const
{
  SG_LOG
  (
    SG_IO,
    p,
    // TODO handle multiple object-names?
    "SGTrackToAnimation(" << getConfig()->getStringValue("object-name") << "): "
    << msg
  );
}
