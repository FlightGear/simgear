// soundmgr_openal_prviate.hxx -- Implementation details of the soung manager
//
// Sound manager initially written by David Findlay
// <david_j_findlay@yahoo.com.au> 2001
//
// C++-ified by Curtis Olson, started March 2001.
// Modified for the new SoundSystem by Erik Hofman, October 2009
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
// Copyright (C) 2009 Erik Hofman <erik@ehofman.com>
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


#ifndef _SG_SOUNDMGR_OPENAL_PRIVATE_HXX
#define _SG_SOUNDMGR_OPENAL_PRIVATE_HXX 1

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <string>
#include <vector>
#include <cmath>
#include <map>

#if defined(__APPLE__)
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#elif defined(OPENALSDK)
# include <al.h>
# include <alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGMisc.hxx>

class SGSampleGroup;

struct refUint {
    unsigned int refctr;
    ALuint id;

    refUint() { refctr = 0; id = (ALuint)-1; };
    refUint(ALuint i) { refctr = 1; id = i; };
    ~refUint() {};
};

using buffer_map = std::map < std::string, refUint >;
using sample_group_map = std::map < std::string, SGSharedPtr<SGSampleGroup> >;

inline bool isNaN(float *v) {
   return (SGMisc<float>::isNaN(v[0]) || SGMisc<float>::isNaN(v[1]) || SGMisc<float>::isNaN(v[2]));
}

#endif // _SG_SOUNDMGR_OPENAL_PRIVATE_HXX


