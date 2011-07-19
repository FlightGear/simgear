#include "HTTPRequest.hxx"

#include <simgear/misc/strutils.hxx>
#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>

using std::string;
using std::map;

#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

namespace simgear
{

namespace HTTP
{

Request::Request(const string& url, const string method) :
    _method(method),
    _url(url)
{
    
}

Request::~Request()
{
    
}

string_list Request::requestHeaders() const
{
    string_list r;
    return r;
}

string Request::header(const std::string& name) const
{
    return string();
}

void Request::responseStart(const string& r)
{
    const int maxSplit = 2; // HTTP/1.1 nnn status code
    string_list parts = strutils::split(r, NULL, maxSplit);
    _responseStatus = strutils::to_int(parts[1]);
    _responseReason = parts[2];
}

void Request::responseHeader(const string& key, const string& value)
{
    _responseHeaders[key] = value;
}

void Request::responseHeadersComplete()
{
    // no op
}

void Request::gotBodyData(const char* s, int n)
{

}

void Request::responseComplete()
{
    
}
    
string Request::scheme() const
{
    int firstColon = url().find(":");
    if (firstColon > 0) {
        return url().substr(0, firstColon);
    }
    
    return ""; // couldn't parse scheme
}
    
string Request::path() const
{
    string u(url());
    int schemeEnd = u.find("://");
    if (schemeEnd < 0) {
        return ""; // couldn't parse scheme
    }
    
    int hostEnd = u.find('/', schemeEnd + 3);
    if (hostEnd < 0) { 
        return ""; // couldn't parse host
    }
    
    int query = u.find('?', hostEnd + 1);
    if (query < 0) {
        // all remainder of URL is path
        return u.substr(hostEnd);
    }
    
    return u.substr(hostEnd, query - hostEnd);
}

string Request::host() const
{
    string u(url());
    int schemeEnd = u.find("://");
    if (schemeEnd < 0) {
        return ""; // couldn't parse scheme
    }
    
    int hostEnd = u.find('/', schemeEnd + 3);
    if (hostEnd < 0) { // all remainder of URL is host
        return u.substr(schemeEnd + 3);
    }
    
    return u.substr(schemeEnd + 3, hostEnd - (schemeEnd + 3));
}

int Request::contentLength() const
{
    HeaderDict::const_iterator it = _responseHeaders.find("content-length");
    if (it == _responseHeaders.end()) {
        return 0;
    }
    
    return strutils::to_int(it->second);
}

} // of namespace HTTP

} // of namespace simgear
