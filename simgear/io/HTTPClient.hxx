#ifndef SG_HTTP_CLIENT_HXX
#define SG_HTTP_CLIENT_HXX

#include <map>

#include <simgear/io/HTTPRequest.hxx>

namespace simgear
{

namespace HTTP
{

class Connection;

class Client
{
public:
    Client();
    
    void update(int waitTimeout = 0);
    
    void makeRequest(const Request_ptr& r);
    
    void setUserAgent(const std::string& ua);
    void setProxy(const std::string& proxy, int port, const std::string& auth = "");
    
    const std::string& userAgent() const
        { return _userAgent; }
        
    const std::string& proxyHost() const
        { return _proxy; }
        
    const std::string& proxyAuth() const
        { return _proxyAuth; }
    
    /**
     * predicate, check if at least one connection is active, with at
     * least one request active or queued.
     */
    bool hasActiveRequests() const; 
private:
    void requestFinished(Connection* con);
    
    friend class Connection;
    friend class Request;
    
    std::string _userAgent;
    std::string _proxy;
    int _proxyPort;
    std::string _proxyAuth;
    
// connections by host
    typedef std::map<std::string, Connection*> ConnectionDict;
    ConnectionDict _connections;
};

} // of namespace HTTP

} // of namespace simgear

#endif // of SG_HTTP_CLIENT_HXX
