/**
 * \file DNSClient.cxx - simple DNS resolver client engine for SimGear
 */

// Written by James Turner
//
// Copyright (C) 2016 Torsten Dreyer - torsten (at) t3r (dot) de
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <simgear_config.h>

#include <algorithm>

#include "DNSClient.hxx"
#include <ares.h>
#include <ctime>

#include <simgear/debug/logstream.hxx>
#include <string.h>

namespace simgear {

namespace DNS {

class Client::ClientPrivate {
public:
    ClientPrivate(const std::string& nameserver)
    {
        if( instanceCounter++ == 0 ) {
            /* Initialize library */
            ares_library_init(ARES_LIB_INIT_ALL);
            if (!ares_threadsafety()) {
                throw std::runtime_error("c-ares not compiled with thread support");
            }
        }
        struct ares_options options;
        int optmask = 0;

        memset(&options, 0, sizeof(options));
        optmask |= ARES_OPT_EVENT_THREAD;
        options.evsys = ARES_EVSYS_DEFAULT;

        /* Initialize channel to run queries, a single channel can accept unlimited queries */
        if (ares_init_options(&channel, &options, optmask) != ARES_SUCCESS) {
            throw std::runtime_error("c-ares initialization issue");
        }
    }

    ~ClientPrivate() {
        /* Wait until no more requests are left to be processed */
        ares_queue_wait_empty(channel, -1);

        /* Cleanup */
        ares_destroy(channel);

        if( --instanceCounter == 0 ) {
            ares_library_cleanup();
        }
    }

    void query( Request * request ) {
        ares_query(
            this->channel,
            request->getDn().c_str(),
            ARES_CLASS_IN,
            request->getType(),
            callback,
            request);
    }

    static void callback(void* arg, int status, int timeouts, unsigned char* abuf, int alen) {
        Request* r = static_cast<Request*>(arg);

        if (status != ARES_SUCCESS) {
            std::cerr << "DNS query failed: " << ares_strerror(status) << std::endl;
        } else {
            // this is not a very oo´ish approach
            switch( r->getType() ) {
                case ARES_REC_TYPE_NAPTR:
                    parse_NAPTR(abuf, alen, static_cast< NAPTRRequest*>(arg));
                    break;
                case ARES_REC_TYPE_SRV:
                    parse_SRV(abuf, alen, static_cast<SRVRequest*>(arg));
                    break;
                case ARES_REC_TYPE_TXT:
                    parse_TXT(abuf, alen, static_cast<TXTRequest*>(arg));
                    break;
                default:
                    std::cerr << "unhandled DNS callback for type: " << r->getType() << " ignored." << std::endl;
                    break;
            }
        }
        r->setComplete(true);
    }

    static void parse_NAPTR( unsigned char * abuf, int alen, NAPTRRequest * record ) {

        struct ares_naptr_reply* naptr_out;
        int result = ares_parse_naptr_reply(abuf, alen, &naptr_out);
        if (result != ARES_SUCCESS) {
            std::cerr << "Failed to parse NAPTR reply: " << ares_strerror(result) << std::endl;
            record->setComplete(true);
            return;
        }

        for (struct ares_naptr_reply* naptr = naptr_out; naptr != nullptr; naptr = naptr->next) {
            std::string naptrService(reinterpret_cast<char*> (naptr->service));
            if (!record->qservice.empty() && record->qservice != naptrService)
                continue;

            std::string naptrFlags(reinterpret_cast<char*>(naptr->flags));
            //TODO: case ignore and result flags may have more than one flag
            if (!record->qflags.empty() && record->qflags != naptrFlags)
                continue;

            NAPTRRequest::NAPTR_ptr n(new NAPTRRequest::NAPTR);
            record->entries.push_back(n);
            n->order = naptr->order;
            n->preference = naptr->preference;
            n->flags = naptrFlags;
            n->service = naptrService;
            n->regexp = reinterpret_cast<char*>(naptr->regexp);
            n->replacement = naptr->replacement;
        }
        std::sort(record->entries.begin(), record->entries.end(), [](const NAPTRRequest::NAPTR_ptr a, const NAPTRRequest::NAPTR_ptr b) {
            if (a->order > b->order) return false;
            if (a->order < b->order) return true;
            return a->preference < b->preference;
        });
    }

