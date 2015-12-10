// sound.cxx -- Sound class implementation
//
// Started by Erik Hofman, February 2002
// (Reuses some code from  fg_fx.cxx created by David Megginson)
//
// Copyright (C) 2002  Curtis L. Olson - http://www.flightgear.org/~curt
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

#include "xmlsound.hxx"


#include <simgear/compiler.h>

#include <string.h>
#include <stdio.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include "sample_group.hxx"
#include "sample_openal.hxx"

using std::string;

// static double _snd_lin(double v)   { return v; }
static double _snd_inv(double v)   { return (v == 0) ? 1e99 : 1/v; }
static double _snd_abs(double v)   { return (v >= 0) ? v : -v; }
static double _snd_sqrt(double v)  { return sqrt(fabs(v)); }
static double _snd_log10(double v) { return log10(fabs(v)+1e-9); }
static double _snd_log(double v)   { return log(fabs(v)+1e-9); }
// static double _snd_sqr(double v)   { return v*v; }
// static double _snd_pow3(double v)  { return v*v*v; }

static const struct {
	const char *name;
	double (*fn)(double);
} __sound_fn[] = {
	{"inv", _snd_inv},
	{"abs", _snd_abs},
	{"sqrt", _snd_sqrt},
	{"log", _snd_log10},
	{"ln", _snd_log},
	{"", NULL}
};

SGXmlSound::SGXmlSound()
  : _sample(NULL),
    _active(false),
    _name(""),
    _mode(SGXmlSound::ONCE),
    _prev_value(0),
    _dt_play(0.0),
    _dt_stop(0.0),
    _delay(0.0),
    _stopping(0.0),
    _initialized(false)
{
}

SGXmlSound::~SGXmlSound()
{
    if (_sample)
        _sample->stop();

    _volume.clear();
    _pitch.clear();
}

