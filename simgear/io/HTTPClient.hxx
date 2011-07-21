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
    
    void makeRequest(const Request_ptr& r);
    
    void setUserAgent(const std::string& ua);
    void setProxy(const std::string& proxy, const std::string& auth = "");
    
    const std::string& userAgent() const
        { return _userAgent; }
        
    const std::string& proxyHost() const
        { return _proxy; }
        
    const std::string& proxyAuth() const
        { return _proxyAuth; }
private:
    void requestFinished(Connection* con);
    
    friend class Connection;
    
    std::string _userAgent;
    std::string _proxy;
    std::string _proxyAuth;
    
// connections by host
    std::map<std::string, Connection*> _connections;
};

} // of namespace HTTP

} // of namespace simgear

#endif // of SG_HTTP_CLIENT_HXX
