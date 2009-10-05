// soundmgr.hxx -- Sound effect management class
//
// Sampel Group handler initially written by Erik Hofman
//
// Copyright (C) 2009  Erik Hofman - <erik@ehofman.com>
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
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

/**
 * \file sample_group.hxx
 * sample groups contain all sounds related to one specific object and
 * have to be added to the sound manager, otherwise they won't get processed.
 */

#ifndef _SG_SAMPLE_GROUP_OPENAL_HXX
#define _SG_SAMPLE_GROUP_OPENAL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#if defined(__APPLE__)
# include <OpenAL/al.h>
#else
# include <AL/al.h>
#endif

#include <string>
#include <map>

#include <simgear/compiler.h>
#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/exception.hxx>

#include "sample_openal.hxx"

using std::map;
using std::string;

typedef map < string, SGSharedPtr<SGSoundSample> > sample_map;
typedef sample_map::iterator sample_map_iterator;
typedef sample_map::const_iterator const_sample_map_iterator;

class SGSoundMgr;

class SGSampleGroup : public SGReferenced
{
public:
    SGSampleGroup ();
    SGSampleGroup ( SGSoundMgr *smgr, const string &refname );
    ~SGSampleGroup ();

    virtual void update (double dt);

    /**
     * add a sound effect, return true if successful
     */
    bool add( SGSoundSample *sound, const string& refname );

    /**
     * remove a sound effect, return true if successful
     */
    bool remove( const string& refname );

    /**
     * return true of the specified sound exists in the sound manager system
     */
    bool exists( const string& refname );

    /**
     * return a pointer to the SGSoundSample if the specified sound
     * exists in the sound manager system, otherwise return NULL
     */
    SGSoundSample *find( const string& refname );

    /**
     * request to stop playing all associated samples until further notice
     */
    void suspend();

    /**
     * request to resume playing all associated samples
     */
    void resume();


    /**
     * request to start playing the associated samples
     */
    bool play( const string& refname, bool looping );
    
    /**
     * tell the scheduler to play the indexed sample in a continuous
     * loop
     */
    inline bool play_looped( const string& refname ) {
        return play( refname, true );
    }

    /**
     * tell the scheduler to play the indexed sample once
     */
    inline bool play_once( const string& refname ) {
        return play( refname, false );
    }

    /**
     * return true of the specified sound is currently being played
     */
    bool is_playing( const string& refname );

    /**
     * request to stop playing the associated samples
     */
    bool stop( const string& refname );

    /**
     * set overall volume for the application.
     * @param must be between 0.0 and 1.0
     */
    void set_volume( float vol );

    /**
     * set the positions of all managed sound sources
     */
    void set_position( SGVec3d pos );

    /**
     * set the velocities of all managed sound sources
     */
    void set_velocity( SGVec3f vel );

    /**
     * set the orientation of all managed sound sources
     */
    void set_orientation( SGVec3f ori );

    /**
     * load the data of the sound sample
     */
    void load_file(SGSoundSample *sound);

protected:
    SGSoundMgr *_smgr;
    bool _active;

private:
    bool _changed;
    bool _position_changed;

    float _volume;

    SGVec3d _position;
    SGVec3f _orientation;

    sample_map _samples;

    bool testForALError(string s);
    bool testForError(void *p, string s);

    string _refname;

    void update_sample_config( SGSoundSample *sound );
};

#endif // _SG_SAMPLE_GROUP_OPENAL_HXX

