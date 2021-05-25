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

static osg::ref_ptr<osg::StateSet> layer_states[SGCloudLayer::SG_MAX_CLOUD_COVERAGES];
static osg::ref_ptr<osg::StateSet> layer_states2[SGCloudLayer::SG_MAX_CLOUD_COVERAGES];
static bool state_initialized = false;

const std::string SGCloudLayer::SG_CLOUD_OVERCAST_STRING = "overcast";
const std::string SGCloudLayer::SG_CLOUD_BROKEN_STRING = "broken";
const std::string SGCloudLayer::SG_CLOUD_SCATTERED_STRING = "scattered";
const std::string SGCloudLayer::SG_CLOUD_FEW_STRING = "few";
const std::string SGCloudLayer::SG_CLOUD_CIRRUS_STRING = "cirrus";
const std::string SGCloudLayer::SG_CLOUD_CLEAR_STRING = "clear";

// make an StateSet for a cloud layer given the named texture
static osg::StateSet*
SGMakeState(const SGPath &path, const char* colorTexture,
            const char* normalTexture)
{
    osg::StateSet *stateSet = new osg::StateSet;

    osg::ref_ptr<SGReaderWriterOptions> options;
    options = SGReaderWriterOptions::fromPath(path);
    stateSet->setTextureAttribute(0, SGLoadTexture2D(colorTexture,
                                                     options.get()));
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
    StateAttributeFactory* attribFactory = StateAttributeFactory::instance();
    stateSet->setAttributeAndModes(attribFactory->getSmoothShadeModel());
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setAttributeAndModes(attribFactory->getStandardAlphaFunc());
    stateSet->setAttributeAndModes(attribFactory->getStandardBlendFunc());
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);

    return stateSet;
}

// Constructor
SGCloudLayer::SGCloudLayer( const SGPath &tex_path ) :
    cloud_root(new osg::Switch),
    layer_root(new osg::Switch),
    group_top(new osg::Group),
    group_bottom(new osg::Group),
    layer_transform(new osg::MatrixTransform),
    cloud_alpha(1.0),
    texture_path(tex_path),
    layer_span(0.0),
    layer_asl(0.0),
    layer_thickness(0.0),
    layer_transition(0.0),
    layer_visibility(25.0),
    layer_coverage(SG_CLOUD_CLEAR),
    scale(4000.0),
    speed(0.0),
    direction(0.0),
    last_course(0.0),
    max_alpha(1.0)
{
    // XXX
    // Render bottoms before the rest of transparent objects (rendered
    // in bin 10), tops after. The negative numbers on the bottoms
    // RenderBins and the positive numbers on the tops enforce this
    // order.
  cloud_root->addChild(layer_root.get(), true);
  layer_root->addChild(group_bottom.get());
  layer_root->addChild(group_top.get());
  osg::StateSet *rootSet = layer_root->getOrCreateStateSet();
  rootSet->setRenderBinDetails(CLOUDS_BIN, "DepthSortedBin");
  rootSet->setTextureAttribute(0, new osg::TexMat);
  rootSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
  // Combiner for fog color and cloud alpha
  osg::TexEnvCombine* combine0 = new osg::TexEnvCombine;
  osg::TexEnvCombine* combine1 = new osg::TexEnvCombine;
  combine0->setCombine_RGB(osg::TexEnvCombine::MODULATE);
  combine0->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
  combine0->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
  combine0->setSource1_RGB(osg::TexEnvCombine::TEXTURE0);
  combine0->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
  combine0->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
  combine0->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
  combine0->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
  combine0->setSource1_Alpha(osg::TexEnvCombine::TEXTURE0);
  combine0->setOperand1_Alpha(osg::TexEnvCombine::SRC_ALPHA);

  combine1->setCombine_RGB(osg::TexEnvCombine::MODULATE);
  combine1->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
  combine1->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
  combine1->setSource1_RGB(osg::TexEnvCombine::CONSTANT);
  combine1->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
  combine1->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
  combine1->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
  combine1->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
  combine1->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
  combine1->setOperand1_Alpha(osg::TexEnvCombine::SRC_ALPHA);
  combine1->setDataVariance(osg::Object::DYNAMIC);
  rootSet->setTextureAttributeAndModes(0, combine0);
  rootSet->setTextureAttributeAndModes(1, combine1);
  rootSet->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON);
  rootSet->setTextureAttributeAndModes(1, StateAttributeFactory::instance()
                                       ->getWhiteTexture(),
                                       osg::StateAttribute::ON);
  rootSet->setDataVariance(osg::Object::DYNAMIC);

  // Ensure repeatability of the random seed within 10 minutes,
  // to keep multi-computer systems in sync.
  sg_srandom_time_10();
  base = osg::Vec2(sg_random(), sg_random());
  group_top->addChild(layer_transform.get());
  group_bottom->addChild(layer_transform.get());

  layer3D = new SGCloudField();
  cloud_root->addChild(layer3D->getNode(), false);

  rebuild();
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
        rebuild();
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
        rebuild();
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

