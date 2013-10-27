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

#include <memory> // for std::auto_ptr
#include <stdint.h> // for uint_64t

#include <simgear/io/HTTPFileRequest.hxx>
#include <simgear/io/HTTPMemoryRequest.hxx>

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

    /**
     * Download a resource and save it to a file.
     *
     * @param url       The resource to download
     * @param filename  Path to the target file
     * @param data      Data for POST request
     */
    FileRequestRef save( const std::string& url,
                         const std::string& filename );

    /**
     * Request a resource and keep it in memory.
     *
     * @param url   The resource to download
     */
    MemoryRequestRef load(const std::string& url);

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
    
    /**
     * crude tracking of bytes-per-second transferred over the socket.
     * suitable for user feedback and rough profiling, nothing more.
     */
    unsigned int transferRateBytesPerSec() const;
    
    /**
     * total bytes downloaded by this HTTP client, for bandwidth usage
     * monitoring
     */
    uint64_t totalBytesDownloaded() const;
private:
    void requestFinished(Connection* con);
    
    void receivedBytes(unsigned int count);
    
    friend class Connection;
    friend class Request;
    
    class ClientPrivate;
    std::auto_ptr<ClientPrivate> d;
};

} // of namespace HTTP

} // of namespace simgear

#endif // of SG_HTTP_CLIENT_HXX
