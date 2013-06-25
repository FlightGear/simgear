/**
 * \file HTTPClient.hxx - simple HTTP client engine for SimHear
 */

// Written by James Turner
//
// Copyright (C) 2013  James Turner  <zakalawe@mac.com>
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

#ifndef SG_HTTP_CLIENT_HXX
#define SG_HTTP_CLIENT_HXX

#include <simgear/io/HTTPRequest.hxx>

namespace simgear
{

namespace HTTP
{

// forward decls
class Connection;
    
class Client
{
public:
    Client();
    ~Client();
    
    void update(int waitTimeout = 0);
    
    void makeRequest(const Request_ptr& r);
    
    void setUserAgent(const std::string& ua);
    void setProxy(const std::string& proxy, int port, const std::string& auth = "");
    
    /**
     * Specify the maximum permitted simultaneous connections
     * (default value is 1)
     */
    void setMaxConnections(unsigned int maxCons);
    
    const std::string& userAgent() const;
        
    const std::string& proxyHost() const;
        
    const std::string& proxyAuth() const;
    
    /**
     * predicate, check if at least one connection is active, with at
     * least one request active or queued.
     */
    bool hasActiveRequests() const; 
private:
    void requestFinished(Connection* con);
    
    friend class Connection;
    friend class Request;
    
    class ClientPrivate;
    std::auto_ptr<ClientPrivate> d;
};

} // of namespace HTTP

} // of namespace simgear

#endif // of SG_HTTP_CLIENT_HXX
