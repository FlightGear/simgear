///@file
//
// Copyright (C) 2011  James Turner <zakalawe@mac.com>
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef SG_HTTP_REQUEST_HXX
#define SG_HTTP_REQUEST_HXX

#include <map>

#include <simgear/structure/function_list.hxx>
#include <simgear/structure/map.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/sg_types.hxx>

#include <boost/bind.hpp>

class SGPropertyNode;

namespace simgear
{
namespace HTTP
{

/**
 * Base class for HTTP request (and answer).
 */
class Request:
  public SGReferenced
{
public:
    typedef boost::function<void(Request*)> Callback;

    enum ReadyState
    {
      UNSENT = 0,
      OPENED,
      HEADERS_RECEIVED,
      LOADING,
      DONE,
      FAILED
    };

    virtual ~Request();

    /**
     *
     */
    StringMap& requestHeaders()             { return _request_headers; }
    const StringMap& requestHeaders() const { return _request_headers; }
    std::string& requestHeader(const std::string& key)
                                            { return _request_headers[key]; }
    const std::string requestHeader(const std::string& key) const
                                            { return _request_headers.get(key); }

    /**
     * Add a handler to be called when the request successfully completes.
     *
     * @note If the request is already complete, the handler is called
     *       immediately.
     */
    Request* done(const Callback& cb);

    template<class C>
    Request* done(C* instance, void (C::*mem_func)(Request*))
    {
      return done(boost::bind(mem_func, instance, _1));
    }

    /**
     * Add a handler to be called when the request completes or aborts with an
     * error.
     *
     * @note If the request has already failed, the handler is called
     *       immediately.
     */
    Request* fail(const Callback& cb);

    template<class C>
    Request* fail(C* instance, void (C::*mem_func)(Request*))
    {
      return fail(boost::bind(mem_func, instance, _1));
    }

    /**
     * Add a handler to be called when the request either successfully completes
     * or fails.
     *
     * @note If the request is already complete or has already failed, the
     *       handler is called immediately.
     */
    Request* always(const Callback& cb);

    template<class C>
    Request* always(C* instance, void (C::*mem_func)(Request*))
    {
      return always(boost::bind(mem_func, instance, _1));
    }

    /**
     * Set the data for the body of the request. The request is automatically
     * send using the POST method.
     *
     * @param data  Body data
     * @param type  Media Type (aka MIME) of the body data
     */
    void setBodyData( const std::string& data,
                      const std::string& type = "text/plain" );
    void setBodyData( const SGPropertyNode* data );

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
    
    StringMap const& responseHeaders() const
        { return _responseHeaders; }

    std::string responseMime() const;

    virtual int responseCode() const
        { return _responseStatus; }
        
    virtual std::string responseReason() const
        { return _responseReason; }
        
    void setResponseLength(unsigned int l);
    virtual unsigned int responseLength() const;
  
    /**
     * Check if request contains body data.
     */
    virtual bool hasBodyData() const;

    /**
     * Retrieve the request body content type.
     */
    virtual std::string bodyType() const;

    /**
     * Retrieve the size of the request body.
     */
    virtual size_t bodyLength() const;
    
    /**
     * Retrieve the body data bytes. Will be passed the maximum body bytes
     * to return in the buffer, and must return the actual number
     * of bytes written. 
     */
    virtual size_t getBodyData(char* s, size_t offset, size_t max_count) const;
  
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
    
    ReadyState readyState() const { return _ready_state; }

    /**
     * Request aborting this request.
     */
    void abort();

    /**
     * Request aborting this request and specify the reported reaseon for it.
     */
    void abort(const std::string& reason);

    bool closeAfterComplete() const;
    bool isComplete() const;

protected:
    Request(const std::string& url, const std::string method = "GET");

    virtual void requestStart();
    virtual void responseStart(const std::string& r);
    virtual void responseHeader(const std::string& key, const std::string& value);
    virtual void responseHeadersComplete();
    virtual void responseComplete();
    virtual void gotBodyData(const char* s, int n);

    virtual void onDone();
    virtual void onFail();
    virtual void onAlways();

    void setFailure(int code, const std::string& reason);

private:
    friend class Client;
    friend class Connection;
    friend class ContentDecoder;
    
    Request(const Request&); // = delete;
    Request& operator=(const Request&); // = delete;

    void processBodyBytes(const char* s, int n);
    void setReadyState(ReadyState state);

    std::string   _method;
    std::string   _url;
    StringMap     _request_headers;
    std::string   _request_data;
    std::string   _request_media_type;

    HTTPVersion   _responseVersion;
    int           _responseStatus;
    std::string   _responseReason;
    StringMap     _responseHeaders;
    unsigned int  _responseLength;
    unsigned int  _receivedBodyBytes;

    function_list<Callback> _cb_done,
                            _cb_fail,
                            _cb_always;

    ReadyState    _ready_state;
    bool          _willClose;
};

typedef SGSharedPtr<Request> Request_ptr;

} // of namespace HTTP
} // of namespace simgear

#endif // of SG_HTTP_REQUEST_HXX
