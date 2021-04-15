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

#include <limits>
#include <string>
#include <vector>

#include <dds/dds.h>

#include <simgear/compiler.h>
#include <simgear/math/sg_types.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/structure/intern.hxx>

#define FG_DDS_DOMAIN           DDS_DOMAIN_DEFAULT

// Data Distribution Service (DDS) class based on SGIOChannel.
class SG_DDS_Topic : public SGIOChannel {
private:

    std::string topic_name;
    const dds_topic_descriptor_t *descriptor = nullptr;

    char *buffer = nullptr;
    size_t packet_size = 0;

    uint32_t status = 0;

    bool shared_participant = true;
    dds_entity_t participant = -1;
    dds_entity_t topic = -1;
    dds_entity_t reader = -1;
    dds_entity_t writer = -1;

    dds_guid_t guid;

public:

    /** Create an instance of SG_DDS_Topic. */
    SG_DDS_Topic();

    SG_DDS_Topic(const char* topic, const dds_topic_descriptor_t *desc, size_t size);

    // Store the pointer to the buffer to be able to call read() without
    // any paramaters. Make sure the buffer is always available and of
    // sufficient size to store a type T otherwise a segmentation fault
    // will occur.
    template<typename T>
    SG_DDS_Topic(T& buf, const dds_topic_descriptor_t *desc) : SG_DDS_Topic()
    {
        buffer = reinterpret_cast<char*>(&buf);
        setup<T>(desc);
    }

    /** Destructor */
    virtual ~SG_DDS_Topic();

    // Set the paramaters which weren't available at creation time and use
    // a custom topic name.
    void setup(const char* topic, const dds_topic_descriptor_t *desc, size_t size);

    // Store the pointer to the buffer to be able to call read() without
    // any paramaters. Make sure the buffer is always available and of
    // sufficient size to store a type T otherwise a segmentation fault
    // will occur.
    template<typename T>
    void setup(T& buf, const dds_topic_descriptor_t *desc) {
        buffer = reinterpret_cast<char*>(&buf);
        std::string type = simgear::getTypeName<T>();
        setup(type.c_str(), desc, sizeof(T));
    }

    template<typename T>
    void setup(const dds_topic_descriptor_t *desc) {
        std::string type = simgear::getTypeName<T>();
        setup(type.c_str(), desc, sizeof(T));
    }

    // If specified as a server start a publishing participant.
    // If specified as a client start a subscribing participant.
    // Create a new partipant and unique identifier and establish a connection.
    bool open(const SGProtocolDir d);

    // Establish the connection using a shared participant.
    // Create the unique identifier (GUID) from the shared participant.
    bool open(dds_entity_t p, const SGProtocolDir d);

    // Establish the connection using a shared participant and shared GUID.
    bool open(dds_entity_t p, dds_guid_t& g, const SGProtocolDir d);

    // read data from the topic.
    int read(char *buf, int length);

    // Calling read without any parameters requires to use de constructor
    // with a buffer type.
    int read() {
        return buffer ? read(buffer, packet_size) : 0;
    }

    template<typename T>
    bool read(T& sample) {
        return (read(reinterpret_cast<char*>(&sample), sizeof(T)) == sizeof(T)) ? true : false;
    }

    // write data to the topic.
    int write(const char *buf, const int length);

    template<typename T>
    bool write(const T& sample) {
        return (write(reinterpret_cast<char*>(&sample), sizeof(T)) == sizeof(T)) ? true : false;
    }

    // Calling write without any parameters requires to use de constructor
    // with a buffer type.
    int write() {
        return buffer ? write(buffer, packet_size) : 0;
    }

    // close the participant.
    bool close();

    dds_entity_t get_reader() { return reader; }
    dds_entity_t get_writer() { return writer; }
    dds_entity_t get(const SGProtocolDir d = SG_IO_IN) {
        return (d == SG_IO_OUT) ? writer : reader;
    }

    const dds_guid_t& get_guid() { return guid; }
};

// a class to manage multiple DDS topics
class SG_DDS {
private:
    dds_entity_t domain = -1;
    dds_entity_t participant = -1;
    dds_domainid_t domain_id = FG_DDS_DOMAIN;

    dds_entity_t guard = -1;
    dds_entity_t waitset = -1;

    std::vector<SG_DDS_Topic*> readers;
    std::vector<SG_DDS_Topic*> writers;

    dds_guid_t guid;

public:
    SG_DDS(dds_domainid_t d = FG_DDS_DOMAIN, const char *c = "");

    SG_DDS(dds_domainid_t d, std::string& c) : SG_DDS(d, c.c_str()) {};

    ~SG_DDS();

    bool add(SG_DDS_Topic *topic, const SGProtocolDir d);
    bool close();

    const std::vector<SG_DDS_Topic*>& get_readers() { return readers; }
    const std::vector<SG_DDS_Topic*>& get_writers() { return writers; }

    bool wait(float dt = std::numeric_limits<float>::max());

    const dds_guid_t& get_guid() { return guid; }
};


#endif // _SG_DDS_Topic_HXX
