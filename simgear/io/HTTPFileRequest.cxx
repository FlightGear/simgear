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
  FileRequest::FileRequest(const std::string& url, const std::string& path):
    Request(url, "GET"),
    _filename(path)
  {

  }

  //----------------------------------------------------------------------------
  void FileRequest::responseHeadersComplete()
  {
    Request::responseHeadersComplete();

    if( responseCode() != 200 )
      return setFailure(responseCode(), responseReason());

    if( !_filename.isNull() )
    {
      // TODO validate path? (would require to expose fgValidatePath somehow to
      //      simgear)
      _filename.create_dir(0755);

      _file.open(_filename, std::ios::binary | std::ios::trunc | std::ios::out);
    }

    if( !_file )
    {
      SG_LOG
      (
        SG_IO,
        SG_WARN,
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
  }

  //----------------------------------------------------------------------------
  void FileRequest::onAlways()
  {
    _file.close();
  }

} // namespace HTTP
} // namespace simgear
