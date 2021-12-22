// SGText.cxx - Manage text in the scene graph
// Copyright (C) 2009 Torsten Dreyer Torsten (_at_) t3r *dot* de
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

#include <simgear_config.h>

#include <cstdio>

#include "SGText.hxx"


#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osgText/Text>
#include <osgText/Font>

#include <simgear/misc/ResourceManager.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/debug/ErrorReportingCallback.hxx>

using std::string;

class SGText::UpdateCallback : public osg::NodeCallback {
public:
  UpdateCallback( osgText::Text * aText, SGConstPropertyNode_ptr aProperty, double aScale, double aOffset, bool aTruncate, bool aNumeric, const char * aFormat ) :
    text( aText ),
    property( aProperty ),
    scale( aScale ),
    offset( aOffset ),
    truncate( aTruncate ),
    numeric( aNumeric ),
    format( simgear::strutils::sanitizePrintfFormat( aFormat ) )
  {
    if( format.empty() ) {
      if( numeric ) format = "%f";
      else format = "%s";
    }
  }

private:
  virtual void operator()(osg::Node * node, osg::NodeVisitor *nv );
  osgText::Text * text;
  SGConstPropertyNode_ptr property;
  double scale;
  double offset;
  bool truncate;
  bool numeric;
  string format;
};

void SGText::UpdateCallback::operator()(osg::Node * node, osg::NodeVisitor *nv ) 
{
  // FIXME:
  // hopefully the users never specifies bad formats here
  // this should better be something more robust
  char buf[256];
  if( numeric ) {
    double d = property->getDoubleValue() * scale + offset;
    if (truncate)  d = (d < 0) ? -floor(-d) : floor(d);
    snprintf( buf, sizeof(buf)-1, format.c_str(), d );
  } else {
    snprintf( buf, sizeof(buf)-1, format.c_str(), property->getStringValue() );
  }
  if( text->getText().createUTF8EncodedString().compare( buf )  ) {
    // be lazy and set the text only if the property has changed.
    // update() computes the glyph representation which looks 
    // more expensive than a the above string compare.
    text->setText( buf, osgText::String::ENCODING_UTF8 );
    text->update();
  }
  traverse( node, nv );
}

