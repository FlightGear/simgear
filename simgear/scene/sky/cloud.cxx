// cloud.cxx -- model a single cloud layer
//
// Written by Curtis Olson, started June 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <sstream>

#include <math.h>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/ShadeModel>
#include <osg/TexEnv>
#include <osg/TexEnvCombine>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osg/Fog>

#include <simgear/math/sg_random.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/screen/extensions.hxx>

#include "newcloud.hxx"
#include "cloudfield.hxx"
#include "cloud.hxx"

using namespace simgear;
using namespace osg;

const std::string SGCloudLayer::SG_CLOUD_OVERCAST_STRING = "overcast";
const std::string SGCloudLayer::SG_CLOUD_BROKEN_STRING = "broken";
const std::string SGCloudLayer::SG_CLOUD_SCATTERED_STRING = "scattered";
const std::string SGCloudLayer::SG_CLOUD_FEW_STRING = "few";
const std::string SGCloudLayer::SG_CLOUD_CIRRUS_STRING = "cirrus";
const std::string SGCloudLayer::SG_CLOUD_CLEAR_STRING = "clear";

// Constructor
SGCloudLayer::SGCloudLayer( const SGPath &tex_path ) :
    cloud_root(new osg::Switch),
    cloud_alpha(1.0),
    texture_path(tex_path),
    layer_span(0.0),
    layer_asl(0.0),
    layer_thickness(0.0),
    layer_transition(0.0),
    layer_visibility(25.0),
    layer_coverage(SG_CLOUD_CLEAR),
    speed(0.0),
    direction(0.0),
    max_alpha(1.0)
{
  layer3D = new SGCloudField();
  cloud_root->addChild(layer3D->getNode(), false);
}

// Destructor
SGCloudLayer::~SGCloudLayer()
{
  delete layer3D;
}

float
SGCloudLayer::getSpan_m () const
{
    return layer_span;
}

void
SGCloudLayer::setSpan_m (float span_m)
{
    if (span_m != layer_span) {
        layer_span = span_m;
    }
}

float
SGCloudLayer::getElevation_m () const
{
    return layer_asl;
}

void
SGCloudLayer::setElevation_m (float elevation_m, bool set_span)
{
    layer_asl = elevation_m;

    if (set_span) {
        if (elevation_m > 4000)
            setSpan_m(  elevation_m * 10 );
        else
            setSpan_m( 40000 );
    }
}

float
SGCloudLayer::getThickness_m () const
{
    return layer_thickness;
}

void
SGCloudLayer::setThickness_m (float thickness_m)
{
    layer_thickness = thickness_m;
}

float
SGCloudLayer::getVisibility_m() const
{
    return layer_visibility;
}

void
SGCloudLayer::setVisibility_m (float visibility_m)
{
    layer_visibility = visibility_m;
}

float
SGCloudLayer::getTransition_m () const
{
    return layer_transition;
}

void
SGCloudLayer::setTransition_m (float transition_m)
{
    layer_transition = transition_m;
}

SGCloudLayer::Coverage
SGCloudLayer::getCoverage () const
{
    return layer_coverage;
}

void
SGCloudLayer::setCoverage (Coverage coverage)
{
    if (coverage != layer_coverage) {
        layer_coverage = coverage;
    }
}

const std::string &
SGCloudLayer::getCoverageString( Coverage coverage )
{
	switch( coverage ) {
		case SG_CLOUD_OVERCAST:
			return SG_CLOUD_OVERCAST_STRING;
		case SG_CLOUD_BROKEN:
			return SG_CLOUD_BROKEN_STRING;
		case SG_CLOUD_SCATTERED:
			return SG_CLOUD_SCATTERED_STRING;
		case SG_CLOUD_FEW:
			return SG_CLOUD_FEW_STRING;
		case SG_CLOUD_CIRRUS:
			return SG_CLOUD_CIRRUS_STRING;
		case SG_CLOUD_CLEAR:
		default:
			return SG_CLOUD_CLEAR_STRING;
	}
}

SGCloudLayer::Coverage
SGCloudLayer::getCoverageType( const std::string & coverage )
{
	if( SG_CLOUD_OVERCAST_STRING == coverage ) {
		return SG_CLOUD_OVERCAST;
	} else if( SG_CLOUD_BROKEN_STRING == coverage ) {
		return SG_CLOUD_BROKEN;
	} else if( SG_CLOUD_SCATTERED_STRING == coverage ) {
		return SG_CLOUD_SCATTERED;
	} else if( SG_CLOUD_FEW_STRING == coverage ) {
		return SG_CLOUD_FEW;
	} else if( SG_CLOUD_CIRRUS_STRING == coverage ) {
		return SG_CLOUD_CIRRUS;
	} else {
		return SG_CLOUD_CLEAR;
	}
}

const std::string &
SGCloudLayer::getCoverageString() const
{
    return getCoverageString(layer_coverage);
}

void
SGCloudLayer::setCoverageString( const std::string & coverage )
{
    setCoverage( getCoverageType(coverage) );
}

// reposition the cloud layer at the specified origin and orientation
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (and orients the
// sunrise/set effects
bool SGCloudLayer::reposition( const SGVec3f& p,
                               const SGVec3f& up,
                               double lon,
                               double lat,
                               double alt,
                               double dt )
{
    layer3D->reposition( p, up, lon, lat, dt, layer_asl, speed, direction);
    return true;
}

void SGCloudLayer::set_enable3dClouds(bool enable) {

    if (layer3D->isDefined3D() && enable) {
        cloud_root->setChildValue(layer3D->getNode(), true);
    } else {
        cloud_root->setChildValue(layer3D->getNode(), false);
    }
}