void
SGXmlSound::init( SGPropertyNode *root,
                  SGPropertyNode *node,
                  SGSampleGroup *sgrp,
                  SGSampleGroup *avionics,
                  const SGPath& path )
{

   //
   // set global sound properties
   //

   _name = node->getStringValue("name", "");
   SG_LOG(SG_SOUND, SG_DEBUG, "Loading sound information for: " << _name );

   string mode_str = node->getStringValue("mode", "");
   if ( mode_str == "looped" ) {
       _mode = SGXmlSound::LOOPED;

   } else if ( mode_str == "in-transit" ) {
       _mode = SGXmlSound::IN_TRANSIT;

   } else {
      _mode = SGXmlSound::ONCE;
   }

   bool is_avionics = false;
   string type_str = node->getStringValue("type", "fx");
   if ( type_str == "avionics" )
      is_avionics = true;

   string propval = node->getStringValue("property", "");
   if (propval != "")
      _property = root->getNode(propval, true);

   SGPropertyNode *condition = node->getChild("condition");
   if (condition != NULL)
      _condition = sgReadCondition(root, condition);

   if (!_property && !_condition)
      SG_LOG(SG_SOUND, SG_WARN,
             "  Neither a condition nor a property specified");

   _delay = node->getDoubleValue("delay-sec", 0.0);

   //
   // set volume properties
   //
   unsigned int i;
   float v = 0.0;
   std::vector<SGPropertyNode_ptr> kids = node->getChildren("volume");
   for (i = 0; (i < kids.size()) && (i < SGXmlSound::MAXPROP); i++) {
      _snd_prop volume = {NULL, NULL, NULL, 1.0, 0.0, 0.0, 0.0, false};

      propval = kids[i]->getStringValue("property", "");
      if ( propval != "" )
         volume.prop = root->getNode(propval, true);

      string intern_str = kids[i]->getStringValue("internal", "");
      if (intern_str == "dt_play")
         volume.intern = &_dt_play;
      else if (intern_str == "dt_stop")
         volume.intern = &_dt_stop;

      if ((volume.factor = kids[i]->getDoubleValue("factor", 1.0)) != 0.0)
         if (volume.factor < 0.0) {
            volume.factor = -volume.factor;
            volume.subtract = true;
         }

      string type_str = kids[i]->getStringValue("type", "");
      if ( type_str != "" ) {

         for (int j=0; __sound_fn[j].fn; j++)
           if ( type_str == __sound_fn[j].name ) {
               volume.fn = __sound_fn[j].fn;
               break;
            }

         if (!volume.fn)
            SG_LOG(SG_SOUND,SG_INFO,
                   "  Unknown volume type, default to 'lin'");
      }

      volume.offset = kids[i]->getDoubleValue("offset", 0.0);

      if ((volume.min = kids[i]->getDoubleValue("min", 0.0)) < 0.0)
         SG_LOG( SG_SOUND, SG_WARN,
          "Volume minimum value below 0. Forced to 0.");

      volume.max = kids[i]->getDoubleValue("max", 0.0);
      if (volume.max && (volume.max < volume.min) )
         SG_LOG(SG_SOUND,SG_ALERT,
                "  Volume maximum below minimum. Neglected.");

      _volume.push_back(volume);
      v += volume.offset;

   }

   // rule of thumb: make reference distance a 100th of the maximum distance.
   float reference_dist = node->getDoubleValue("reference-dist", 60.0);
   float max_dist = node->getDoubleValue("max-dist", 6000.0);

   //
   // set pitch properties
   //
   float p = 0.0;
   kids = node->getChildren("pitch");
   for (i = 0; (i < kids.size()) && (i < SGXmlSound::MAXPROP); i++) {
      _snd_prop pitch = {NULL, NULL, NULL, 1.0, 1.0, 0.0, 0.0, false};

      propval = kids[i]->getStringValue("property", "");
      if (propval != "")
         pitch.prop = root->getNode(propval, true);

      string intern_str = kids[i]->getStringValue("internal", "");
      if (intern_str == "dt_play")
         pitch.intern = &_dt_play;
      else if (intern_str == "dt_stop")
         pitch.intern = &_dt_stop;

      if ((pitch.factor = kids[i]->getDoubleValue("factor", 1.0)) != 0.0)
         if (pitch.factor < 0.0) {
            pitch.factor = -pitch.factor;
            pitch.subtract = true;
         }

      string type_str = kids[i]->getStringValue("type", "");
      if ( type_str != "" ) {

         for (int j=0; __sound_fn[j].fn; j++) 
            if ( type_str == __sound_fn[j].name ) {
               pitch.fn = __sound_fn[j].fn;
               break;
            }

         if (!pitch.fn)
            SG_LOG(SG_SOUND,SG_INFO,
                   "  Unknown pitch type, default to 'lin'");
      }
     
      pitch.offset = kids[i]->getDoubleValue("offset", 1.0);

      if ((pitch.min = kids[i]->getDoubleValue("min", 0.0)) < 0.0)
         SG_LOG(SG_SOUND,SG_WARN,
                "  Pitch minimum value below 0. Forced to 0.");

      pitch.max = kids[i]->getDoubleValue("max", 0.0);
      if (pitch.max && (pitch.max < pitch.min) )
         SG_LOG(SG_SOUND,SG_ALERT,
                "  Pitch maximum below minimum. Neglected");

      _pitch.push_back(pitch);
      p += pitch.offset;
   }

   //
   // Relative position
   //
   SGVec3f offset_pos = SGVec3f::zeros();
   SGPropertyNode_ptr prop = node->getChild("position");
   SGPropertyNode_ptr pos_prop[3];
   if ( prop != NULL ) {
       offset_pos[0] = -prop->getDoubleValue("x", 0.0);
       offset_pos[1] = -prop->getDoubleValue("y", 0.0);
       offset_pos[2] = -prop->getDoubleValue("z", 0.0);

       pos_prop[0] = prop->getChild("x");
       if (pos_prop[0]) pos_prop[0] = pos_prop[0]->getNode("property");
       if (pos_prop[0]) {
           pos_prop[0] = root->getNode(pos_prop[0]->getStringValue(), true);
       }
       pos_prop[1] = prop->getChild("y");
       if (pos_prop[1]) pos_prop[1] = pos_prop[1]->getNode("property");
       if (pos_prop[1]) {
           pos_prop[1] = root->getNode(pos_prop[1]->getStringValue(), true);
       }
       pos_prop[2] = prop->getChild("z");
       if (pos_prop[2]) pos_prop[2] = pos_prop[1]->getNode("property");
       if (pos_prop[2]) {
           pos_prop[2] = root->getNode(pos_prop[2]->getStringValue(), true);
       }
   }

   //
   // Orientation
   //
   SGVec3f dir = SGVec3f::zeros();
   float inner = 360.0;
   float outer = 360.0;
   float outer_gain = 0.0;
   prop = node->getChild("orientation");
   if ( prop != NULL ) {
      dir = SGVec3f(-prop->getFloatValue("x", 0.0),
                    -prop->getFloatValue("y", 0.0),
                    -prop->getFloatValue("z", 0.0));
      inner = prop->getFloatValue("inner-angle", 360.0);
      outer = prop->getFloatValue("outer-angle", 360.0);
      outer_gain = prop->getFloatValue("outer-gain", 0.0);
   }

   //
   // Initialize the sample
   //
   if ((is_avionics)&&(avionics)) {
      _sgrp = avionics;
   } else {
      _sgrp = sgrp;
   }
   string soundFileStr = node->getStringValue("path", "");
   _sample = new SGSoundSample(soundFileStr.c_str(), path);
   if (!_sample->file_path().exists()) {
      throw sg_io_exception("XML sound: couldn't find file: '" + soundFileStr + "'");
      return;
   }
   
   _sample->set_relative_position( offset_pos );
   _sample->set_position_properties( pos_prop );
   _sample->set_direction( dir );
   _sample->set_audio_cone( inner, outer, outer_gain );
   _sample->set_reference_dist( reference_dist );
   _sample->set_max_dist( max_dist );
   _sample->set_volume( v );
   _sample->set_pitch( p );
   _sgrp->add( _sample, _name );
}

