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
    virtual std::string query() const;
    
    virtual string_list requestHeaders() const;
    virtual std::string header(const std::string& name) const;

    virtual int responseCode() const
        { return _responseStatus; }
        
    virtual std::string responseReason() const
        { return _responseReason; }
        
    void setResponseLength(unsigned int l);    
    virtual unsigned int responseLength() const;
  
    /**
     * Query the size of the request body. -1 (the default value) means no
     * request body
     */
    virtual int requestBodyLength() const;
    
    /**
     * Retrieve the body data bytes. Will be passed the maximum body bytes
     * to return in the buffer, and must return the actual number
     * of bytes written. 
     */
    virtual int getBodyData(char* s, int count) const;
  
    /**
     * retrieve the request body content type. Default is text/plain
     */
    virtual std::string requestBodyType() const;
  
    /**
     * running total of body bytes received so far. Can be used
     * to generate a completion percentage, if the response length is
     * known. 
     */
    unsigned int responseBytesReceived() const
        { return _receivedBodyBytes; }
        
    enum HTTPVersion {
        HTTP_VERSION_UNKNOWN = 0,
        HTTP_0_x, // 0.9 or similar
        HTTP_1_0,
        HTTP_1_1
    };
    
    HTTPVersion responseVersion() const
        { return _responseVersion; }
    
    static HTTPVersion decodeVersion(const std::string& v);
    
    bool closeAfterComplete() const;
protected:
    Request(const std::string& url, const std::string method = "GET");

    virtual void requestStart();
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
    HTTPVersion _responseVersion;
    int _responseStatus;
    std::string _responseReason;
    unsigned int _responseLength;
    unsigned int _receivedBodyBytes;
    bool _willClose;
    
    typedef std::map<std::string, std::string> HeaderDict;
    HeaderDict _responseHeaders; 
};

typedef SGSharedPtr<Request> Request_ptr;

} // of namespace HTTP

} // of namespace simgear

#endif // of SG_HTTP_REQUEST_HXX

