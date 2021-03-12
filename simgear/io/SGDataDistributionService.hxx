/**
 * \file SGDataDistributionService.hxx
 * Data Distribution Service (DDS) I/O routines
 */

// Written by Erik Hofman, started March 2021.
//
// Copyright (C) 2021 by Erik Hofman <erik@ehofman.com>
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


#ifndef _SG_DDS_HXX
#define _SG_DDS_HXX

#include <string>
#include <dds/dds.h>

#include <simgear/compiler.h>
#include <simgear/math/sg_types.hxx>
#include <simgear/io/iochannel.hxx>


/**
 * A socket I/O class based on SGIOChannel.
 */
class SG_DDS : public SGIOChannel {
private:

    std::string topic_name;
    const dds_topic_descriptor_t *descriptor = nullptr;
    size_t packet_size = 0;

    uint32_t status = 0;

    dds_entity_t participant = -1;
    dds_entity_t topic = -1;
    dds_entity_t writer = -1;
    dds_entity_t reader = -1;

public:

    /** Create an instance of SG_DDS. */
    SG_DDS();

    /** Destructor */
    ~SG_DDS();

    // Set the paramaters which weren't available at creation time.
    void setup(const char* topic, const dds_topic_descriptor_t *desc, size_t size);

    // If specified as a server start a publishing participant.
    // If specified as a client start a subscribing participant.
    bool open( const SGProtocolDir d );

    // read data from the topic.
    int read( char *buf, int length );

    // write data to the topic.
    int write( const char *buf, const int length );

    // close the participant.
    bool close();
};


#endif // _SG_DDS_HXX
