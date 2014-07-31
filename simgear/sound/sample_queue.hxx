///@file
/// Provides a sample queue encapsulation
//
// based on sample.hxx
// 
// Copyright (C) 2010 Erik Hofman <erik@ehofman.com>
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

#ifndef _SG_QUEUE_HXX
#define _SG_QUEUE_HXX 1

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
     * Queue new data for this audio sample.
     *
     * @param data  Pointer to a memory block containg this audio sample data.
     * @param len   Length of the sample buffer in bytes.
     */
    void add( const void* data, size_t len );

    /**
     * Set the source id of this source.
     *
     * @param sid OpenAL source-id
     */
    virtual void set_source(unsigned int sid);

    /**
     * Test if the buffer-id of this audio sample may be passed to OpenAL.
     *
     * @return false for sample queue
     */
    inline bool is_valid_buffer() const { return false; }

    inline virtual bool is_queue() const { return true; }

private:
    std::string _refname;	// sample name
    std::vector<unsigned int> _buffers;

    bool _playing;

    std::string random_string();
};


#endif // _SG_QUEUE_HXX


