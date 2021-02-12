// HTTPRepository.hxx - plain HTTP TerraSync remote server client
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


#ifndef SG_IO_HTTP_REPOSITORY_HXX
#define SG_IO_HTTP_REPOSITORY_HXX

#include <functional>
#include <memory>

#include <simgear/misc/sg_path.hxx>
#include <simgear/io/HTTPClient.hxx>

namespace simgear  {

class HTTPRepoPrivate;

class HTTPRepository
{
public:
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
    REPO_ERROR_CANCELLED,
    REPO_PARTIAL_UPDATE ///< repository is working, but file-level failures
                        ///< occurred
  };

  HTTPRepository(const SGPath &root, HTTP::Client *cl);
  virtual ~HTTPRepository();

  virtual SGPath fsBase() const;

  virtual void setBaseUrl(const std::string &url);
  virtual std::string baseUrl() const;

  virtual HTTP::Client *http() const;

  virtual void update();

  virtual bool isDoingSync() const;

  /**
        @brief call this periodically to progress non-network tasks
     */
  void process();

  virtual ResultCode failure() const;

  virtual size_t bytesToDownload() const;

  virtual size_t bytesDownloaded() const;

  virtual size_t bytesToExtract() const;

  /**
   * optionally provide the location of an installer copy of this
   * repository. When a file is missing it will be copied from this tree.
   */
  void setInstalledCopyPath(const SGPath &copyPath);

  static std::string resultCodeAsString(ResultCode code);

  enum class SyncAction { Add, Update, Delete, UpToDate };

  enum EntryType { FileType, DirectoryType, TarballType };

  struct SyncItem {
    const std::string directory; // relative path in the repository
    const EntryType type;
    const std::string filename;
    const SyncAction action;
    const SGPath pathOnDisk; // path the entry does / will have
  };

    using SyncPredicate = std::function<bool(const SyncItem &item)>;

    void setFilter(SyncPredicate sp);

    struct Failure {
      SGPath path;
      ResultCode error;
    };

    using FailureVec = std::vector<Failure>;

    /**
     * @brief return file-level failures
     */
    FailureVec failures() const;

  private:
    bool isBare() const;

    std::unique_ptr<HTTPRepoPrivate> _d;
};

} // of namespace simgear

#endif // of HTTPRepository
