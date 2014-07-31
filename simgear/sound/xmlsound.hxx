///@file
/// Sound class implementation
///
/// Provides a class to manage a single sound event including things like
/// looping, volume and pitch changes.
//
// Started by Erik Hofman, February 2002
//
// Copyright (C) 2002  Erik Hofman - erik@ehofman.com
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _SG_SOUND_HXX
#define _SG_SOUND_HXX 1


#include <vector>
#include <string>
     
#include <simgear/compiler.h>

#include <simgear/props/propsfwd.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

// forward decls
class SGSampleGroup;
class SGSoundSample;
class SGCondition;
class SGPath;

static const double MAX_TRANSIT_TIME = 0.1;	// 100 ms.


/**
 * Class for handling one sound event.
 *
 * This class handles everything for a particular sound event, by
 * scanning an a pre-loaded property tree structure for sound
 * settings, setting up its internal states, and managing sound
 * playback whenever such an event happens.
 */
class SGXmlSound
{

public:

  SGXmlSound();
  virtual ~SGXmlSound();

  /**
   * Initialize the sound event.
   *
   * Prior to initialization of the sound event the program's property root
   * has to be defined, the sound configuration XML tree has to be loaded 
   * and a sound manager class has to be defined.
   *
   * A sound configuration file would look like this:
   * @code{xml}
   *  <fx>
   *   <event_name>
   *    <name/> Define the name of the event. For reference only.
   *    <mode/> Either:
   *              looped: play this sound looped.
   *              in-transit: play looped while the event is happening.
   *              once: play this sound once.
   *    <path/> The relative path to the audio file.
   *    <property/> Take action if this property becomes true.
   *    <condition/> Take action if this condition becomes true.
   *    <delay-sec/> Time after which the sound should be played.
   *    <volume> or <pitch> Define volume or pitch settings.
   *     <property/> Take the value of this property as a reference for the
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
   * @endcode
   *
   * @param root        The root node of the programs property tree.
   * @param child       A pointer to the location of the current event as
   *                    defined in the configuration file.
   * @param sgrp        A pointer to a pre-initialized sample group class.
   * @param avionics    A pointer to the pre-initialized avionics sample group.
   * @param path        The path where the audio files remain.
   */
  virtual void init( SGPropertyNode *root,
                     SGPropertyNode *child,
                     SGSampleGroup *sgrp,
                     SGSampleGroup *avionics,
                     const SGPath& path );

  /**
   * Check whether an event has happened and if action has to be taken.
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

  // SGXmlSound properties
  typedef struct {
        SGPropertyNode_ptr prop;
        double (*fn)(double);
        double *intern;
        double factor;
        double offset;
        double min;
        double max;
        bool subtract;
  } _snd_prop;

private:

  SGSampleGroup * _sgrp;
  SGSharedPtr<SGSoundSample> _sample;

  SGSharedPtr<SGCondition> _condition;
  SGPropertyNode_ptr _property;

  bool _active;
  std::string _name;
  int _mode;
  double _prev_value;
  double _dt_play;
  double _dt_stop;
  double _delay;        // time after which the sound should be started (default: 0)
  double _stopping;     // time after the sound should have stopped.
                        // This is useful for lost packets in in-transit mode.
  bool _initialized;

  std::vector<_snd_prop> _volume;
  std::vector<_snd_prop> _pitch;
};

#endif // _SG_SOUND_HXX
