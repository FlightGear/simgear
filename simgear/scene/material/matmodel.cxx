// matmodel.cxx -- class to handle models tied to a material property
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2003  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <map>
SG_USING_STD(map);

#include <simgear/compiler.h>

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/scene/model/modellib.hxx>

#include "matmodel.hxx"


////////////////////////////////////////////////////////////////////////
// Local static functions.
////////////////////////////////////////////////////////////////////////

/**
 * Internal method to test whether a file exists.
 *
 * TODO: this should be moved to a SimGear library of local file
 * functions.
 */
static inline bool
local_file_exists( const string& path ) {
    sg_gzifstream in( path );
    if ( ! in.is_open() ) {
	return false;
    } else {
	return true;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGMatModel.
////////////////////////////////////////////////////////////////////////

SGMatModel::SGMatModel (const SGPropertyNode * node, double range_m)
  : _models_loaded(false),
    _coverage_m2(node->getDoubleValue("coverage-m2", 1000000)),
    _range_m(range_m)
{
				// Sanity check
  if (_coverage_m2 < 1000) {
    SG_LOG(SG_INPUT, SG_ALERT, "Random object coverage " << _coverage_m2
	   << " is too small, forcing, to 1000");
    _coverage_m2 = 1000;
  }

				// Note all the model paths
  vector <SGPropertyNode_ptr> path_nodes = node->getChildren("path");
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
  for (unsigned int i = 0; i < _models.size(); i++) {
    if (_models[i] != 0) {
      _models[i]->deRef();
      _models[i] = 0;
    }
  }
}

int
SGMatModel::get_model_count( SGModelLib *loader,
                                   const string &fg_root,
                                   SGPropertyNode *prop_root,
                                   double sim_time_sec )
{
  load_models( loader, fg_root, prop_root, sim_time_sec );
  return _models.size();
}

inline void
SGMatModel::load_models ( SGModelLib *loader,
                                const string &fg_root,
                                SGPropertyNode *prop_root,
                                double sim_time_sec )
{
				// Load model only on demand
  if (!_models_loaded) {
    for (unsigned int i = 0; i < _paths.size(); i++) {
      ssgEntity *entity = loader->load_model( fg_root, _paths[i],
                                              prop_root, sim_time_sec );
      if (entity != 0) {
                                // FIXME: this stuff can be handled
                                // in the XML wrapper as well (at least,
                                // the billboarding should be handled
                                // there).
	float ranges[] = {0, _range_m};
	ssgRangeSelector * lod = new ssgRangeSelector;
        lod->ref();
        lod->setRanges(ranges, 2);
	if (_heading_type == HEADING_BILLBOARD) {
	  ssgCutout * cutout = new ssgCutout(false);
	  cutout->addKid(entity);
	  lod->addKid(cutout);
	} else {
	  lod->addKid(entity);
	}
	_models.push_back(lod);
      } else {
	SG_LOG(SG_INPUT, SG_ALERT, "Failed to load object " << _paths[i]);
      }
    }
  }
  _models_loaded = true;
}

ssgEntity *
SGMatModel::get_model( int index,
                               SGModelLib *loader,
                               const string &fg_root,
                               SGPropertyNode *prop_root,
                               double sim_time_sec )
{
  load_models( loader, fg_root, prop_root, sim_time_sec ); // comment this out if preloading models
  return _models[index];
}

ssgEntity *
SGMatModel::get_random_model( SGModelLib *loader,
                                      const string &fg_root,
                                      SGPropertyNode *prop_root,
                                      double sim_time_sec )
{
  load_models( loader, fg_root, prop_root, sim_time_sec ); // comment this out if preloading models
  int nModels = _models.size();
  int index = int(sg_random() * nModels);
  if (index >= nModels)
    index = 0;
  return _models[index];
}

double
SGMatModel::get_coverage_m2 () const
{
  return _coverage_m2;
}

SGMatModel::HeadingType
SGMatModel::get_heading_type () const
{
  return _heading_type;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGMatModelGroup.
////////////////////////////////////////////////////////////////////////

SGMatModelGroup::SGMatModelGroup (SGPropertyNode * node)
  : _range_m(node->getDoubleValue("range-m", 2000))
{
				// Load the object subnodes
  vector<SGPropertyNode_ptr> object_nodes =
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
  for (unsigned int i = 0; i < _objects.size(); i++) {
    delete _objects[i];
    _objects[i] = 0;
  }
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
