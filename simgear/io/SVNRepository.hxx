// DAVMirrorTree.hxx - mirror a DAV tree to the local file system
//
// Copyright (C) 2012  James Turner <zakalawe@mac.com>
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


#ifndef SG_IO_DAVMIRRORTREE_HXX
#define SG_IO_DAVMIRRORTREE_HXX

#include <string>
#include <vector>
#include <memory>

#include <simgear/misc/sg_path.hxx>

namespace simgear  {
  
  namespace HTTP {
    class Client;
  }
  
class SVNDirectory;  
class SVNRepoPrivate;

class SVNRepository
{
public:
  
    SVNRepository(const SGPath& root, HTTP::Client* cl);
    ~SVNRepository();

    SVNDirectory* rootDir() const;
    SGPath fsBase() const;

    void setBaseUrl(const std::string& url);
    std::string baseUrl() const;

    HTTP::Client* http() const;

    void update();

    bool isDoingSync() const;
    
    enum ResultCode {
        SVN_NO_ERROR = 0,
        SVN_ERROR_NOT_FOUND,
        SVN_ERROR_SOCKET,
        SVN_ERROR_XML,
        SVN_ERROR_TXDELTA,
        SVN_ERROR_IO,
        SVN_ERROR_CHECKSUM,
        SVN_ERROR_FILE_NOT_FOUND,
        SVN_ERROR_HTTP
    };
    
    ResultCode failure() const;
private:
    bool isBare() const;
    
    std::auto_ptr<SVNRepoPrivate> _d;
};

} // of namespace simgear

#endif // of SG_IO_DAVMIRRORTREE_HXX
