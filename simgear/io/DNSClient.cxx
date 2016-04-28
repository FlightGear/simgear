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
		_complete(false)
{
}

Request::~Request()
{
}

NAPTRRequest::NAPTRRequest( const std::string & dn ) :
		Request(dn)
{
	_type = DNS_T_NAPTR;
}

static void dnscbNAPTR(struct dns_ctx *ctx, struct dns_rr_naptr *result, void *data)
{
	NAPTRRequest * r = static_cast<NAPTRRequest*>(data);
	if (result) {
		r->cname = result->dnsnaptr_cname;
		r->qname = result->dnsnaptr_qname;
		r->ttl = result->dnsnaptr_ttl;
		for (int i = 0; i < result->dnsnaptr_nrr; i++) {
			NAPTRRequest::NAPTR_ptr naptr(new NAPTRRequest::NAPTR);
			r->entries.push_back(naptr);
			naptr->order = result->dnsnaptr_naptr[i].order;
			naptr->preference = result->dnsnaptr_naptr[i].preference;
			naptr->flags = result->dnsnaptr_naptr[i].flags;
			naptr->service = result->dnsnaptr_naptr[i].service;
			naptr->regexp = result->dnsnaptr_naptr[i].regexp;
			naptr->replacement = result->dnsnaptr_naptr[i].replacement;
		}
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
