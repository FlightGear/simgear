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
#include <udns.h>
#include <ctime>

#include <simgear/debug/logstream.hxx>

namespace simgear {

namespace DNS {

class Client::ClientPrivate {
public:
    ClientPrivate(const std::string& nameserver)
    {
        if( instanceCounter++ == 0 )
          if (dns_init(NULL, 0) < 0)
              SG_LOG(SG_IO, SG_ALERT, "Can't init udns library" );

        ctx = dns_new(NULL);

        if (dns_init(ctx, 0) < 0)
            SG_LOG(SG_IO, SG_ALERT, "Can't create udns context" );

        if (!nameserver.empty()) {
            dns_add_serv(ctx, nullptr);
            dns_add_serv(ctx, nameserver.c_str());
        }

        if( dns_open(ctx) < 0 )
            SG_LOG(SG_IO, SG_ALERT, "Can't open udns context" );
    }

    ~ClientPrivate() {
        dns_close(ctx);
        dns_free(ctx);
        if( --instanceCounter == 0 )
            dns_close(NULL);
    }

    struct dns_ctx * ctx;
    static size_t instanceCounter;

    using RequestVec = std::vector<Request_ptr>;

    RequestVec _activeRequests;
};

size_t Client::ClientPrivate::instanceCounter = 0;

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
    _type = DNS_T_NAPTR;
}

SRVRequest::SRVRequest( const std::string & dn ) :
        Request(dn)
{
    _type = DNS_T_SRV;
}

SRVRequest::SRVRequest( const std::string & dn, const string & service, const string & protocol ) :
        Request(dn),
        _service(service),
        _protocol(protocol)
{
    _type = DNS_T_SRV;
}

static bool sortSRV( const SRVRequest::SRV_ptr a, const SRVRequest::SRV_ptr b )
{
    if( a->priority > b->priority ) return false;
    if( a->priority < b->priority ) return true;
    return a->weight > b->weight;
}

static void dnscbSRV(struct dns_ctx *ctx, struct dns_rr_srv *result, void *data)
{
    SRVRequest * r = static_cast<SRVRequest*>(data);
    if (result) {
        if (!r->isCancelled()) {
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
            std::sort(r->entries.begin(), r->entries.end(), sortSRV);
        }
        free(result);
    }
    r->setComplete();
}

void SRVRequest::submit( Client * client )
{
    // if service is defined, pass service and protocol
    auto q = dns_submit_srv(client->d->ctx, getDn().c_str(), _service.empty() ? NULL : _service.c_str(),
                            _service.empty() ? NULL : _protocol.c_str(),
                            0, dnscbSRV, this);

    if (!q) {
        SG_LOG(SG_IO, SG_ALERT, "Can't submit dns request for " << getDn());
        return;
    }
    _start = time(NULL);
    _query = q;
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
        if (!r->isCancelled()) {
            r->cname = result->dnstxt_cname;
            r->qname = result->dnstxt_qname;
            r->ttl = result->dnstxt_ttl;
            for (int i = 0; i < result->dnstxt_nrr; i++) {
                //TODO: interprete the .len field of dnstxt_txt?
                auto rawTxt = reinterpret_cast<char*>(result->dnstxt_txt[i].txt);
                if (!rawTxt) {
                    continue;
                }

                const string txt{rawTxt};
                r->entries.push_back(txt);
                string_list tokens = simgear::strutils::split(txt, "=", 1);
                if (tokens.size() == 2) {
                    r->attributes[tokens[0]] = tokens[1];
                }
            }
        }
        free(result);
    }
    r->setComplete();
}

void TXTRequest::submit( Client * client )
{
    // protocol and service an already encoded in DN so pass in NULL for both
    auto q = dns_submit_txt(client->d->ctx, getDn().c_str(), DNS_C_IN, 0, dnscbTXT, this);
    if (!q) {
        SG_LOG(SG_IO, SG_ALERT, "Can't submit dns request for " << getDn());
        return;
    }
    _start = time(NULL);
    _query = q;
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
        if (!r->isCancelled()) {
            r->cname = result->dnsnaptr_cname;
            r->qname = result->dnsnaptr_qname;
            r->ttl = result->dnsnaptr_ttl;
            for (int i = 0; i < result->dnsnaptr_nrr; i++) {
                if (!r->qservice.empty() && r->qservice != result->dnsnaptr_naptr[i].service)
                    continue;

                //TODO: case ignore and result flags may have more than one flag
                if (!r->qflags.empty() && r->qflags != result->dnsnaptr_naptr[i].flags)
                    continue;

                NAPTRRequest::NAPTR_ptr naptr(new NAPTRRequest::NAPTR);
                r->entries.push_back(naptr);
                naptr->order = result->dnsnaptr_naptr[i].order;
                naptr->preference = result->dnsnaptr_naptr[i].preference;
                naptr->flags = result->dnsnaptr_naptr[i].flags;
                naptr->service = result->dnsnaptr_naptr[i].service;
                naptr->regexp = result->dnsnaptr_naptr[i].regexp;
                naptr->replacement = result->dnsnaptr_naptr[i].replacement;
            }
            std::sort(r->entries.begin(), r->entries.end(), sortNAPTR);
        }
        free(result);
    }
    r->setComplete();
}

void NAPTRRequest::submit( Client * client )
{
    auto q = dns_submit_naptr(client->d->ctx, getDn().c_str(), 0, dnscbNAPTR, this);
    if (!q) {
        SG_LOG(SG_IO, SG_ALERT, "Can't submit dns request for " << getDn());
        return;
    }
    _start = time(NULL);
    _query = q;
}

Client::Client(const std::string& nameserver) : d(new ClientPrivate{nameserver})
{
}

Client::~Client()
{
}

void Client::makeRequest(const Request_ptr& r)
{
    d->_activeRequests.push_back(r);
    r->submit(this);
}

void Client::update(int waitTimeout)
{
    time_t now = time(NULL);
    if( dns_timeouts( d->ctx, -1, now ) < 0 )
        return;

    dns_ioevent(d->ctx, now);

    // drop our owning ref to completed requests,
    // and cancel any which timed out
    auto it = std::remove_if(d->_activeRequests.begin(), d->_activeRequests.end(),
                             [this](const Request_ptr& r) {
                                 // check complete-ness before timeout,
                                 // since both can be true, but cancelling
                                 // a completed request breaks udns
                                 if (r->isComplete()) {
                                     return true;
                                 }

                                 if (r->isTimeout()) {
                                     dns_cancel(d->ctx, reinterpret_cast<struct dns_query*>(r->_query));
                                     return true;
                                 }

                                 return false;
                             });
    d->_activeRequests.erase(it, d->_activeRequests.end());
}

} // of namespace DNS

} // of namespace simgear
