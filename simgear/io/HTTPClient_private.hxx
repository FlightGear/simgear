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

#pragma once

#include <list>
#include <map>

#include "HTTPClient.hxx"
#include "HTTPRequest.hxx"

#include <simgear/timing/timestamp.hxx>

#include <curl/multi.h>

namespace simgear {
namespace HTTP {

typedef std::list<Request_ptr> RequestList;

using ResponseDoneCallback =
    std::function<bool(int curlResult, Request_ptr req)>;

class Client::ClientPrivate {
public:
  CURLM *curlMulti;

  void createCurlMulti();

  typedef std::map<Request_ptr, CURL *> RequestCurlMap;
  RequestCurlMap requests;

  std::string userAgent;
  std::string proxy;
  int proxyPort;
  std::string proxyAuth;
  unsigned int maxConnections;
  unsigned int maxHostConnections;
  unsigned int maxPipelineDepth;

  RequestList pendingRequests;

  bool curlPerformActive = false;
  RequestList pendingCancelRequests;

  SGTimeStamp timeTransferSample;
  unsigned int bytesTransferred;
  unsigned int lastTransferRate;
  uint64_t totalBytesDownloaded;

  SGPath tlsCertificatePath;

  // only used by unit-tests / test-api, but
  // only costs us a pointe here to declare it.
  ResponseDoneCallback testsuiteResponseDoneCallback;
};

} // namespace HTTP

} // namespace simgear
