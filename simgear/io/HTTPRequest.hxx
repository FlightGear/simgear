#ifndef SG_HTTP_REQUEST_HXX
#define SG_HTTP_REQUEST_HXX

#include <map>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/sg_types.hxx>

namespace simgear
{

namespace HTTP
{

class Request : public SGReferenced
{
public:
    virtual ~Request();
    
    virtual void setUrl(const std::string& url);
    
    virtual std::string method() const
        { return _method; }
    virtual std::string url() const
        { return _url; }
    
    virtual std::string scheme() const;
    virtual std::string path() const;
    virtual std::string host() const;
    virtual std::string hostAndPort() const;
    virtual unsigned short port() const;
    
    virtual string_list requestHeaders() const;
    virtual std::string header(const std::string& name) const;

    virtual int responseCode() const
        { return _responseStatus; }
        
    virtual std::string responseReason() const
        { return _responseReason; }
        
    void setResponseLength(unsigned int l);    
    virtual unsigned int responseLength() const;
    
    /**
     * running total of body bytes received so far. Can be used
     * to generate a completion percentage, if the response length is
     * known. 
     */
    unsigned int responseBytesReceived() const
        { return _receivedBodyBytes; }
protected:
    Request(const std::string& url, const std::string method = "GET");

    virtual void responseStart(const std::string& r);
    virtual void responseHeader(const std::string& key, const std::string& value);
    virtual void responseHeadersComplete();
    virtual void responseComplete();
    virtual void failed();
    virtual void gotBodyData(const char* s, int n);
private:
    friend class Client;
    friend class Connection;
    
    void processBodyBytes(const char* s, int n);
    void setFailure(int code, const std::string& reason);

    std::string _method;
    std::string _url;
    int _responseStatus;
    std::string _responseReason;
    unsigned int _responseLength;
    unsigned int _receivedBodyBytes;
    
    typedef std::map<std::string, std::string> HeaderDict;
    HeaderDict _responseHeaders; 
};

typedef SGSharedPtr<Request> Request_ptr;

} // of namespace HTTP

} // of namespace simgear

#endif // of SG_HTTP_REQUEST_HXX