void
SGCloudLayer::setTextureOffset(const osg::Vec2& offset)
{
    osg::StateAttribute* attr = layer_root->getStateSet()
        ->getTextureAttribute(0, osg::StateAttribute::TEXMAT);
    osg::TexMat* texMat = dynamic_cast<osg::TexMat*>(attr);
    if (!texMat)
        return;
    texMat->setMatrix(osg::Matrix::translate(offset[0], offset[1], 0.0));
}

// colors for debugging the cloud layers
#ifdef CLOUD_DEBUG
Vec3 cloudColors[] = {Vec3(1.0f, 1.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f),
                      Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)};
#else
Vec3 cloudColors[] = {Vec3(1.0f, 1.0f, 1.0f), Vec3(1.0f, 1.0f, 1.0f),
                      Vec3(1.0f, 1.0f, 1.0f), Vec3(1.0f, 1.0f, 1.0f)};
#endif

// build the cloud object
void
SGCloudLayer::rebuild()
{
    // Initialize states and sizes if necessary.
    if ( !state_initialized ) {
        state_initialized = true;

        SG_LOG(SG_ASTRO, SG_INFO, "initializing cloud layers");

        osg::StateSet* state;
        state = SGMakeState(texture_path, "overcast.png", "overcast_n.png");
        layer_states[SG_CLOUD_OVERCAST] = state;
        state = SGMakeState(texture_path, "overcast_top.png", "overcast_top_n.png");
        layer_states2[SG_CLOUD_OVERCAST] = state;

        state = SGMakeState(texture_path, "broken.png", "broken_n.png");
        layer_states[SG_CLOUD_BROKEN] = state;
        layer_states2[SG_CLOUD_BROKEN] = state;

        state = SGMakeState(texture_path, "scattered.png", "scattered_n.png");
        layer_states[SG_CLOUD_SCATTERED] = state;
        layer_states2[SG_CLOUD_SCATTERED] = state;

        state = SGMakeState(texture_path, "few.png", "few_n.png");
        layer_states[SG_CLOUD_FEW] = state;
        layer_states2[SG_CLOUD_FEW] = state;

        state = SGMakeState(texture_path, "cirrus.png", "cirrus_n.png");
        layer_states[SG_CLOUD_CIRRUS] = state;
        layer_states2[SG_CLOUD_CIRRUS] = state;

        layer_states[SG_CLOUD_CLEAR] = 0;
        layer_states2[SG_CLOUD_CLEAR] = 0;
#if 1
        // experimental optimization that may not make any difference
        // at all :/
        osg::CopyOp copyOp;
        for (int i = 0; i < SG_MAX_CLOUD_COVERAGES; ++i) {
            StateAttributeFactory *saf = StateAttributeFactory::instance();
            if (layer_states[i].valid()) {
                if (layer_states[i] == layer_states2[i])
                    layer_states2[i] = static_cast<osg::StateSet*>(layer_states[i]->clone(copyOp));
                layer_states[i]->setAttribute(saf ->getCullFaceFront());
                layer_states2[i]->setAttribute(saf ->getCullFaceBack());
            }
        }
#endif
    }

    scale = 4000.0;

    setTextureOffset(base);
    // build the cloud layer
    const float layer_scale = layer_span / scale;
    const float mpi = SG_PI/4;

    // caclculate the difference between a flat-earth model and
    // a round earth model given the span and altutude ASL of
    // the cloud layer. This is the difference in altitude between
    // the top of the inverted bowl and the edge of the bowl.
    // const float alt_diff = layer_asl * 0.8;
    const float layer_to_core = (SG_EARTH_RAD * 1000 + layer_asl);
    const float layer_angle = 0.5*layer_span / layer_to_core; // The angle is half the span
    const float border_to_core = layer_to_core * cos(layer_angle);
    const float alt_diff = layer_to_core - border_to_core;

    for (int i = 0; i < 4; i++) {
      if ( layer[i] != NULL ) {
        layer_transform->removeChild(layer[i].get()); // automatic delete
      }

      vl[i] = new osg::Vec3Array;
      cl[i] = new osg::Vec4Array;
      tl[i] = new osg::Vec2Array;


      osg::Vec3 vertex(layer_span*(i-2)/2, -layer_span,
                       alt_diff * (sin(i*mpi) - 2));
      osg::Vec2 tc(layer_scale * i/4, 0.0f);
      osg::Vec4 color(cloudColors[0], (i == 0) ? 0.0f : 0.15f);

      cl[i]->push_back(color);
      vl[i]->push_back(vertex);
      tl[i]->push_back(tc);

      for (int j = 0; j < 4; j++) {
        vertex = osg::Vec3(layer_span*(i-1)/2, layer_span*(j-2)/2,
                           alt_diff * (sin((i+1)*mpi) + sin(j*mpi) - 2));
        tc = osg::Vec2(layer_scale * (i+1)/4, layer_scale * j/4);
        color = osg::Vec4(cloudColors[0],
                          ( (j == 0) || (i == 3)) ?
                          ( (j == 0) && (i == 3)) ? 0.0f : 0.15f : 1.0f );

        cl[i]->push_back(color);
        vl[i]->push_back(vertex);
        tl[i]->push_back(tc);

        vertex = osg::Vec3(layer_span*(i-2)/2, layer_span*(j-1)/2,
                           alt_diff * (sin(i*mpi) + sin((j+1)*mpi) - 2) );
        tc = osg::Vec2(layer_scale * i/4, layer_scale * (j+1)/4 );
        color = osg::Vec4(cloudColors[0],
                          ((j == 3) || (i == 0)) ?
                          ((j == 3) && (i == 0)) ? 0.0f : 0.15f : 1.0f );
        cl[i]->push_back(color);
        vl[i]->push_back(vertex);
        tl[i]->push_back(tc);
      }

      vertex = osg::Vec3(layer_span*(i-1)/2, layer_span,
                         alt_diff * (sin((i+1)*mpi) - 2));

      tc = osg::Vec2(layer_scale * (i+1)/4, layer_scale);

      color = osg::Vec4(cloudColors[0], (i == 3) ? 0.0f : 0.15f );

      cl[i]->push_back( color );
      vl[i]->push_back( vertex );
      tl[i]->push_back( tc );

      osg::Geometry* geometry = new osg::Geometry;
      geometry->setUseDisplayList(false);
      geometry->setVertexArray(vl[i].get());
      geometry->setNormalBinding(osg::Geometry::BIND_OFF);
      geometry->setColorArray(cl[i].get(), osg::Array::BIND_PER_VERTEX);
      geometry->setTexCoordArray(0, tl[i].get(), osg::Array::BIND_PER_VERTEX);
      geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vl[i]->size()));
      layer[i] = new osg::Geode;

      std::stringstream sstr;
      sstr << "Cloud Layer (" << i << ")";
      geometry->setName(sstr.str());
      layer[i]->setName(sstr.str());
      layer[i]->addDrawable(geometry);
      layer_transform->addChild(layer[i].get());
    }

    //OSGFIXME: true
    if ( layer_states[layer_coverage].valid() ) {
      osg::CopyOp copyOp;    // shallow copy
      // render bin will be set in reposition
      osg::StateSet* stateSet = static_cast<osg::StateSet*>(layer_states2[layer_coverage]->clone(copyOp));
      stateSet->setDataVariance(osg::Object::DYNAMIC);
      group_top->setStateSet(stateSet);
      stateSet = static_cast<osg::StateSet*>(layer_states[layer_coverage]->clone(copyOp));
      stateSet->setDataVariance(osg::Object::DYNAMIC);
      group_bottom->setStateSet(stateSet);
    }
}

