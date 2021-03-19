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


#ifndef _SG_DDS_Topic_HXX
#define _SG_DDS_Topic_HXX

#include <string>
#include <vector>

#include <dds/dds.h>

#include <simgear/compiler.h>
#include <simgear/math/sg_types.hxx>
#include <simgear/io/iochannel.hxx>

#define FG_DDS_DOMAIN           DDS_DOMAIN_DEFAULT

// Data Distribution Service (DDS) class based on SGIOChannel.
class SG_DDS_Topic : public SGIOChannel {
private:

    std::string topic_name;
    const dds_topic_descriptor_t *descriptor = nullptr;
    size_t packet_size = 0;

    uint32_t status = 0;

    bool shared_participant = true;
    dds_entity_t participant = -1;
    dds_entity_t topic = -1;
    dds_entity_t entry = -1;

public:

    /** Create an instance of SG_DDS_Topic. */
    SG_DDS_Topic();

    SG_DDS_Topic(const char* topic, const dds_topic_descriptor_t *desc, size_t size);

    /** Destructor */
    ~SG_DDS_Topic();

    // Set the paramaters which weren't available at creation time and use
    // a custom topic name.
    void setup(const char* topic, const dds_topic_descriptor_t *desc, size_t size);

    // If specified as a server start a publishing participant.
    // If specified as a client start a subscribing participant.
    bool open(const SGProtocolDir d);

    // If specified as a server start publishing to a participant.
    // If specified as a client start subscribing to a participant.
    bool open(dds_entity_t p, const SGProtocolDir d);

    // read data from the topic.
    int read(char *buf, int length);

    // write data to the topic.
    int write(const char *buf, const int length);

    // close the participant.
    bool close();

    dds_entity_t get() { return entry; }
};

// a class to manage multiple DDS topics
class SG_DDS {
private:
    dds_entity_t participant = -1;
    dds_domainid_t domain = FG_DDS_DOMAIN;

    dds_entity_t waitset = -1;

    std::vector<SG_DDS_Topic*> readers;
    std::vector<SG_DDS_Topic*> writers;

public:
    SG_DDS(dds_domainid_t d = FG_DDS_DOMAIN);
    ~SG_DDS();

    bool add(SG_DDS_Topic *topic, const SGProtocolDir d);
    bool close();

    bool wait(float dt = 0.0f);
};


#endif // _SG_DDS_Topic_HXX
