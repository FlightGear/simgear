// queue.hxx -- Sample Queue encapsulation class
// 
// based on sample.hxx
// 
// Copyright (C) 2010 Erik Hofman <erik@ehofman.com>
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
 * \file audio sample.hxx
 * Provides a sample queue encapsulation
 */

#ifndef _SG_QUEUE_HXX
#define _SG_QUEUE_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <string>
#include <vector>

#include <simgear/compiler.h>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include "sample_openal.hxx"

/**
 * manages everything we need to know for an individual audio sample
 */

class SGSampleQueue : public SGSoundSample {
public:


     /**
      * Empty constructor, can be used to read data to the systems
      * memory and not to the driver.
      * @param freq sample frequentie of the samples
      * @param format OpenAL format id of the data
      */
    SGSampleQueue(int freq, int format = AL_FORMAT_MONO8);

    /**
     * Destructor
     */
    ~SGSampleQueue ();

    /**
     * Schedule this audio sample to stop playing.
     */
    virtual void stop();

    /**
     * Queue new data for this audio sample
     * @param data Pointer to a memory block containg this audio sample data.
     * @param len length of the sample buffer in bytes
     */
    void add( const void* smp_data, size_t len );

    /**
     * Set the source id of this source
     * @param sid OpenAL source-id
     */
    virtual void set_source(unsigned int sid);

    /**
     * Test if the buffer-id of this audio sample may be passed to OpenAL.
     * @return false for sample queue
     */
    inline bool is_valid_buffer() const { return false; }

    inline virtual bool is_queue() const { return true; }

private:
    std::string _refname;	// sample name
    std::vector<unsigned int> _buffers;
    unsigned int _buffer;

    bool _playing;

    std::string random_string();
};


#endif // _SG_QUEUE_HXX


