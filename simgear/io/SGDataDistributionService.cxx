// SGDataDistributionService.cxx -- Data Distribution Service (DDS) I/O routines
//
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <cstring>
#include <algorithm>

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>

#include "SGDataDistributionService.hxx"


SG_DDS_Topic::SG_DDS_Topic()
{
    set_type(sgDDSType);
}

SG_DDS_Topic::~SG_DDS_Topic()
{
    this->close();
}


SG_DDS_Topic::SG_DDS_Topic(const char *topic, const dds_topic_descriptor_t *desc, size_t size)
{
    set_type(sgDDSType);
    setup(topic, desc, size);
}

// Set the paramaters which weren't available at creation time.
void
SG_DDS_Topic::setup(const char *topic, const dds_topic_descriptor_t *desc, size_t size)
{
    topic_name = topic;
    descriptor = desc;
    packet_size = size;
}


// If specified as a server (in direction) create a subscriber.
// If specified as a client (out direction), create a publisher.
// If sepcified as bi-directional create a publisher and a subscriber.
bool
SG_DDS_Topic::open(SGProtocolDir direction)
{
    set_dir(direction);

    if (participant < 0)
    {
        participant = dds_create_participant(FG_DDS_DOMAIN, NULL, NULL);

        if (participant < 0) {
            SG_LOG(SG_IO, SG_ALERT, "dds_create_participant: "
                                     << dds_strretcode(-participant));
            return false;
        }
        shared_participant = false;
    }
    return open(participant, direction);
}

bool
SG_DDS_Topic::open(dds_entity_t p, SGProtocolDir direction)
{
    dds_guid_t g;
    int status = dds_get_guid(p, &g);
    if (status < 0) {
        SG_LOG(SG_IO, SG_ALERT, "dds_get_guid: "
                                << dds_strretcode(-status));
    }
    return open(p, g, direction);
}

bool
SG_DDS_Topic::open(dds_entity_t p, dds_guid_t& g, SGProtocolDir direction)
{
    guid = g;
    participant = p;

    if (topic < 0)
    {
        topic = dds_create_topic(participant, descriptor,
                                 topic_name.c_str(), NULL, NULL);

        if (topic < 0) {
            SG_LOG(SG_IO, SG_ALERT, "dds_create_topic: "
                                     << dds_strretcode(-topic));
            return false;
        }
    }

    if (direction == SG_IO_IN || direction == SG_IO_BI)
    {
        dds_qos_t *qos = dds_create_qos();
        dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_MSECS(1));
        dds_qset_history(qos, DDS_HISTORY_KEEP_LAST, 1);

        reader = dds_create_reader(participant, topic, qos, NULL);
        if (reader < 0) {
           SG_LOG(SG_IO, SG_ALERT, "dds_create_reader: "
                                    << dds_strretcode(-reader));
           return false;
        }

        dds_delete_qos(qos);
    }

    if (direction == SG_IO_OUT || direction == SG_IO_BI)
    {
        dds_return_t rc;

        writer = dds_create_writer(participant, topic, NULL, NULL);
        if (writer < 0) {
            SG_LOG(SG_IO, SG_ALERT, "dds_create_writer: "
                                     << dds_strretcode(-writer));
            return false;
        }

        rc = dds_set_status_mask(writer, DDS_PUBLICATION_MATCHED_STATUS);
        if (rc != DDS_RETCODE_OK) {
            SG_LOG(SG_IO, SG_ALERT, "dds_set_status_mask: "
                                     << dds_strretcode(-rc));
            return false;
        }
    }

    return true;
}


// read data from topic (server)
// read a block of data of specified size
int
SG_DDS_Topic::read(char *buf, int length)
{
    int result;

    if (reader < 0 || length < (int)packet_size) {
        SG_LOG(SG_IO, SG_ALERT, "SG_DDS_Topic: data reader not properly set up.");
        return 0;
    }

    dds_sample_info_t info[1];
    memset(buf, 0, length);
    result = dds_take(reader, (void**)&buf, info, 1, 1);
    if(result < 0)
    {
        errno = -result;
        result = 0;

        SG_LOG(SG_IO, SG_ALERT, "dds_take: " << dds_strretcode(errno));
    }
    else if (result > 0 && !info[0].valid_data) {
        SG_LOG(SG_IO, SG_ALERT, "dds_take: invalid data");
    }

    return result*length;
}


