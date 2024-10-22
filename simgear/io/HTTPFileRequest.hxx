// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief HTTP request writing response to a file.
 */

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
