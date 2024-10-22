// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 James Turner <james@flightgear.org>

/**
 * @file
 * @brief Simple HTTP client engine for SimHear
 */

#ifndef SG_HTTP_CLIENT_HXX
#define SG_HTTP_CLIENT_HXX

#include <functional>
#include <memory>   // for std::unique_ptr
#include <stdint.h> // for uint_64t

#include <simgear/io/HTTPFileRequest.hxx>
#include <simgear/io/HTTPMemoryRequest.hxx>

namespace simgear
{

namespace HTTP
{

// forward decls
class Connection;

class Client final
{
public:
    Client();
    ~Client();  // non-virtual is intentional

    void update(int waitTimeout = 0);

    void reset();

    void makeRequest(const Request_ptr& r);

    void cancelRequest(const Request_ptr& r, std::string reason = std::string());

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

    void setMaxHostConnections(unsigned int maxHostConns);

    /**
     * maximum depth to pipeline requests - set to 0 to disable pipelining
     */
    void setMaxPipelineDepth(unsigned int depth);

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

    void debugDumpRequests();

    void clearAllConnections();
private:
    // libCurl callbacks
    static size_t requestWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t requestReadCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t requestHeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata);

    void requestFinished(Connection* con);

    void receivedBytes(unsigned int count);

    friend class Connection;
    friend class Request;
    friend class TestApi;

    class ClientPrivate;
    std::unique_ptr<ClientPrivate> d;
};

} // of namespace HTTP

} // of namespace simgear

#endif // of SG_HTTP_CLIENT_HXX
