// sound.hxx -- Sound class implementation
//
// Started by Erik Hofman, February 2002
//
// Copyright (C) 2002  Erik Hofman - erik@ehofman.com
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

/** 
 * \file sound.hxx 
 * Provides a class to manage a single sound event including things
 * like looping, volume and pitch changes.
 */

#ifndef _SG_SOUND_HXX
#define _SG_SOUND_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>
#include <simgear/props/condition.hxx>

#include "soundmgr.hxx"

static const double MAX_TRANSIT_TIME = 0.1;	// 100 ms.


/**
 * Class for handling one sound event.
 *
 * This class handles everything for a particular sound event, by
 * scanning an a pre-loaded property tree structure for sound
 * settings, setting up its internal states, and managing sound
 * playback whenever such an event happens.
 */
class SGSound
{

public:

  SGSound();
  virtual ~SGSound();

  /**
   * Initialize the sound event.
   *
   * Prior to initialization of the sound event the propgrams property root
   * has to be defined, the sound configuration XML tree has to be loaded 
   * and a sound manager class has to be defined.
   *
   * A sound configuration file would look like this:
   *  <fx>
   *   <event_name>
   *    <name/> Define the name of the event. For refference only.
   *    <mode/> Either:
   *              looped: play this sound looped.
   *              in-transit: play looped while the event is happening.
   *              once: play this sound once.
   *    <path/> The relative path to the audio file.
   *    <property/> Take action if this property becomes true.
   *    <condition/> Take action if this condition becomes true.
   *    <volume> or <pitch> Define volume or pitch settings.
   *     <property/> Take the value of this property as a refference for the
   *                 result.
   *     <internal/> Either:
   *                   dt_start: the time elapsed since this sound is playing.
   *                   dt_stop: the time elapsed since this sound has stopped.
   *     <offset/> Add this value to the result.
   *     <factor/> Multiply the result by this factor.
   *     <min/> Make sure the value is never less than this value.
   *     <max/> Make sure the value is never larger than this value.
   *    </volume> or </pitch>
   *   </event_name>
   *
   *   <event_name>
   *   </event_name>
   *  </fx>
   *
   * @param root The root node of the programs property tree.
   * @param child A pointer to the location of the current event as defined
   * in the configuration file.
   * @param sndmgr A pointer to a pre-initialized sound manager class.
   * @param path The path where the audio files remain.
   */
  virtual void init (SGPropertyNode *, SGPropertyNode *, SGSoundMgr *,
                     const string &);

  /**
   * Check wheter an event has happened and if action has to be taken.
   */
  virtual void update (double dt);

  /**
   * Stop taking action on the pre-defined events.
   */
  void stop();

protected:

  enum { MAXPROP=5 };
  enum { ONCE=0, LOOPED, IN_TRANSIT };
  enum { LEVEL=0, INVERTED, FLIPFLOP };

  // SGSound properties
  typedef struct {
        SGPropertyNode * prop;
        double (*fn)(double);
        double *intern;
        double factor;
        double offset;
        double min;
        double max;
        bool subtract;
  } _snd_prop;

private:

  SGSoundMgr * _mgr;
  SGSimpleSound * _sample;

  SGCondition * _condition;
  SGPropertyNode * _property;

  bool _active;
  string _name;
  int _mode;
  double _prev_value;
  double _dt_play;
  double _dt_stop;
  double _stopping;	// time after the sound should have stopped.
			// This is usefull for lost packets in in-trasit mode.

  vector<_snd_prop> _volume;
  vector<_snd_prop> _pitch;

};

#endif // _SG_SOUND_HXX