    static void parse_SRV(unsigned char* abuf, int alen, SRVRequest* record)
    {
        struct ares_srv_reply* srv_out;
        int result = ares_parse_srv_reply(abuf, alen, &srv_out);
        if (result != ARES_SUCCESS) {
            std::cerr << "Failed to parse SRV reply: " << ares_strerror(result) << std::endl;
            record->setComplete(true);
            return;
        }

        for (struct ares_srv_reply* srv = srv_out; srv != nullptr; srv = srv->next) {
            SRVRequest::SRV_ptr n(new SRVRequest::SRV);
            record->entries.push_back(n);
            n->port = srv->port;
            n->target = srv->host;
            n->priority = srv->priority;
            n->weight = srv->weight;
        }
        std::sort(record->entries.begin(), record->entries.end(), [](const SRVRequest::SRV_ptr a, const SRVRequest::SRV_ptr b) {
            if (a->priority > b->priority) return false;
            if (a->priority < b->priority) return true;
            return a->weight > b->weight;
        });
    }

    static void parse_TXT(unsigned char* abuf, int alen, TXTRequest* record)
    {
        struct ares_txt_reply* txt_out;
        int result = ares_parse_txt_reply(abuf, alen, &txt_out);
        if (result != ARES_SUCCESS) {
            std::cerr << "Failed to parse TXT reply: " << ares_strerror(result) << std::endl;
            record->setComplete(true);
            return;
        }

        for (struct ares_txt_reply* txt = txt_out; txt != nullptr; txt = txt->next) {
            auto rawTxt = reinterpret_cast<char*>(txt->txt);
            if (!rawTxt) {
                continue;
            }

            const std::string _txt{rawTxt};
            record->entries.push_back(_txt);

            string_list tokens = simgear::strutils::split(_txt, "=", 1);
            if (tokens.size() == 2) {
                record->attributes[tokens[0]] = tokens[1];
            }

        }
    }

    ares_channel_t* channel = NULL;

    static size_t instanceCounter;
};

size_t Client::ClientPrivate::instanceCounter = 0;

Request::Request( const std::string & dn ) :
        _dn(dn),
        _type(-1),
        _complete(false),
        _timeout_secs(5),
        _start(0)
{
}

Request::~Request()
{
}

void Request::cancel()
{
    _cancelled = true;
}

bool Request::isTimeout() const
{
    return (time(NULL) - _start) > _timeout_secs;
}

NAPTRRequest::NAPTRRequest( const std::string & dn ) :
        Request(dn)
{
    _type = ARES_REC_TYPE_NAPTR;
}

SRVRequest::SRVRequest( const std::string & dn ) :
        Request(dn)
{
    _type = ARES_REC_TYPE_SRV;
}

SRVRequest::SRVRequest(const std::string& dn, const std::string& service, const std::string& protocol) : Request(dn),
                                                                                                         _service(service),
                                                                                                         _protocol(protocol)
{
    _type = ARES_REC_TYPE_SRV;
}

void SRVRequest::submit( Client * client )
{
    _start = time(NULL);
}

TXTRequest::TXTRequest( const std::string & dn ) :
        Request(dn)
{
    _type = ARES_REC_TYPE_TXT;
}

void TXTRequest::submit( Client * client )
{
    _start = time(NULL);
}

void NAPTRRequest::submit( Client * client )
{
    _start = time(NULL);
}

Client::Client(const std::string& nameserver) : d(new ClientPrivate{nameserver})
{
}

Client::~Client()
{
}

void Client::makeRequest(const Request_ptr& r)
{
    r->submit(this);
    d->query( r );
}

void Client::update(int waitTimeout)
{
    //not needed for c-ares
}

} // of namespace DNS

} // of namespace simgear