void
SGXmlSound::update (double dt)
{
   double curr_value = 0.0;

   //
   // If the state changes to false, stop playing.
   //
   if (_property)
       curr_value = _property->getDoubleValue();

   if (!_initialized)
   {
       // update initial value before detecting changes
       _prev_value  = curr_value;
       _initialized = true;
   }

   // If a condition is defined, test whether it is FALSE,
   // else
   //   if a property is defined then test if it's value is FALSE
   //      or if the mode is IN_TRANSIT then
   //            test whether the current value matches the previous value.
   if (							// Lisp, anyone?
       (_condition && !_condition->test()) ||
       (!_condition && _property &&
        (
         !curr_value ||
         ( (_mode == SGXmlSound::IN_TRANSIT) && (curr_value == _prev_value) )
         )
        )
       )
   {
       if ((_mode != SGXmlSound::IN_TRANSIT) || (_stopping > MAX_TRANSIT_TIME))
       {
           if (_sample->is_playing()) {
               SG_LOG(SG_SOUND, SG_DEBUG, "Stopping audio after " << _dt_play
                      << " sec: " << _name );

               _sample->stop();
           }

           _active = false;
           _dt_stop += dt;
           _dt_play = 0.0;
       } else {
           _stopping += dt;
       }

       return;
   }

   //
   // mode is ONCE and the sound is still playing?
   //
   if (_active && (_mode == SGXmlSound::ONCE)) {

      if (!_sample->is_playing()) {
         _dt_stop += dt;
         _dt_play = 0.0;
      } else {
         _dt_play += dt;
      }

   } else {

      //
      // Update the playing time, cache the current value and
      // clear the delay timer.
      //
      _dt_play += dt;
      _prev_value = curr_value;
      _stopping = 0.0;
   }

   if (_dt_play < _delay)
      return;

   //
   // Update the volume
   //
   int i;
   int max = _volume.size();
   double volume = 1.0;
   double volume_offset = 0.0;

   for(i = 0; i < max; i++) {
      double v = 1.0;

      if (_volume[i].prop)
         v = _volume[i].prop->getDoubleValue();

      else if (_volume[i].intern)
         v = *_volume[i].intern;

      if (_volume[i].fn)
         v = _volume[i].fn(v);

      v *= _volume[i].factor;

      if (_volume[i].max && (v > _volume[i].max))
         v = _volume[i].max;

      else if (v < _volume[i].min)
         v = _volume[i].min;

      if (_volume[i].subtract)				// Hack!
         volume = _volume[i].offset - v;

      else {
         volume_offset += _volume[i].offset;
         volume *= v;
      }
   }

   //
   // Update the pitch
   //
   max = _pitch.size();
   double pitch = 1.0;
   double pitch_offset = 0.0;

   for(i = 0; i < max; i++) {
      double p = 1.0;

      if (_pitch[i].prop)
         p = _pitch[i].prop->getDoubleValue();

      else if (_pitch[i].intern)
         p = *_pitch[i].intern;

      if (_pitch[i].fn)
         p = _pitch[i].fn(p);

      p *= _pitch[i].factor;

      if (_pitch[i].max && (p > _pitch[i].max))
         p = _pitch[i].max;

      else if (p < _pitch[i].min)
         p = _pitch[i].min;

      if (_pitch[i].subtract)				// Hack!
         pitch = _pitch[i].offset - p;

      else {
         pitch_offset += _pitch[i].offset;
         pitch *= p;
      }
   }

   //
   // Change sample state
   //

   double vol = volume_offset + volume;
   if (vol > 1.0) {
      SG_LOG(SG_SOUND, SG_DEBUG, "Sound volume too large for '"
              << _name << "':  " << vol << "  ->  clipping to 1.0");
      vol = 1.0;
   }
   _sample->set_volume(vol);
   _sample->set_pitch( pitch_offset + pitch );


   //
   // Do we need to start playing the sample?
   //
   if (!_active) {

      if (_mode == SGXmlSound::ONCE)
         _sample->play(false);

      else
         _sample->play(true);

      SG_LOG(SG_SOUND, SG_DEBUG, "Playing audio after " << _dt_stop
                                   << " sec: " << _name);
      SG_LOG(SG_SOUND, SG_DEBUG,
                         "Playing " << ((_mode == ONCE) ? "once" : "looped"));

      _active = true;
      _dt_stop = 0.0;
   }
}
