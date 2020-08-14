// matmodel.cxx -- class to handle models tied to a material property
//
// Written by David Megginson, started May 1998.
//
// Copyright (C) 1998 - 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <map>
#include <mutex>

#include <osg/AlphaFunc>
#include <osg/Group>
#include <osg/LOD>
#include <osg/StateSet>
#include <osg/Transform>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/scene/model/modellib.hxx>

#include "matmodel.hxx"

using namespace simgear;
using std::string;
using std::map;
////////////////////////////////////////////////////////////////////////
// Implementation of SGMatModel.
////////////////////////////////////////////////////////////////////////

SGMatModel::SGMatModel (const SGPropertyNode * node, double range_m)
  : _models_loaded(false),
    _coverage_m2(node->getDoubleValue("coverage-m2", 1000000)),
    _spacing_m(node->getDoubleValue("spacing-m", 20)),
    _range_m(range_m)
{
				// Sanity check
  if (_coverage_m2 < 1000) {
    SG_LOG(SG_INPUT, SG_ALERT, "Random object coverage " << _coverage_m2
	   << " is too small, forcing, to 1000");
    _coverage_m2 = 1000;
  }

				// Note all the model paths
  std::vector <SGPropertyNode_ptr> path_nodes = node->getChildren("path");
  for (unsigned int i = 0; i < path_nodes.size(); i++)
    _paths.push_back(path_nodes[i]->getStringValue());

				// Note the heading type
  string hdg = node->getStringValue("heading-type", "fixed");
  if (hdg == "fixed") {
    _heading_type = HEADING_FIXED;
  } else if (hdg == "billboard") {
    _heading_type = HEADING_BILLBOARD;
  } else if (hdg == "random") {
    _heading_type = HEADING_RANDOM;
  } else if (hdg == "mask") {
    _heading_type = HEADING_MASK;
  } else {
    _heading_type = HEADING_FIXED;
    SG_LOG(SG_INPUT, SG_ALERT, "Unknown heading type: " << hdg
	   << "; using 'fixed' instead.");
  }

  // uncomment to preload models
  // load_models();
}

SGMatModel::~SGMatModel ()
{
}

int
SGMatModel::get_model_count( SGPropertyNode *prop_root )
{
  load_models( prop_root );
  return _models.size();
}

inline void
SGMatModel::load_models( SGPropertyNode *prop_root )
{
    std::lock_guard<std::mutex> g(_loadMutex);

    // Load model only on demand
    if (!_models_loaded) {
        for (unsigned int i = 0; i < _paths.size(); i++) {
            osg::Node* entity = SGModelLib::loadModel(_paths[i], prop_root);
            if (entity != 0) {
                // FIXME: this stuff can be handled
                // in the XML wrapper as well (at least,
                // the billboarding should be handled
                // there).

                if (_heading_type == HEADING_BILLBOARD) {
                    // if the model is a billboard, it is likely :
                    // 1. a branch with only leaves,
                    // 2. a tree or a non rectangular shape faked by transparency
                    // We add alpha clamp then
                    osg::StateSet* stateSet = entity->getOrCreateStateSet();
                    osg::AlphaFunc* alphaFunc =
                        new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.01f);
                    stateSet->setAttributeAndModes(alphaFunc,
                                                   osg::StateAttribute::OVERRIDE);
                    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                }

                _models.push_back(entity);

            } else {
                SG_LOG(SG_INPUT, SG_ALERT, "Failed to load object " << _paths[i]);
                // Ensure the vector contains something, otherwise get_random_model below fails
                _models.push_back(new osg::Node());
            }
        }
  }
  _models_loaded = true;
}

osg::Node*
SGMatModel::get_random_model( SGPropertyNode *prop_root, mt* seed )
{
  load_models( prop_root ); // comment this out if preloading models
  int nModels = _models.size();
  return _models[mt_rand(seed) * nModels].get();
}

double
SGMatModel::get_coverage_m2 () const
{
  return _coverage_m2;
}

double SGMatModel::get_range_m() const
{
  return _range_m;
}

double SGMatModel::get_spacing_m() const
{
  return _spacing_m;
}

double SGMatModel::get_randomized_range_m(mt* seed) const
{
  double lrand = mt_rand(seed);
  
  // Note that the LoD is not completely randomized.
  // 10% at 2   * range_m
  // 30% at 1.5 * range_m
  // 60% at 1   * range_m
  if (lrand < 0.1) return 2   * _range_m;
  if (lrand < 0.4) return 1.5 * _range_m;
  else return _range_m;
}

SGMatModel::HeadingType
SGMatModel::get_heading_type () const
{
  return _heading_type;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGMatModelGroup.
////////////////////////////////////////////////////////////////////////

SGMatModelGroup::SGMatModelGroup (SGPropertyNode * node, float default_object_range)
  : _range_m(node->getDoubleValue("range-m", default_object_range))
{
				// Load the object subnodes
  std::vector<SGPropertyNode_ptr> object_nodes =
    ((SGPropertyNode *)node)->getChildren("object");
  for (unsigned int i = 0; i < object_nodes.size(); i++) {
    const SGPropertyNode * object_node = object_nodes[i];
    if (object_node->hasChild("path"))
      _objects.push_back(new SGMatModel(object_node, _range_m));
    else
      SG_LOG(SG_INPUT, SG_ALERT, "No path supplied for object");
  }
}

SGMatModelGroup::~SGMatModelGroup ()
{
}

double
SGMatModelGroup::get_range_m () const
{
  return _range_m;
}

int
SGMatModelGroup::get_object_count () const
{
  return _objects.size();
}

SGMatModel *
SGMatModelGroup::get_object (int index) const
{
  return _objects[index];
}

// end of matmodel.cxx
