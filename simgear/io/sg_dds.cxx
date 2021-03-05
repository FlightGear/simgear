// sg_dds.cxx -- Data Distribution Service (DDS) I/O routines
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

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>

#include "sg_dds.hxx"


#define FG_DDS_DOMAIN		DDS_DOMAIN_DEFAULT

SG_DDS::SG_DDS()
{
    set_type( sgDDSType );
}


SG_DDS::~SG_DDS()
{
    this->close();
}

// Set the paramaters which weren't available at creation time.
void
SG_DDS::setup( const char *topic, const dds_topic_descriptor_t *desc, const size_t size)
{
    topic_name = topic;
    descriptor = desc;
    packet_size = size;
}

// If specified as a server (in direction) create a subscriber.
// If specified as a client (out direction), create a publisher.
bool
SG_DDS::open( SGProtocolDir direction )
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
    }

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
        dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_SECS(10));
    
        reader = dds_create_reader(participant, topic, qos, NULL);
        if (reader < 0) {
           SG_LOG(SG_IO, SG_ALERT, "dds_create_reader: "
                                    << dds_strretcode(-reader));
           return false;
        }

        dds_delete_qos(qos);
        sample[0] = dds_alloc(packet_size);
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

        uint32_t status = 0;
        while(!(status & DDS_PUBLICATION_MATCHED_STATUS))
        {
            rc = dds_get_status_changes(writer, &status);
            if (rc != DDS_RETCODE_OK) {
                SG_LOG(SG_IO, SG_ALERT, "dds_get_status_changes: "
                                        << dds_strretcode(-rc));
            }

            /* Polling sleep. */
            dds_sleepfor(DDS_MSECS(20));
        }
    }

    return true;
}


// read data from topic (server)
// read a block of data of specified size
int
SG_DDS::read( char *buf, int length )
{
    int result;
    
    if (reader < 0 || length < packet_size) {
        return 0;
    }

    result = dds_take(reader, sample, info, 1, 1);
    if(result < 0)
    {
      SG_LOG(SG_IO, SG_ALERT, "dds_take: " << dds_strretcode(errno));
      errno = -result;
      result = 0;
    }

    return result*length;
}


// write data to topic (client)
int
SG_DDS::write( const char *buf, const int length )
{
    int result;

    if (writer < 0 || length < packet_size) {
        return 0;
    }

    result = dds_write(writer, buf);
    if(result != DDS_RETCODE_OK) 
    {
        SG_LOG(SG_IO, SG_ALERT, "dds_write: " << dds_strretcode(errno));
        errno = -result;
        result = 0;
    }
    else {
       result = length;
    }

    return result;
}


// close the port
bool
SG_DDS::close()
{
    dds_sample_free(sample[0], descriptor, DDS_FREE_ALL);
    dds_delete(participant);

    reader = -1;
    writer = -1;
    topic = -1;
    participant = -1;

    return true;
}

