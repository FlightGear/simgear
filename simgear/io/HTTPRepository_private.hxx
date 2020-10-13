// HTTPRepository.cxx -- plain HTTP TerraSync remote client
//
// Copyright (C) 20126  James Turner <zakalawe@mac.com>
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
// USA.

#pragma once

#include <memory>
#include <string>

#include <simgear/io/HTTPClient.hxx>
#include <simgear/misc/sg_path.hxx>

#include "HTTPRepository.hxx"

namespace simgear {

class HTTPDirectory;
using HTTPDirectory_ptr = std::unique_ptr<HTTPDirectory>;

class HTTPRepoGetRequest : public HTTP::Request {
public:
  HTTPRepoGetRequest(HTTPDirectory *d, const std::string &u)
      : HTTP::Request(u), _directory(d) {}

  virtual void cancel();

  size_t contentSize() const { return _contentSize; }

  void setContentSize(size_t sz) { _contentSize = sz; }

protected:
  HTTPDirectory *_directory;
  size_t _contentSize = 0;
};

typedef SGSharedPtr<HTTPRepoGetRequest> RepoRequestPtr;

class HTTPRepoPrivate {
public:
  struct HashCacheEntry {
    std::string filePath;
    time_t modTime;
    size_t lengthBytes;
    std::string hashHex;
  };

  typedef std::vector<HashCacheEntry> HashCache;
  HashCache hashes;
  int hashCacheDirty = 0;

  HTTPRepository::FailureVec failures;
  int maxPermittedFailures = 16;

  HTTPRepoPrivate(HTTPRepository *parent)
      : p(parent), isUpdating(false), status(HTTPRepository::REPO_NO_ERROR),
        totalDownloaded(0) {
    ;
  }

  ~HTTPRepoPrivate();

  HTTPRepository *p; // link back to outer
  HTTP::Client *http;
  std::string baseUrl;
  SGPath basePath;
  bool isUpdating;
  HTTPRepository::ResultCode status;
  HTTPDirectory_ptr rootDir;
  size_t totalDownloaded;
  HTTPRepository::SyncPredicate syncPredicate;

  HTTP::Request_ptr updateFile(HTTPDirectory *dir, const std::string &name,
                               size_t sz);
  HTTP::Request_ptr updateDir(HTTPDirectory *dir, const std::string &hash,
                              size_t sz);

  std::string hashForPath(const SGPath &p);
  void updatedFileContents(const SGPath &p, const std::string &newHash);
  void parseHashCache();
  std::string computeHashForPath(const SGPath &p);
  void writeHashCache();

  void failedToGetRootIndex(HTTPRepository::ResultCode st);
  void failedToUpdateChild(const SGPath &relativePath,
                           HTTPRepository::ResultCode fileStatus);

  void updatedChildSuccessfully(const SGPath &relativePath);

  typedef std::vector<RepoRequestPtr> RequestVector;
  RequestVector queuedRequests, activeRequests;

  void makeRequest(RepoRequestPtr req);

  enum class RequestFinish { Done, Retry };

  void finishedRequest(const RepoRequestPtr &req, RequestFinish retryRequest);

  HTTPDirectory *getOrCreateDirectory(const std::string &path);
  bool deleteDirectory(const std::string &relPath, const SGPath &absPath);

  typedef std::vector<HTTPDirectory_ptr> DirectoryVector;
  DirectoryVector directories;

  SGPath installedCopyPath;
};

} // namespace simgear