// repaint the cloud layer colors
bool SGCloudLayer::repaint( const SGVec3f& fog_color ) {
    osg::Vec4f combineColor(toOsg(fog_color), cloud_alpha);
    osg::StateAttribute* textureAtt = layer_root->getStateSet()->getTextureAttribute(1, osg::StateAttribute::TEXENV);
    osg::TexEnvCombine* combiner = dynamic_cast<osg::TexEnvCombine*>(textureAtt);

    if (combiner == nullptr)
        return false;
    combiner->setConstantColor(combineColor);

    // Set the fog color for the 3D clouds too.
    //cloud3dfog->setColor(combineColor);
    return true;
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

    if (getCoverage() != SGCloudLayer::SG_CLOUD_CLEAR)
    {
        // combine p and asl (meters) to get translation offset
        osg::Vec3 asl_offset(toOsg(up));
        asl_offset.normalize();
        if ( alt <= layer_asl ) {
            asl_offset *= layer_asl;
        } else {
            asl_offset *= layer_asl + layer_thickness;
        }

        // cout << "asl_offset = " << asl_offset[0] << "," << asl_offset[1]
        //      << "," << asl_offset[2] << endl;
        asl_offset += toOsg(p);
        // cout << "  asl_offset = " << asl_offset[0] << "," << asl_offset[1]
        //      << "," << asl_offset[2] << endl;

        osg::Matrix T, LON, LAT;
        // Translate to zero elevation
        // Point3D zero_elev = current_view.get_cur_zero_elev();
        T.makeTranslate( asl_offset );

        // printf("  Translated to %.2f %.2f %.2f\n",
        //        zero_elev.x, zero_elev.y, zero_elev.z );

        // Rotate to proper orientation
        // printf("  lon = %.2f  lat = %.2f\n",
        //        lon * SGD_RADIANS_TO_DEGREES,
        //        lat * SGD_RADIANS_TO_DEGREES);
        LON.makeRotate(lon, osg::Vec3(0, 0, 1));

        // xglRotatef( 90.0 - f->get_Latitude() * SGD_RADIANS_TO_DEGREES,
        //             0.0, 1.0, 0.0 );
        LAT.makeRotate(90.0 * SGD_DEGREES_TO_RADIANS - lat, osg::Vec3(0, 1, 0));

        layer_transform->setMatrix( LAT*LON*T );

        // The layers need to be drawn in order because they are
        // translucent, but OSG transparency sorting doesn't work because
        // the cloud polys are huge. However, the ordering is simple: the
        // bottom polys should be drawn from high altitude to low, and the
        // top polygons from low to high. The altitude can be used
        // directly to order the polygons!
        group_bottom->getStateSet()->setRenderBinDetails(-(int)layer_asl,
                                                         "RenderBin");
        group_top->getStateSet()->setRenderBinDetails((int)layer_asl,
                                                      "RenderBin");
        if ( alt <= layer_asl ) {
          layer_root->setSingleChildOn(0);
        } else if ( alt >= layer_asl + layer_thickness ) {
          layer_root->setSingleChildOn(1);
        } else {
          layer_root->setAllChildrenOff();
        }


        // now calculate update texture coordinates
        SGGeod pos = SGGeod::fromRad(lon, lat);
        if ( last_pos == SGGeod() ) {
            last_pos = pos;
        }

        double sp_dist = speed*dt;


        if ( lon != last_pos.getLongitudeRad() || lat != last_pos.getLatitudeRad() || sp_dist != 0 ) {
            double course = SGGeodesy::courseDeg(last_pos, pos) * SG_DEGREES_TO_RADIANS,
                dist = SGGeodesy::distanceM(last_pos, pos);

            // if start and dest are too close together,
            // calc_gc_course_dist() can return a course of "nan".  If
            // this happens, lets just use the last known good course.
            // This is a hack, and it would probably be better to make
            // calc_gc_course_dist() more robust.
            if ( isNaN(course) ) {
                course = last_course;
            } else {
                last_course = course;
            }

            // calculate cloud movement due to external forces
            double ax = 0.0, ay = 0.0, bx = 0.0, by = 0.0;

            if (dist > 0.0) {
                ax = -cos(course) * dist;
                ay = sin(course) * dist;
            }

            if (sp_dist > 0) {
                bx = cos((180.0-direction) * SGD_DEGREES_TO_RADIANS) * sp_dist;
                by = sin((180.0-direction) * SGD_DEGREES_TO_RADIANS) * sp_dist;
            }


            double xoff = (ax + bx) / (2 * scale);
            double yoff = (ay + by) / (2 * scale);


    //        const float layer_scale = layer_span / scale;

            // cout << "xoff = " << xoff << ", yoff = " << yoff << endl;
            base[0] += xoff;

            // the while loops can lead to *long* pauses if base[0] comes
            // with a bogus value.
            // while ( base[0] > 1.0 ) { base[0] -= 1.0; }
            // while ( base[0] < 0.0 ) { base[0] += 1.0; }
            if ( base[0] > -10.0 && base[0] < 10.0 ) {
                base[0] -= (int)base[0];
            } else {
                SG_LOG(SG_ASTRO, SG_DEBUG,
                    "Error: base = " << base[0] << "," << base[1] <<
                    " course = " << course << " dist = " << dist );
                base[0] = 0.0;
            }

            base[1] += yoff;
            // the while loops can lead to *long* pauses if base[0] comes
            // with a bogus value.
            // while ( base[1] > 1.0 ) { base[1] -= 1.0; }
            // while ( base[1] < 0.0 ) { base[1] += 1.0; }
            if ( base[1] > -10.0 && base[1] < 10.0 ) {
                base[1] -= (int)base[1];
            } else {
                SG_LOG(SG_ASTRO, SG_DEBUG,
                        "Error: base = " << base[0] << "," << base[1] <<
                        " course = " << course << " dist = " << dist );
                base[1] = 0.0;
            }

            // cout << "base = " << base[0] << "," << base[1] << endl;

            setTextureOffset(base);
            last_pos = pos;
        }
    }

    layer3D->reposition( p, up, lon, lat, dt, layer_asl, speed, direction);
    return true;
}

void SGCloudLayer::set_enable3dClouds(bool enable) {

    if (layer3D->isDefined3D() && enable) {
        cloud_root->setChildValue(layer3D->getNode(), true);
        cloud_root->setChildValue(layer_root.get(),   false);
    } else {
        cloud_root->setChildValue(layer3D->getNode(), false);
        cloud_root->setChildValue(layer_root.get(),   true);
    }
}
