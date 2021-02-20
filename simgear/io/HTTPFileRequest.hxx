///@file
//
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

#ifndef SG_HTTP_FILEREQUEST_HXX_
#define SG_HTTP_FILEREQUEST_HXX_

#include <simgear/misc/sg_path.hxx>

#include "HTTPRequest.hxx"

#include <simgear/io/iostreams/sgstream.hxx>

#include <functional>

namespace simgear
{
namespace HTTP
{

  /**
   * HTTP request writing response to a file.
   */
  class FileRequest:
    public Request
  {
    public:

      /**
       *
       * @param url     Adress to download from
       * @param path    Path to file for saving response
       * @append        If true we assume any existing file is partial download
       *                and use simgear::HTTP::Request::setRange() to download
       *                and append any remaining data. We don't currently
       *                attempt to use an If-Range validator to protect against
       *                any existing file containing incorrect data.
       */
      FileRequest(const std::string& url, const std::string& path, bool append=false);
      
      /*
       * Set callback for each chunk of data we receive. Called with (nullptr,
       * 0) when download has completed (successfully or unsuccesfully) - this
       * will be done from inside setCallback() if the download has already
       * finished.
       */
      void setCallback(std::function<void(const void* data, size_t numbytes)> callbackf);
      
    protected:
      SGPath _filename;
      sg_ofstream _file;
      bool _append;
      std::function<void(const void* data, size_t numbytes)> _callback;

      virtual void responseHeadersComplete();
      virtual void gotBodyData(const char* s, int n);
      virtual void onAlways();
  };

  typedef SGSharedPtr<FileRequest> FileRequestRef;

} // namespace HTTP
} // namespace simgear

#endif /* SG_HTTP_FILEREQUEST_HXX_ */