// write data to topic (client)
int
SG_DDS_Topic::write(const char *buf, const int length)
{
    int result;

    if (writer < 0 || length < (int)packet_size) {
        SG_LOG(SG_IO, SG_ALERT, "SG_DDS_Topic: data writer not properly set up.");
        return 0;
    }

    if (!status)
    {
        dds_return_t rc = dds_get_status_changes(writer, &status);
        if (rc != DDS_RETCODE_OK) {
            SG_LOG(SG_IO, SG_ALERT, "dds_get_status_changes: "
                                     << dds_strretcode(-rc));
            return 0;
        }

        if (!(status & DDS_PUBLICATION_MATCHED_STATUS)) {
            SG_LOG(SG_IO, SG_INFO, "SG_DDS_Topic: skipping write: no readers.");
            return length; // no readers yet.
        }
    }

    result = dds_write(writer, buf);
    if(result != DDS_RETCODE_OK)
    {
        errno = -result;
        result = 0;

        SG_LOG(SG_IO, SG_ALERT, "dds_write: " << dds_strretcode(errno));
    }
    else {
       result = length;
    }

    return result;
}


// close the port
bool
SG_DDS_Topic::close()
{
    if (!shared_participant)
        dds_delete(participant);

    shared_participant = true;
    participant = -1;
    topic = -1;
    reader = -1;
    writer = -1;

    return true;
}


// participant
SG_DDS::SG_DDS(dds_domainid_t domain_id, const char *config)
{
    if (domain < 0 && domain_id != DDS_DOMAIN_DEFAULT)
    {
        domain = dds_create_domain(domain_id, config);
        if (domain < 0) {
            SG_LOG(SG_IO, SG_ALERT, "dds_create_domain: "
                                     << dds_strretcode(-domain));
            return;
        }
    }

    if (participant < 0)
    {
        participant = dds_create_participant(domain_id, NULL, NULL);

        if (participant < 0) {
            SG_LOG(SG_IO, SG_ALERT, "dds_create_participant: "
                                     << dds_strretcode(-participant));
            return;
        }

        int status = dds_get_guid(participant, &guid);
        if (status < 0) {
            SG_LOG(SG_IO, SG_ALERT, "dds_get_guid: "
                                    << dds_strretcode(-status));
        }
    }

    waitset = dds_create_waitset(participant);

    // a guard condition is able to wakeup the waitset at destruction.
    guard = dds_create_guardcondition(participant);
    if (guard < 0)
        SG_LOG(SG_IO, SG_ALERT, "dds_create_guardcondition: "
                                 << dds_strretcode(-guard));
}

SG_DDS::~SG_DDS()
{
    this->close();
    dds_delete(participant);
}

bool
SG_DDS::add(SG_DDS_Topic *topic, const SGProtocolDir d)
{
    bool result = topic->open(participant, guid, d);
    if (result)
    {
        if (d == SG_IO_OUT || d == SG_IO_BI)
            writers.push_back(topic);
        else if (d == SG_IO_IN || d == SG_IO_BI)
        {
            readers.push_back(topic);

            dds_entity_t entry = topic->get_reader();
            dds_entity_t rdcond = dds_create_readcondition(entry,
                                                     DDS_NOT_READ_SAMPLE_STATE);
            int status = dds_waitset_attach(waitset, rdcond, entry);
            if (status < 0)
                SG_LOG(SG_IO, SG_ALERT, "ds_waitset_attach: "
                                         << dds_strretcode(-status));
        }
    }

    return result;
}

bool
SG_DDS::close()
{
    // wakeup the waitset.
    if (guard >= 0)
        dds_set_guardcondition(guard, true);

    for (auto it : readers)
        delete it;

    for (auto it : writers)
        delete it;

    readers.clear();
    writers.clear();

    return true;
}

bool
SG_DDS::wait(float dt)
{
    size_t num = readers.size();
    if (!num) return false;

    dds_duration_t timeout = dt * DDS_NSECS_IN_SEC;
    if (dt == std::numeric_limits<float>::max())
        timeout = DDS_INFINITY;

    bool triggered;
    int status = dds_read_guardcondition(guard, &triggered);
    if (triggered) return true;

    dds_attach_t results[num];
    status = dds_waitset_wait(waitset, results, num, timeout);
    if (status < 0) {
        SG_LOG(SG_IO, SG_ALERT, "dds_waitset_wait: "
                                 << dds_strretcode(-status));
        return false;
    }

    return true;
}
