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

class SGPropertyNode;

namespace simgear
{
namespace HTTP
{

class Client;

/**
 * Base class for HTTP request (and answer).
 */
class Request:
  public SGReferenced
{
public:
    typedef std::function<void(Request*)> Callback;

    enum ReadyState
    {
      UNSENT = 0,
      OPENED,
      STATUS_RECEIVED,
      HEADERS_RECEIVED,
      LOADING,
      DONE,
      FAILED,
      CANCELLED
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
      return done(std::bind(mem_func, instance, std::placeholders::_1));
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
      return fail(std::bind(mem_func, instance, std::placeholders::_1));
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
      return always(std::bind(mem_func, instance, std::placeholders::_1));
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
    
    /*
     * Set Range header, e.g. "1234-" to skip first 1234 bytes.
     *
     * If a range is specified, we treat http response codes 206 'Partial
     * Content' and 416 'Range Not Satisfiable' both as success.
     *
     * The latter is unfortunate - e.g. with range='1234-' it is returned if
     * the remote file is exactly 1234 bytes long, which for example will occur
     * if we are updating a file that was fully downloaded previously.
     */
    virtual void setRange(const std::string& range);
    
    /*
     * Control underlying curl library's automatic decompression support. <enc>
     * is passed directly to curl_easy_setopt(CURLOPT_ACCEPT_ENCODING); see
     * CURLOPT_ACCEPT_ENCODING(3) for details.
     *
     * E.g. pass env="" to allow any available compression algorithm.
     */
    virtual void setAcceptEncoding(const char* enc);
    
    const char* getAcceptEncoding();

    virtual std::string method() const
        { return _method; }
    virtual std::string url() const
        { return _url; }
    virtual std::string range() const
        { return _range; }
    
    /*
     * Limits download speed using CURLOPT_MAX_RECV_SPEED_LARGE.
     */
    void setMaxBytesPerSec(unsigned long maxBytesPerSec)
        { _maxBytesPerSec = maxBytesPerSec; }
    
    unsigned long getMaxBytesPerSec() const
        { return _maxBytesPerSec; }

    Client* http() const
    { return _client; }

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

    bool closeAfterComplete() const;
    bool isComplete() const;

    /**
     * Check if the server response indicates pipelining should be continued.
     * Currently tests that HTTP_1_1 is explicitly supported, and that the
     * server/proxy did not request Connection: close
     */
    bool serverSupportsPipelining() const;

    virtual void prepareForRetry();

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

    void setSuccess(int code);
    void setFailure(int code, const std::string &reason);

  private:
    friend class Client;
    friend class Connection;
    friend class ContentDecoder;
    friend class TestApi;

    Request(const Request&); // = delete;
    Request& operator=(const Request&); // = delete;

    void processBodyBytes(const char* s, int n);
    void setReadyState(ReadyState state);

    void setCloseAfterComplete();

    Client*       _client; // HTTP client we're active on

    std::string   _method;
    std::string   _url;
    std::string   _range;
    bool          _enc_set = false;
    std::string   _enc;
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
    bool          _connectionCloseHeader;
    unsigned long _maxBytesPerSec = 0;
};

typedef SGSharedPtr<Request> Request_ptr;

} // of namespace HTTP
} // of namespace simgear

#endif // of SG_HTTP_REQUEST_HXX
