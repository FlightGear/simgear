// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief HTTP request keeping response in memory.
 */

#include <simgear_config.h>
#include "HTTPMemoryRequest.hxx"

namespace simgear
{
namespace HTTP
{

  //----------------------------------------------------------------------------
  MemoryRequest::MemoryRequest(const std::string& url):
    Request(url, "GET")
  {

  }

  //----------------------------------------------------------------------------
  const std::string& MemoryRequest::responseBody() const
  {
    return _response;
  }

  //----------------------------------------------------------------------------
  void MemoryRequest::responseHeadersComplete()
  {
    Request::responseHeadersComplete();

    if( responseLength() )
      _response.reserve( responseLength() );
  }

  //----------------------------------------------------------------------------
  void MemoryRequest::gotBodyData(const char* s, int n)
  {
    _response.append(s, n);
  }

} // namespace HTTP
} // namespace simgear
