// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <functional>

#include "HTTPRequest.hxx"

namespace simgear {
namespace HTTP {

class Client;

using ResponseDoneCallback =
    std::function<bool(int curlResult, Request_ptr req)>;

/**
 * @brief this API is for unit-testing HTTP code.
 * Don't use it for anything else. It's for unit-testing.
 */
class TestApi {
public:
  // alow test suite to manipulate requests to simulate network errors;
  // without this, it's hard to provoke certain failures in a loop-back
  // network sitation.
  static void setResponseDoneCallback(Client *cl, ResponseDoneCallback cb);

  static void markRequestAsFailed(Request_ptr req, int curlCode,
                                  const std::string &message);
};

} // namespace HTTP
} // namespace simgear