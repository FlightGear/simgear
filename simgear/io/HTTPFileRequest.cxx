// HTTP request writing response to a file.
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

#include <simgear_config.h>

#include "HTTPFileRequest.hxx"
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

namespace simgear
{
namespace HTTP
{
  //----------------------------------------------------------------------------
  FileRequest::FileRequest(const std::string& url, const std::string& path, bool append):
    Request(url, "GET"),
    _filename(path),
    _append(append)
  {
    if (append && _filename.isFile()) {
      size_t size = _filename.sizeInBytes();
      if (size) {
        // Only download remaining bytes.
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%zi-", size);
        setRange(buffer);
      }
    }
  }

  void FileRequest::setCallback(std::function<void(const void* data, size_t numbytes)> callback)
  {
    _callback = callback;
    if (readyState() == DONE) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "making callback because readyState() == DONE");
        _callback(nullptr, 0);
    }
  }

  //----------------------------------------------------------------------------
  void FileRequest::responseHeadersComplete()
  {
    Request::responseHeadersComplete();
    SG_LOG(SG_GENERAL, SG_BULK, "FileRequest::responseHeadersComplete()"
        << " responseCode()=" << responseCode()
        << " responseReason()=" << responseReason()
        << " _append=" << _append
        );

    bool ok = false;
    if (responseCode() == 200) {
      ok = true;
    }
    else if (_append && (responseCode() == 206 || responseCode() == 416)) {
      /* See comments for simgear::HTTP::Client::setRange(). */
      SG_LOG(SG_IO, SG_DEBUG, "_append is true so treating response code as success: "
          << responseCode());
      ok = true;
    }
    if (!ok) {
      SG_LOG(SG_GENERAL, SG_DEBUG, "failure. calling setFailure()."
          << " responseCode()=" << responseCode()
          << " responseReason()=" << responseReason()
          );
      return setFailure(responseCode(), responseReason());
    }

    if( !_filename.isNull() )
    {
      // TODO validate path? (would require to expose fgValidatePath somehow to
      //      simgear)
      _filename.create_dir(0755);
      auto flags = std::ios::binary | std::ios::out | (_append ? std::ios::app : std::ios::trunc);
      SG_LOG(SG_GENERAL, SG_DEBUG, "attempting to open file. _append=" << _append);
      _file.open(_filename, flags);
    }

    if( !_file )
    {
      SG_LOG
      (
        SG_IO,
        SG_ALERT,
        "HTTP::FileRequest: failed to open file '" << _filename << "'"
      );
    }
  }

  //----------------------------------------------------------------------------
  void FileRequest::gotBodyData(const char* s, int n)
  {
    if( !_file )
    {
      SG_LOG
      (
        SG_IO,
        SG_DEBUG,
        "HTTP::FileRequest: received data for closed file '" << _filename << "'"
      );
      return;
    }

    _file.write(s, n);
    if (_callback) {
        _callback(s, n);
    }
  }

  //----------------------------------------------------------------------------
  void FileRequest::onAlways()
  {
    _file.close();
    if (_callback) {
        _callback(nullptr, 0);
    }
  }

} // namespace HTTP
} // namespace simgear
