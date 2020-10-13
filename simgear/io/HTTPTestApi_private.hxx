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