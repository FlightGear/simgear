// AbstractRepository.hxx - API for terrasyc to access remote server
//
// Copyright (C) 2016  James Turner <zakalawe@mac.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


#ifndef SG_IO_ABSTRACT_REPOSITORY_HXX
#define SG_IO_ABSTRACT_REPOSITORY_HXX

#include <string>

#include <simgear/misc/sg_path.hxx>

namespace simgear  {

  namespace HTTP {
    class Client;
  }

class AbstractRepository
{
public:

    virtual ~AbstractRepository();

    virtual SGPath fsBase() const = 0;

    virtual void setBaseUrl(const std::string& url) =0;
    virtual std::string baseUrl() const = 0;;

    virtual HTTP::Client* http() const = 0;

    virtual void update() = 0;

    virtual bool isDoingSync() const = 0;

    enum ResultCode {
        REPO_NO_ERROR = 0,
        REPO_ERROR_NOT_FOUND,
        REPO_ERROR_SOCKET,
        SVN_ERROR_XML,
        SVN_ERROR_TXDELTA,
        REPO_ERROR_IO,
        REPO_ERROR_CHECKSUM,
        REPO_ERROR_FILE_NOT_FOUND,
        REPO_ERROR_HTTP,
        REPO_PARTIAL_UPDATE
    };

    virtual ResultCode failure() const = 0;
  protected:

};

} // of namespace simgear

#endif // of SG_IO_ABSTRACT_REPOSITORY_HXX
