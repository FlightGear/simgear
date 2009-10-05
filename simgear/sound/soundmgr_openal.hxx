// soundmgr.hxx -- Sound effect management class
//
// Sound manager initially written by David Findlay
// <david_j_findlay@yahoo.com.au> 2001
//
// C++-ified by Curtis Olson, started March 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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

/**
 * \file soundmgr.hxx
 * Provides a sound manager class to keep track of
 * multiple sounds and manage playing them with different effects and
 * timings.
 */

#ifndef _SG_SOUNDMGR_OPENAL_HXX
#define _SG_SOUNDMGR_OPENAL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <string>
#include <vector>
#include <map>

#if defined(__APPLE__)
# define AL_ILLEGAL_ENUM AL_INVALID_ENUM
# define AL_ILLEGAL_COMMAND AL_INVALID_OPERATION
# include <OpenAL/al.h>
# include <OpenAL/alut.h>
#else
# include <AL/al.h>
# include <AL/alut.h>
#endif

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/SGMathFwd.hxx>

#include "sample_group.hxx"
#include "sample_openal.hxx"

using std::map;
using std::string;

typedef map < string, SGSharedPtr<SGSampleGroup> > sample_group_map;
typedef sample_group_map::iterator sample_group_map_iterator;
typedef sample_group_map::const_iterator const_sample_group_map_iterator;


/**
 * Manage a collection of SGSampleGroup instances
 */
class SGSoundMgr : public SGSubsystem
{
public:

    SGSoundMgr();
    ~SGSoundMgr();

    void init();
    void bind();
    void unbind();
    void update(double dt);
    void update_late(double dt);
    
    void suspend();
    void resume();
    void stop();

    inline void reinit() { stop(); init(); }

    /**
     * is audio working?
     */
    inline bool is_working() const { return _working; }

    /**
     * add a sample group, return true if successful
     */
    bool add( SGSampleGroup *sgrp, const string& refname );

    /** 
     * remove a sample group, return true if successful
     */
    bool remove( const string& refname );

    /**
     * return true of the specified sound exists in the sound manager system
     */
    bool exists( const string& refname );

    /**
     * return a pointer to the SGSampleGroup if the specified sound
     * exists in the sound manager system, otherwise return NULL
     */
    SGSampleGroup *find( const string& refname, bool create = false );

    /**
     * set the position of the listener (in opengl coordinates)
     */
    inline void set_position( SGVec3d pos ) {
        _listener_pos = pos;
        _changed = true;
    }

    inline double *get_position() {
       return _listener_pos.data();
    }

    /**
     * set the velocity of the listener (in opengl coordinates)
     */
    inline void set_velocity( SGVec3f vel ) {
        _listener_vel = vel;
        _changed = true;
    }

    /**
     * set the orientation of the listener (in opengl coordinates)
     */
    void set_orientation( SGQuatd ori );

    enum {
        NO_SOURCE = (unsigned int)-1,
        NO_BUFFER = (unsigned int)-1
    };

    void set_volume( float v );
    inline float get_volume() { return _volume; }

    /**
     * get a new OpenAL source id
     * returns NO_SOURCE is no source is available
     */
    unsigned int request_source();

    /**
     * give back an OpenAL source id for further use.
     */
    void release_source( unsigned int source );

    static bool load(string &samplepath, void **data, int *format,
                                         unsigned int*size, int *freq );


private:
    static int _alut_init;

    bool _working;
    bool _changed;
    float _volume;

    ALCdevice *_device;
    ALCcontext *_context;

    // Position of the listener.
    SGVec3d _listener_pos;

    // Velocity of the listener.
    SGVec3f _listener_vel;

    // Orientation of the listener. 
    // first 3 elements are "at" vector, second 3 are "up" vector
    ALfloat _listener_ori[6];

    sample_group_map _sample_groups;

    vector<ALuint> _free_sources;
    vector<ALuint> _sources_in_use;

    char *_devname;

    bool testForALError(string s);
    bool testForALCError(string s);
    bool testForALUTError(string s);
    bool testForError(void *p, string s);
    void update_sample_config( SGSampleGroup *sound );
};


#endif // _SG_SOUNDMGR_OPENAL_HXX


