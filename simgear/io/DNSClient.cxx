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

#include "DNSClient.hxx"
#include "udns.h"
#include <time.h>
#include <simgear/debug/logstream.hxx>

namespace simgear {

namespace DNS {

class Client::ClientPrivate {
public:
    ClientPrivate() {
        if (dns_init(NULL, 0) < 0)
            SG_LOG(SG_IO, SG_ALERT, "Can't init udns library" );

        if( dns_open(NULL) < 0 )
            SG_LOG(SG_IO, SG_ALERT, "Can't open udns context" );
    }

    ~ClientPrivate() {
        dns_close(NULL);
    }
};

Request::Request( const std::string & dn ) :
        _dn(dn),
        _type(DNS_T_ANY),
        _complete(false),
        _timeout_secs(5),
        _start(0)
{
}

Request::~Request()
{
}

bool Request::isTimeout() const
{
    return (time(NULL) - _start) > _timeout_secs;
}

NAPTRRequest::NAPTRRequest( const std::string & dn ) :
        Request(dn)
{
    _type = DNS_T_NAPTR;
}

SRVRequest::SRVRequest( const std::string & dn ) :
        Request(dn)
{
    _type = DNS_T_SRV;
}

static void dnscbSRV(struct dns_ctx *ctx, struct dns_rr_srv *result, void *data)
{
    SRVRequest * r = static_cast<SRVRequest*>(data);
    if (result) {
        r->cname = result->dnssrv_cname;
        r->qname = result->dnssrv_qname;
        r->ttl = result->dnssrv_ttl;
        for (int i = 0; i < result->dnssrv_nrr; i++) {
            SRVRequest::SRV_ptr srv(new SRVRequest::SRV);
            r->entries.push_back(srv);
            srv->priority = result->dnssrv_srv[i].priority;
            srv->weight = result->dnssrv_srv[i].weight;
            srv->port = result->dnssrv_srv[i].port;
            srv->target = result->dnssrv_srv[i].name;
        }
//        std::sort( r->entries.begin(), r->entries.end(), sortNAPTR );
        free(result);
    }
    r->setComplete();
}
void SRVRequest::submit()
{
    // protocol and service an already encoded in DN so pass in NULL for both
    if (!dns_submit_srv(NULL, getDn().c_str(), NULL, NULL, 0, dnscbSRV, this )) {
        SG_LOG(SG_IO, SG_ALERT, "Can't submit dns request for " << getDn());
        return;
    }
    _start = time(NULL);
}

TXTRequest::TXTRequest( const std::string & dn ) :
        Request(dn)
{
    _type = DNS_T_TXT;
}

static void dnscbTXT(struct dns_ctx *ctx, struct dns_rr_txt *result, void *data)
{
    TXTRequest * r = static_cast<TXTRequest*>(data);
    if (result) {
        r->cname = result->dnstxt_cname;
        r->qname = result->dnstxt_qname;
        r->ttl = result->dnstxt_ttl;
        for (int i = 0; i < result->dnstxt_nrr; i++) {
          //TODO: interprete the .len field of dnstxt_txt?
          r->entries.push_back(string((char*)result->dnstxt_txt[i].txt));
        }
        free(result);
    }
    r->setComplete();
}

void TXTRequest::submit()
{
    // protocol and service an already encoded in DN so pass in NULL for both
    if (!dns_submit_txt(NULL, getDn().c_str(), 0, 0, dnscbTXT, this )) {
        SG_LOG(SG_IO, SG_ALERT, "Can't submit dns request for " << getDn());
        return;
    }
    _start = time(NULL);
}


static bool sortNAPTR( const NAPTRRequest::NAPTR_ptr a, const NAPTRRequest::NAPTR_ptr b )
{
    if( a->order > b->order ) return false;
    if( a->order < b->order ) return true;
    return a->preference < b->preference;
}

static void dnscbNAPTR(struct dns_ctx *ctx, struct dns_rr_naptr *result, void *data)
{
    NAPTRRequest * r = static_cast<NAPTRRequest*>(data);
    if (result) {
        r->cname = result->dnsnaptr_cname;
        r->qname = result->dnsnaptr_qname;
        r->ttl = result->dnsnaptr_ttl;
        for (int i = 0; i < result->dnsnaptr_nrr; i++) {
            if( !r->qservice.empty() && r->qservice != result->dnsnaptr_naptr[i].service )
                return;

            //TODO: case ignore and result flags may have more than one flag
            if( !r->qflags.empty() && r->qflags != result->dnsnaptr_naptr[i].flags )
                return;

            NAPTRRequest::NAPTR_ptr naptr(new NAPTRRequest::NAPTR);
            r->entries.push_back(naptr);
            naptr->order = result->dnsnaptr_naptr[i].order;
            naptr->preference = result->dnsnaptr_naptr[i].preference;
            naptr->flags = result->dnsnaptr_naptr[i].flags;
            naptr->service = result->dnsnaptr_naptr[i].service;
            naptr->regexp = result->dnsnaptr_naptr[i].regexp;
            naptr->replacement = result->dnsnaptr_naptr[i].replacement;
        }
        std::sort( r->entries.begin(), r->entries.end(), sortNAPTR );
        free(result);
    }
    r->setComplete();
}

void NAPTRRequest::submit()
{
    if (!dns_submit_naptr(NULL, getDn().c_str(), 0, dnscbNAPTR, this )) {
        SG_LOG(SG_IO, SG_ALERT, "Can't submit dns request for " << getDn());
        return;
    }
    _start = time(NULL);
}


Client::~Client()
{
}

Client::Client() :
    d(new ClientPrivate)
{
}

void Client::makeRequest(const Request_ptr& r)
{
    r->submit();
}

void Client::update(int waitTimeout)
{
    time_t now = time(NULL);
    if( dns_timeouts( NULL, waitTimeout, now ) < 0 )
        return;
    dns_ioevent(NULL, now);
}

} // of namespace DNS

} // of namespace simgear
