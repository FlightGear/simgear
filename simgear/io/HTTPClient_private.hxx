// SPDX-License-Identifier: LGPL-2.1-or-later

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