osg::Node * SGText::appendText(const SGPropertyNode* configNode, 
  SGPropertyNode* modelRoot, const osgDB::Options* options)
{
  SGConstPropertyNode_ptr p;

  osgText::Text * text = new osgText::Text();
  osg::Geode * g = new osg::Geode;
  g->addDrawable( text );

  const std::string requestedFont = configNode->getStringValue("font","Helvetica");
  const SGPath fontPath = simgear::ResourceManager::instance()->findPath("Fonts/" + requestedFont);
  if ( !fontPath.isNull() ) {
    text->setFont( fontPath.utf8Str() );
  } else {
    simgear::reportFailure(simgear::LoadFailure::NotFound,
                          simgear::ErrorCode::LoadingTexture,
                                   "SGText: couldn;t find font:" + requestedFont);
  }

  text->setCharacterSize(configNode->getDoubleValue("character-size", 1.0 ), 
                         configNode->getDoubleValue("character-aspect-ratio", 1.0 ));
  
  if( (p = configNode->getNode( "font-resolution" )) != NULL )
    text->setFontResolution( p->getIntValue( "width", 32 ), p->getIntValue( "height", 32 ) );

  if( (p = configNode->getNode( "kerning" )) != NULL ) {
    string kerning = p->getStringValue();
    if( kerning.compare( "default" ) == 0 ) {
      text->setKerningType( osgText::KERNING_DEFAULT );
    } else if( kerning.compare( "unfitted" ) == 0 ) {
      text->setKerningType( osgText::KERNING_UNFITTED );
    } else if( kerning.compare( "none" ) == 0 ) {
      text->setKerningType( osgText::KERNING_NONE );
    } else {
      SG_LOG(SG_GENERAL, SG_ALERT, "ignoring unknown kerning'" << kerning << "'." );
    }
  }

  if( ( p = configNode->getNode( "axis-alignment" )) != NULL ) {
    string axisAlignment = p->getStringValue();
    if( axisAlignment.compare( "xy-plane" ) == 0 ) {
      text->setAxisAlignment( osgText::Text::XY_PLANE );
    } else if( axisAlignment.compare( "reversed-xy-plane" ) == 0 ) {
      text->setAxisAlignment( osgText::Text::REVERSED_XY_PLANE );
    } else if( axisAlignment.compare( "xz-plane" ) == 0 ) {
      text->setAxisAlignment( osgText::Text::XZ_PLANE );
    } else if( axisAlignment.compare( "reversed-xz-plane" ) == 0 ) {
      text->setAxisAlignment( osgText::Text::REVERSED_XZ_PLANE );
    } else if( axisAlignment.compare( "yz-plane" ) == 0 ) {
      text->setAxisAlignment( osgText::Text::YZ_PLANE );
    } else if( axisAlignment.compare( "reversed-yz-plane" ) == 0 ) {
      text->setAxisAlignment( osgText::Text::REVERSED_YZ_PLANE );
    } else if( axisAlignment.compare( "screen" ) == 0 ) {
      text->setAxisAlignment( osgText::Text::SCREEN );
    } else {
      SG_LOG(SG_GENERAL, SG_ALERT, "ignoring unknown axis-alignment'" << axisAlignment << "'." );
    }
  }

  unsigned drawMode = osgText::Text::TEXT;
  if( (p = configNode->getNode( "draw-text" )) != NULL && p->getBoolValue() == false )
    drawMode &= ~ osgText::Text::TEXT;

  if( (p = configNode->getNode( "draw-alignment" )) != NULL && p->getBoolValue() == true )
    drawMode |= osgText::Text::ALIGNMENT;

  if( (p = configNode->getNode( "draw-boundingbox" )) != NULL && p->getBoolValue() == true )
    drawMode |= osgText::Text::BOUNDINGBOX;

  text->setDrawMode( drawMode );

  if( (p = configNode->getNode( "alignment" )) != NULL ) {
    string alignment = p->getStringValue();
    if( alignment.compare( "left-top" ) == 0 ) {
      text->setAlignment( osgText::Text::LEFT_TOP );
    } else if( alignment.compare( "left-center" ) == 0 ) {
      text->setAlignment( osgText::Text::LEFT_CENTER );
    } else if( alignment.compare( "left-bottom" ) == 0 ) {
      text->setAlignment( osgText::Text::LEFT_BOTTOM );
    } else if( alignment.compare( "center-top" ) == 0 ) {
      text->setAlignment( osgText::Text::CENTER_TOP );
    } else if( alignment.compare( "center-center" ) == 0 ) {
      text->setAlignment( osgText::Text::CENTER_CENTER );
    } else if( alignment.compare( "center-bottom" ) == 0 ) {
      text->setAlignment( osgText::Text::CENTER_BOTTOM );
    } else if( alignment.compare( "right-top" ) == 0 ) {
      text->setAlignment( osgText::Text::RIGHT_TOP );
    } else if( alignment.compare( "right-center" ) == 0 ) {
      text->setAlignment( osgText::Text::RIGHT_CENTER );
    } else if( alignment.compare( "right-bottom" ) == 0 ) {
      text->setAlignment( osgText::Text::RIGHT_BOTTOM );
    } else if( alignment.compare( "left-baseline" ) == 0 ) {
      text->setAlignment( osgText::Text::LEFT_BASE_LINE );
    } else if( alignment.compare( "center-baseline" ) == 0 ) {
      text->setAlignment( osgText::Text::CENTER_BASE_LINE );
    } else if( alignment.compare( "right-baseline" ) == 0 ) {
      text->setAlignment( osgText::Text::RIGHT_BASE_LINE );
    } else if( alignment.compare( "baseline" ) == 0 ) {
      text->setAlignment( osgText::Text::BASE_LINE );
    } else {
      SG_LOG(SG_GENERAL, SG_ALERT, "ignoring unknown text-alignment '" << alignment <<"'." );
    }
  }

  if( (p = configNode->getNode( "layout" )) != NULL ) {
    string layout = p->getStringValue();
    if( layout.compare( "left-to-right" ) == 0 ) {
      text->setLayout( osgText::Text::LEFT_TO_RIGHT );
    } else if( layout.compare( "right-to-left" ) == 0 ) {
      text->setLayout( osgText::Text::RIGHT_TO_LEFT );
    } else if( layout.compare( "vertical" ) == 0 ) {
      text->setLayout( osgText::Text::VERTICAL );
    } else {
      SG_LOG(SG_GENERAL, SG_ALERT, "ignoring unknown layout '" << layout <<"'." );
    }
  }
  
  if( (p = configNode->getNode( "max-width" )) != NULL )
    text->setMaximumWidth( p->getDoubleValue() );

  if( (p = configNode->getNode( "max-height" )) != NULL )
    text->setMaximumHeight( p->getDoubleValue() );

  // FIXME: Should we support <chunks><chunk/><chunk/></chunks> here?
  string type = configNode->getStringValue( "type", "literal" );
  if( type == "literal" ) {
    text->setText( configNode->getStringValue( "text", "" ) );
  } else {
    SGConstPropertyNode_ptr property = modelRoot->getNode( configNode->getStringValue( "property", "foo" ), true );
    const char * format = configNode->getStringValue( "format", "" );
    double scale = configNode->getDoubleValue( "scale", 1.0 );
    double offset = configNode->getDoubleValue( "offset", 0.0 );
    bool   truncate = configNode->getBoolValue( "truncate", false );

    if( (p = configNode->getNode( "property")) != NULL ) {
      p = modelRoot->getNode( p->getStringValue(), true );
      UpdateCallback * uc = new UpdateCallback( text, property, scale, offset, truncate, type == "number-value", format );
      g->setUpdateCallback( uc );
    }
  }

  osg::Node * reply = NULL;
  if( (p = configNode->getNode( "offsets")) == NULL ) {
    reply = g;
  } else {
    // Set up the alignment node ("stolen" from animation.cxx)
    // XXX Order of rotations is probably not correct.
    osg::MatrixTransform *align = new osg::MatrixTransform;
    osg::Matrix res_matrix;
    res_matrix.makeRotate(
        p->getFloatValue("pitch-deg", 0.0)*SG_DEGREES_TO_RADIANS,
        osg::Vec3(0, 1, 0),
        p->getFloatValue("roll-deg", 0.0)*SG_DEGREES_TO_RADIANS,
        osg::Vec3(1, 0, 0),
        p->getFloatValue("heading-deg", 0.0)*SG_DEGREES_TO_RADIANS,
        osg::Vec3(0, 0, 1));

    osg::Matrix tmat;
    tmat.makeTranslate(configNode->getFloatValue("offsets/x-m", 0.0),
                       configNode->getFloatValue("offsets/y-m", 0.0),
                       configNode->getFloatValue("offsets/z-m", 0.0));

    align->setMatrix(res_matrix * tmat);
    align->addChild( g );
    reply = align;
  }

  if( (p = configNode->getNode( "name" )) != NULL )
    reply->setName(p->getStringValue());
  else
    reply->setName("text");
  return reply;
}
