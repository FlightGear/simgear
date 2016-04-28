/**
 * \file DNSClient.hxx - simple DNS resolver client for SimGear
 */

// Written by Torsten Dreyer
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

#ifndef SG_DNS_CLIENT_HXX
#define SG_DNS_CLIENT_HXX

#include <memory> // for std::auto_ptr
#include <string>
#include <vector>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

namespace simgear
{

namespace DNS
{

class Request : public SGReferenced
{
public:
	Request( const std::string & dn );
	virtual ~Request();
	std::string getDn() const { return _dn; }
	int getType() const { return _type; }
	bool complete() const { return _complete; }
	void setComplete( bool b = true ) { _complete = b; }

	virtual void submit() = 0;
protected:
	std::string _dn;
	int _type;
	bool _complete;
};

class NAPTRRequest : public Request
{
public:
	NAPTRRequest( const std::string & dn );
	virtual void submit();

	struct NAPTR : SGReferenced {
		int order;
		int preference;
		std::string flags;
		std::string service;
		std::string regexp;
		std::string replacement;
	};
	typedef SGSharedPtr<NAPTR> NAPTR_ptr;
	typedef std::vector<NAPTR_ptr> NAPTR_list;
	std::string cname;
	std::string qname;
	unsigned ttl;
	NAPTR_list entries;
};

typedef SGSharedPtr<Request> Request_ptr;

class Client
{
public:
    Client();
    ~Client();

    void update(int waitTimeout = 0);

    void makeRequest(const Request_ptr& r);

//    void cancelRequest(const Request_ptr& r, std::string reason = std::string());

private:

    class ClientPrivate;
    std::auto_ptr<ClientPrivate> d;
};

} // of namespace DNS

} // of namespace simgear

#endif // of SG_DNS_CLIENT_HXX
