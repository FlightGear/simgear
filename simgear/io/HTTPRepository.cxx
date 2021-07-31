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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <simgear_config.h>

#include "HTTPRepository.hxx"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>

#include <fcntl.h>

#include "simgear/debug/logstream.hxx"
#include "simgear/misc/strutils.hxx"

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/debug/Reporting.hxx>

#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/io/untar.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/timing/timestamp.hxx>

#include <simgear/misc/sg_hash.hxx>

#include "HTTPRepository_private.hxx"

namespace simgear
{

namespace {

std::string innerResultCodeAsString(HTTPRepository::ResultCode code) {
  switch (code) {
  case HTTPRepository::REPO_NO_ERROR:
    return "no error";
  case HTTPRepository::REPO_ERROR_NOT_FOUND:
    return "not found";
  case HTTPRepository::REPO_ERROR_SOCKET:
    return "socket error";
  case HTTPRepository::SVN_ERROR_XML:
    return "malformed XML";
  case HTTPRepository::SVN_ERROR_TXDELTA:
    return "malformed XML";
  case HTTPRepository::REPO_ERROR_IO:
    return "I/O error";
  case HTTPRepository::REPO_ERROR_CHECKSUM:
    return "checksum verification error";
  case HTTPRepository::REPO_ERROR_FILE_NOT_FOUND:
    return "file not found";
  case HTTPRepository::REPO_ERROR_HTTP:
    return "HTTP-level error";
  case HTTPRepository::REPO_ERROR_CANCELLED:
    return "cancelled";
  case HTTPRepository::REPO_PARTIAL_UPDATE:
    return "partial update (incomplete)";
  }

  return "Unknown response code";
}

struct HashCacheEntry {
    std::string filePath;
    time_t modTime;
    size_t lengthBytes;
    std::string hashHex;
};

using HashCache = std::unordered_map<std::string, HashCacheEntry>;

std::string computeHashForPath(const SGPath& p)
{
    if (!p.exists())
        return {};

    sha1nfo info;
    sha1_init(&info);

    const int bufSize = 1024 * 1024;
    char* buf = static_cast<char*>(malloc(bufSize));
    if (!buf) {
        sg_io_exception("Couldn't allocate SHA1 computation buffer");
    }

    size_t readLen;
    SGBinaryFile f(p);
    if (!f.open(SG_IO_IN)) {
        free(buf);
        throw sg_io_exception("Couldn't open file for compute hash", p);
    }
    while ((readLen = f.read(buf, bufSize)) > 0) {
        sha1_write(&info, buf, readLen);
    }

    f.close();
    free(buf);
    std::string hashBytes((char*)sha1_result(&info), HASH_LENGTH);
    return strutils::encodeHex(hashBytes);
}

} // namespace

class HTTPDirectory
{
    struct ChildInfo
    {
      ChildInfo(HTTPRepository::EntryType ty, const std::string &nameData,
                const std::string &hashData)
          : type(ty), name(nameData), hash(hashData) {}

        ChildInfo(const ChildInfo& other) = default;
        ChildInfo& operator=(const ChildInfo& other) = default;

      void setSize(const std::string &sizeData) {
        sizeInBytes = ::strtol(sizeData.c_str(), NULL, 10);
        }

        bool operator<(const ChildInfo& other) const
        {
            return name < other.name;
        }

        HTTPRepository::EntryType type;
        std::string name, hash;
        size_t sizeInBytes = 0;
        SGPath path; // absolute path on disk
    };

    typedef std::vector<ChildInfo> ChildInfoList;
    ChildInfoList children;

    mutable HashCache hashes;
    mutable bool hashCacheDirty = false;

public:
    HTTPDirectory(HTTPRepoPrivate* repo, const std::string& path) :
        _repository(repo),
        _relativePath(path)
  {
      assert(repo);

      SGPath p(absolutePath());
      if (p.exists()) {
          try {
              // already exists on disk
              parseDirIndex(children);
              std::sort(children.begin(), children.end());

              parseHashCache();
          } catch (sg_exception& ) {
              // parsing cache failed
              children.clear();
          }
      }
  }

    HTTPRepoPrivate* repository() const
    {
        return _repository;
    }

    std::string url() const
    {
      if (_relativePath.empty()) { // root directory of the repo
        return _repository->baseUrl;
      }

        return _repository->baseUrl + "/" + _relativePath;
    }

    void dirIndexUpdated(const std::string& hash)
    {
        SGPath fpath(absolutePath());
        fpath.append(".dirindex");
        updatedFileContents(fpath, hash);

        children.clear();
        parseDirIndex(children);
        std::sort(children.begin(), children.end());

        _repository->updatedChildSuccessfully(_relativePath);
    }

    void failedToUpdate(HTTPRepository::ResultCode status)
    {
        if (_relativePath.empty()) {
            // root dir failed
            _repository->failedToGetRootIndex(status);
        } else {
            _repository->failedToUpdateChild(_relativePath, status);
        }
    }

    void copyInstalledChildren()
    {
        if (_repository->installedCopyPath.isNull()) {
            return;
        }

        std::string buf;

        for (auto& child : children) {
            if (child.type != HTTPRepository::FileType)
                continue;

            if (child.path.exists())
                continue;

            SGPath cp = _repository->installedCopyPath;
            cp.append(relativePath());
            cp.append(child.name);
            if (!cp.exists()) {
                continue;
            }

            SGBinaryFile src(cp);
            SGBinaryFile dst(child.path);
            src.open(SG_IO_IN);
            dst.open(SG_IO_OUT);

            const size_t sizeToCopy = cp.sizeInBytes();
            if (buf.size() < sizeToCopy) {
                try {
                    simgear::ReportBadAllocGuard g;
                    buf.resize(sizeToCopy);
                } catch (std::bad_alloc&) {
                    simgear::reportFailure(simgear::LoadFailure::OutOfMemory, simgear::ErrorCode::TerraSync,
                                           "copyInstalledChildren: couldn't allocation copy buffer of size:" + std::to_string(sizeToCopy),
                                           child.path);
                    return;
                }
            }

            const size_t r = (size_t) src.read(buf.data(), sizeToCopy);
            if (r != sizeToCopy) {
                simgear::reportFailure(simgear::LoadFailure::IOError, simgear::ErrorCode::TerraSync,
                                       "copyInstalledChildren: read underflow, got:" + std::to_string(r),
                                       cp);
                return;
            }

            const size_t written = dst.write(buf.data(), sizeToCopy);
            if (written != sizeToCopy) {
                simgear::reportFailure(simgear::LoadFailure::IOError, simgear::ErrorCode::TerraSync,
                                       "copyInstalledChildren: write underflow, wrote:" + std::to_string(r),
                                       child.path);
                return;
            }

            src.close();
            dst.close();

            // reset caching
            child.path.set_cached(false);
            child.path.set_cached(true);

            std::string hash = computeHashForPath(child.path);
            updatedFileContents(child.path, hash);
        }
    }

    /// helper to check and erase 'fooBar' from paths, if passed fooBar.zip, fooBar.tgz, etc.
    void removeExtractedDirectoryFromList(PathList& paths, const std::string& tarballName)
    {
        const auto directoryName = SGPath::fromUtf8(tarballName).file_base();
        auto it = std::find_if(paths.begin(), paths.end(), [directoryName](const SGPath& p) {
            return p.isDir() && (p.file() == directoryName);
        });

        if (it != paths.end()) {
            paths.erase(it);
        }
    }

    void updateChildrenBasedOnHash()
    {
      using SAct = HTTPRepository::SyncAction;

      copyInstalledChildren();

      ChildInfoList toBeUpdated;

      simgear::Dir d(absolutePath());
      PathList fsChildren = d.children(0);
      PathList orphans = fsChildren;

      // on Windows, children() will return our .hashes and .dirIndex
      // entries; skip them.
      orphans.erase(std::remove_if(orphans.begin(), orphans.end(),
                                   [](const SGPath &p) {
                                     return p.file().front() == '.';
                                   }),
                    orphans.end());

      for (const auto &c : children) {
        // Check if the file exists
        auto p = std::find_if(fsChildren.begin(), fsChildren.end(),
                              LocalFileMatcher(c));

        const bool isNew = (p == fsChildren.end());
        const bool upToDate = hashForChild(c) == c.hash;

        if (!isNew) {
          orphans.erase(std::remove(orphans.begin(), orphans.end(), *p),
                        orphans.end());
        }

        // ensure the extracted directory corresponding to a tarball, is *not* considered an orphan
        if (c.type == HTTPRepository::TarballType) {
            removeExtractedDirectoryFromList(orphans, c.name);
        }

        if (_repository->syncPredicate) {
          const auto pathOnDisk = isNew ? absolutePath() / c.name : *p;
          // never handle deletes here, do them at the end
          const auto action =
              isNew ? SAct::Add : (upToDate ? SAct::UpToDate : SAct::Update);
          const HTTPRepository::SyncItem item = {relativePath(), c.type, c.name,
                                                 action, pathOnDisk};

          const bool doSync = _repository->syncPredicate(item);
          if (!doSync) {
            continue; // skip it, predicate filtered it out
          }
        }

        if (isNew) {
          // File or directory does not exist on local disk, so needs to be updated.
          toBeUpdated.push_back(c);
        } else if (!upToDate) {
          // File/directory exists, but hash doesn't match.
          toBeUpdated.push_back(c);
        } else {
          // File/Directory exists and hash is valid.
          if (c.type == HTTPRepository::DirectoryType) {
            // If it's a directory,perform a recursive check.
            HTTPDirectory *childDir = childDirectory(c.name);
            _repository->scheduleUpdateOfChildren(childDir);
          }
        }
      } // of repository-defined (well, .dirIndex) children iteration

      // allow the filtering of orphans; this is important so that a filter
      // can be used to preserve non-repo files in a directory,
      // i.e somewhat like a .gitignore
      if (!orphans.empty() && _repository->syncPredicate) {
        const auto ourPath = relativePath();
        const auto pred = _repository->syncPredicate;

        auto l = [ourPath, pred](const SGPath &o) {
          // this doesn't special-case for tarballs (they will be reported as a
          // file) I think that's okay, since a filter can see the full path
          const auto type = o.isDir() ? HTTPRepository::DirectoryType
                                      : HTTPRepository::FileType;

          const HTTPRepository::SyncItem item = {ourPath, type, o.file(),
                                                 SAct::Delete, o};

          const bool r = pred(item);
          // clarification: the predicate returns true if the file should be
          // handled as normal, false if it should be skipped. But since we're
          // inside a remove_if, we want to remove *skipped* files from orphans,
          // so they don't get deleted. So we want to return true here, if the
          // file should be skipped.
          return (r == false);
        };

        auto it = std::remove_if(orphans.begin(), orphans.end(), l);
        orphans.erase(it, orphans.end());
      }

      // We now have a list of entries that need to be updated, and a list
      // of orphan files that should be removed.
      try {
          removeOrphans(orphans);
      } catch (sg_exception& e) {
          _repository->failedToUpdateChild(_relativePath, HTTPRepository::ResultCode::REPO_ERROR_IO);
      }

      scheduleUpdates(toBeUpdated);
    }

    HTTPDirectory* childDirectory(const std::string& name)
    {
        std::string childPath = relativePath().empty() ? name : relativePath() + "/" + name;
        return _repository->getOrCreateDirectory(childPath);
    }

    void removeOrphans(const PathList orphans)
    {
        for (const auto& o : orphans) {
            if (o.file() == ".dirindex") continue;
            if (o.file() == ".hash") continue;
            removeChild(o);
        }
    }

    string_list indexChildren() const
    {
        string_list r;
        r.reserve(children.size());
        ChildInfoList::const_iterator it;
        for (it=children.begin(); it != children.end(); ++it) {
            r.push_back(it->name);
        }
        return r;
    }

    void scheduleUpdates(const ChildInfoList names)
    {
        ChildInfoList::const_iterator it;
        for (it = names.begin(); it != names.end(); ++it) {
          if (it->type == HTTPRepository::FileType) {
            _repository->updateFile(this, it->name, it->sizeInBytes);
          } else if (it->type == HTTPRepository::DirectoryType) {
            HTTPDirectory *childDir = childDirectory(it->name);
            _repository->updateDir(childDir, it->hash, it->sizeInBytes);
          } else if (it->type == HTTPRepository::TarballType) {
            // Download a tarball just as a file.
            _repository->updateFile(this, it->name, it->sizeInBytes);
          } else {
            SG_LOG(SG_TERRASYNC, SG_ALERT,
                   "Coding error!  Unknown Child type to schedule update");
          }
        }
    }

    SGPath absolutePath() const
    {
        SGPath r(_repository->basePath);
        r.append(_relativePath);
        return r;
    }

    std::string relativePath() const
    {
        return _relativePath;
    }

    class ArchiveExtractTask {
    public:
      ArchiveExtractTask(SGPath p, const std::string &relPath)
          : relativePath(relPath), file(p), extractor(p.dir()) {
        if (!file.open(SG_IO_IN)) {
          SG_LOG(SG_TERRASYNC, SG_ALERT,
                 "Unable to open " << p << " to extract");
          return;
        }

        compressedBytes = p.sizeInBytes();
        buffer = (uint8_t *)malloc(bufferSize);
      }

      ArchiveExtractTask(const ArchiveExtractTask &) = delete;

      HTTPRepoPrivate::ProcessResult run(HTTPRepoPrivate* repo)
      {
          if (!buffer) {
              return HTTPRepoPrivate::ProcessFailed;
          }

          size_t rd = file.read((char*)buffer, bufferSize);
          repo->bytesExtracted += rd;
          extractor.extractBytes(buffer, rd);

          if (file.eof()) {
              extractor.flush();
              file.close();

              if (!extractor.isAtEndOfArchive()) {
                  SG_LOG(SG_TERRASYNC, SG_ALERT, "Corrupt tarball " << relativePath);
                  repo->failedToUpdateChild(relativePath,
                                            HTTPRepository::REPO_ERROR_IO);
                  return HTTPRepoPrivate::ProcessFailed;
              }

              if (extractor.hasError()) {
                  SG_LOG(SG_TERRASYNC, SG_ALERT, "Error extracting " << relativePath);
                  repo->failedToUpdateChild(relativePath,
                                            HTTPRepository::REPO_ERROR_IO);
                  return HTTPRepoPrivate::ProcessFailed;
              }

              return HTTPRepoPrivate::ProcessDone;
          }

          return HTTPRepoPrivate::ProcessContinue;
      }

      size_t archiveSizeBytes() const
      {
          return compressedBytes;
      }

      ~ArchiveExtractTask() { free(buffer); }


    private:
      // intentionally small so we extract incrementally on Windows
      // where Defender throttles many small files, sorry
      // if you make this bigger we will be more efficient but stall for
      // longer when extracting the Airports_archive
      const int bufferSize = 1024 * 64;

      std::string relativePath;
      uint8_t *buffer = nullptr;
      SGBinaryFile file;
      ArchiveExtractor extractor;
      std::size_t compressedBytes;
    };

    using ArchiveExtractTaskPtr = std::shared_ptr<ArchiveExtractTask>;

    void didUpdateFile(const std::string& file, const std::string& hash, size_t sz)
    {
        // check hash matches what we expected
        auto it = findIndexChild(file);
        if (it == children.end()) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "updated file but not found in dir:" << _relativePath << " " << file);
        } else {
            if (it->hash != hash) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "Checksum error for " << absolutePath() << "/" << file << " " << it->hash << " " << hash);
                // we don't erase the file on a hash mismatch, because if we're syncing during the
                // middle of a server-side update, the downloaded file may actually become valid.
                _repository->failedToUpdateChild(
                    _relativePath + "/" + file,
                    HTTPRepository::REPO_ERROR_CHECKSUM);
            } else {
                updatedFileContents(it->path, hash);
                _repository->updatedChildSuccessfully(_relativePath + "/" +
                                                      file);

                _repository->totalDownloaded += sz;
                SGPath p = SGPath(absolutePath(), file);

                if (it->type == HTTPRepository::TarballType) {
                    // We require that any compressed files have the same filename as the file or directory
                    // they expand to, so we can remove the old file/directory before extracting the new
                    // data.
                    SGPath removePath = SGPath(p.base());
                    bool pathAvailable = true;
                    if (removePath.exists()) {
                        if (removePath.isDir()) {
                            simgear::Dir pd(removePath);
                            pathAvailable = pd.removeChildren();
                        } else {
                            pathAvailable = removePath.remove();
                        }
                    }

                    if (pathAvailable) {
                        // we use a Task helper to extract tarballs incrementally.
                        // without this, archive extraction blocks here, which
                        // prevents other repositories downloading / updating.
                        // Unfortunately due Windows AV (Defender, etc) we cna block
                        // here for many minutes.

                        // use a lambda to own this shared_ptr; this means when the
                        // lambda is destroyed, the ArchiveExtraTask will get
                        // cleaned up.
                        ArchiveExtractTaskPtr t =
                            std::make_shared<ArchiveExtractTask>(p, _relativePath);
                        auto cb = [t](HTTPRepoPrivate* repo) {
                            return t->run(repo);
                        };
                        _repository->bytesToExtract += t->archiveSizeBytes();
                        _repository->addTask(cb);
                    } else {
                        SG_LOG(SG_TERRASYNC, SG_ALERT, "Unable to remove old file/directory " << removePath);
                    } // of pathAvailable
                }     // of handling archive files
            } // of hash matches
        } // of found in child list
    }

    void didFailToUpdateFile(const std::string& file,
                             HTTPRepository::ResultCode status)
    {
        SGPath fpath(_relativePath);
        fpath.append(file);
        _repository->failedToUpdateChild(fpath, status);
    }

    std::string hashForPath(const SGPath& p) const
    {
        const auto ps = p.utf8Str();
        auto it = hashes.find(ps);
        if (it != hashes.end()) {
            const auto& entry = it->second;
            // ensure data on disk hasn't changed.
            // we could also use the file type here if we were paranoid
            if ((p.sizeInBytes() == entry.lengthBytes) && (p.modTime() == entry.modTime)) {
                return entry.hashHex;
            }

            // entry in the cache, but it's stale so remove and fall through
            hashes.erase(it);
        }

        std::string hash = computeHashForPath(p);
        updatedFileContents(p, hash);
        return hash;
    }

    bool isHashCacheDirty() const
    {
        return hashCacheDirty;
    }

    void writeHashCache() const
    {
        if (!hashCacheDirty)
            return;

        hashCacheDirty = false;

        SGPath cachePath = absolutePath() / ".dirhash";
        sg_ofstream stream(cachePath, std::ios::out | std::ios::trunc | std::ios::binary);
        for (const auto& e : hashes) {
            const auto& entry = e.second;
            stream << entry.filePath << "*" << entry.modTime << "*"
                   << entry.lengthBytes << "*" << entry.hashHex << "\n";
        }
        stream.close();
    }

private:

    struct ChildWithName
    {
        ChildWithName(const std::string& n) : name(n) {}
        std::string name;

        bool operator()(const ChildInfo& info) const
        { return info.name == name; }
    };

    struct LocalFileMatcher
    {
        LocalFileMatcher(const ChildInfo ci) : childInfo(ci) {}
        ChildInfo childInfo;

        bool operator()(const SGPath path) const {
          return path.file() == childInfo.name;
        }
    };

    ChildInfoList::iterator findIndexChild(const std::string& name)
    {
        return std::find_if(children.begin(), children.end(), ChildWithName(name));
    }

    bool parseDirIndex(ChildInfoList& children)
    {
        SGPath p(absolutePath());
        p.append(".dirindex");
        if (!p.exists()) {
            return false;
        }

        sg_ifstream indexStream(p, std::ios::in );

        if ( !indexStream.is_open() ) {
            throw sg_io_exception("cannot open dirIndex file", p);
        }

        while (!indexStream.eof() ) {
            std::string line;
            std::getline( indexStream, line );
            line = simgear::strutils::strip(line);

            // skip blank line or comment beginning with '#'
            if( line.empty() || line[0] == '#' )
                continue;

            string_list tokens = simgear::strutils::split( line, ":" );

            std::string typeData = tokens[0];

            if( typeData == "version" ) {
                if( tokens.size() < 2 ) {
                    SG_LOG(SG_TERRASYNC, SG_WARN, "malformed .dirindex file: missing version number in line '" << line << "'"
                           << "\n\tparsing:" << p.utf8Str());
                    break;
                }
                if( tokens[1] != "1" ) {
                    SG_LOG(SG_TERRASYNC, SG_WARN, "invalid .dirindex file: wrong version number '" << tokens[1] << "' (expected 1)"
                           << "\n\tparsing:" << p.utf8Str());
                    break;
                }
                continue; // version is good, continue
            }

            if( typeData == "path" ) {
                continue; // ignore path, next line
            }

            if( typeData == "time" && tokens.size() > 1 ) {
               // SG_LOG(SG_TERRASYNC, SG_INFO, ".dirindex at '" << p.str() << "' timestamp: " << tokens[1] );
                continue;
            }

            if( tokens.size() < 3 ) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "malformed .dirindex file: not enough tokens in line '" << line << "' (ignoring line)"
                       << "\n\tparsing:" << p.utf8Str());
                continue;
            }

            if (typeData != "f" && typeData != "d"  && typeData != "t" ) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "malformed .dirindex file: invalid type in line '" << line << "', expected 't', 'd' or 'f', (ignoring line)"
                       << "\n\tparsing:" << p.utf8Str());
                continue;
            }

            // security: prevent writing outside the repository via ../../.. filenames
            // (valid filenames never contain / - subdirectories have their own .dirindex)
            if ((tokens[1] == "..") || (tokens[1].find_first_of("/\\") != std::string::npos)) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "malformed .dirindex file: invalid filename in line '" << line << "', (ignoring line)"
                       << "\n\tparsing:" << p.utf8Str());
                continue;
            }

            ChildInfo ci =
                ChildInfo(HTTPRepository::FileType, tokens[1], tokens[2]);
            if (typeData == "d")
              ci.type = HTTPRepository::DirectoryType;
            if (typeData == "t")
              ci.type = HTTPRepository::TarballType;

            children.emplace_back(ci);
            children.back().path = absolutePath() / tokens[1];
            if (tokens.size() > 3) {
                children.back().setSize(tokens[3]);
            }
        }

        return true;
    }

    void removeChild(SGPath path)
    {
        bool ok;
        SG_LOG(SG_TERRASYNC, SG_INFO, "Removing:" << path);

        std::string fpath = _relativePath + "/" + path.file();
        if (path.isDir()) {
            ok = _repository->deleteDirectory(fpath, path);
        } else {
            // remove the hash cache entry
            updatedFileContents(path, std::string());
            ok = path.remove();
        }

        if (!ok) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "removal failed for:" << path);
            throw sg_io_exception("Failed to remove existing file/dir:", path, _repository->basePath.utf8Str(), false);
        }
    }

    std::string hashForChild(const ChildInfo& child) const
    {
      SGPath p(child.path);
      if (child.type == HTTPRepository::DirectoryType) {
          p.append(".dirindex");
      }
      return hashForPath(p);
    }

    void parseHashCache()
    {
        hashes.clear();
        SGPath cachePath = absolutePath() / ".dirhash";
        if (!cachePath.exists()) {
            return;
        }

        sg_ifstream stream(cachePath, std::ios::in);

        while (!stream.eof()) {
            std::string line;
            std::getline(stream, line);
            line = simgear::strutils::strip(line);
            if (line.empty() || line[0] == '#')
                continue;

            string_list tokens = simgear::strutils::split(line, "*");
            if (tokens.size() < 4) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "invalid entry in '" << cachePath << "': '" << line << "' (ignoring line)");
                continue;
            }
            const std::string nameData = simgear::strutils::strip(tokens[0]);
            const std::string timeData = simgear::strutils::strip(tokens[1]);
            const std::string sizeData = simgear::strutils::strip(tokens[2]);
            const std::string hashData = simgear::strutils::strip(tokens[3]);

            if (nameData.empty() || timeData.empty() || sizeData.empty() || hashData.empty()) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "invalid entry in '" << cachePath << "': '" << line << "' (ignoring line)");
                continue;
            }

            HashCacheEntry entry;
            entry.filePath = nameData;
            entry.hashHex = hashData;
            entry.modTime = strtol(timeData.c_str(), NULL, 10);
            entry.lengthBytes = strtol(sizeData.c_str(), NULL, 10);
            hashes.insert(std::make_pair(entry.filePath, entry));
        }
    }

    void updatedFileContents(const SGPath& p, const std::string& newHash) const
    {
        // remove the existing entry
        const auto ps = p.utf8Str();
        auto it = hashes.find(ps);
        if (it != hashes.end()) {
            hashes.erase(it);
            hashCacheDirty = true;
        }

        if (newHash.empty()) {
            return; // we're done
        }

        // use a cloned SGPath and reset its caching to force one stat() call
        SGPath p2(p);
        p2.set_cached(false);
        p2.set_cached(true);

        HashCacheEntry entry;
        entry.filePath = ps;
        entry.hashHex = newHash;
        entry.modTime = p2.modTime();
        entry.lengthBytes = p2.sizeInBytes();
        hashes.insert(std::make_pair(ps, entry));

        hashCacheDirty = true;
    }


    HTTPRepoPrivate* _repository;
    std::string _relativePath; // in URL and file-system space
};

HTTPRepository::HTTPRepository(const SGPath& base, HTTP::Client *cl) :
    _d(new HTTPRepoPrivate(this))
{
    _d->http = cl;
    _d->basePath = base;
    _d->rootDir.reset(new HTTPDirectory(_d.get(), ""));
}

HTTPRepository::~HTTPRepository()
{
}

void HTTPRepository::setBaseUrl(const std::string &url)
{
  _d->baseUrl = url;
}

std::string HTTPRepository::baseUrl() const
{
  return _d->baseUrl;
}

HTTP::Client* HTTPRepository::http() const
{
  return _d->http;
}

SGPath HTTPRepository::fsBase() const
{
  return SGPath();
}

void HTTPRepository::update()
{
    if (_d->isUpdating) {
        return;
    }

    _d->status = REPO_NO_ERROR;
    _d->isUpdating = true;
    _d->failures.clear();
    _d->updateDir(_d->rootDir.get(), {}, 0);
}

bool HTTPRepository::isDoingSync() const
{
    if (_d->status != REPO_NO_ERROR) {
        return false;
    }

    return _d->isUpdating;
}

void HTTPRepository::process()
{
    int processedCount = 0;
    const int maxToProcess = 16;

    while (processedCount < maxToProcess) {
      if (_d->pendingTasks.empty()) {
        break;
      }

      auto task = _d->pendingTasks.front();
      auto result = task(_d.get());
      if (result == HTTPRepoPrivate::ProcessContinue) {
        // assume we're not complete
        return;
      }

      _d->pendingTasks.pop_front();
      ++processedCount;
    }

    _d->checkForComplete();
}

void HTTPRepoPrivate::checkForComplete()
{
  if (pendingTasks.empty() && activeRequests.empty() &&
      queuedRequests.empty()) {
    isUpdating = false;
  }
}

size_t HTTPRepository::bytesToDownload() const
{
    size_t result = 0;

    HTTPRepoPrivate::RequestVector::const_iterator r;
    for (r = _d->queuedRequests.begin(); r != _d->queuedRequests.end(); ++r) {
        result += (*r)->contentSize();
    }

    for (r = _d->activeRequests.begin(); r != _d->activeRequests.end(); ++r) {
        if ((*r)->contentSize() > 0) {
            // Content size for root dirindex of a repository is zero,
            // and returing a negative value breaks everyting, so just ignore
            // it
            result += (*r)->contentSize() - (*r)->responseBytesReceived();
        }
    }

    return result;
}

size_t HTTPRepository::bytesDownloaded() const
{
    size_t result = _d->totalDownloaded;

    HTTPRepoPrivate::RequestVector::const_iterator r;
    for (r = _d->activeRequests.begin(); r != _d->activeRequests.end(); ++r) {
        result += (*r)->responseBytesReceived();
    }

    return result;
}

size_t HTTPRepository::bytesToExtract() const
{
    return _d->bytesToExtract - _d->bytesExtracted;
}

void HTTPRepository::setInstalledCopyPath(const SGPath& copyPath)
{
    _d->installedCopyPath = copyPath;
}

std::string HTTPRepository::resultCodeAsString(ResultCode code)
{
    return innerResultCodeAsString(code);
}

HTTPRepository::FailureVec HTTPRepository::failures() const {
  return _d->failures;
}

void HTTPRepository::setFilter(SyncPredicate sp) { _d->syncPredicate = sp; }

HTTPRepository::ResultCode
HTTPRepository::failure() const
{
    if ((_d->status == REPO_NO_ERROR) && !_d->failures.empty()) {
        return REPO_PARTIAL_UPDATE;
    }

    return _d->status;
}

    void HTTPRepoGetRequest::cancel()
    {
        _directory->repository()->http->cancelRequest(this, "Repository cancelled");
        _directory = 0;
    }

    class FileGetRequest : public HTTPRepoGetRequest
    {
    public:
        FileGetRequest(HTTPDirectory* d, const std::string& file) :
            HTTPRepoGetRequest(d, makeUrl(d, file)),
            fileName(file)
        {
            pathInRepo = _directory->absolutePath();
            pathInRepo.append(fileName);
        }

    protected:
        void gotBodyData(const char* s, int n) override
        {
            if (!file.get()) {
                const bool ok = createOutputFile();
                if (!ok) {
                    ioFailureOccurred = true;
                    _directory->repository()->http->cancelRequest(
                        this, "Unable to create output file:" + pathInRepo.utf8Str());
                }
            }

            sha1_write(&hashContext, s, n);
            const auto written = file->write(s, n);
            if (written != n) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "Underflow writing to " << pathInRepo);
                ioFailureOccurred = true;
                _directory->repository()->http->cancelRequest(
                    this, "Unable to write to output file:" + pathInRepo.utf8Str());
            }
        }

        bool createOutputFile()
        {
            file.reset(new SGBinaryFile(pathInRepo));
            if (!file->open(SG_IO_OUT)) {
                SG_LOG(SG_TERRASYNC, SG_WARN,
                       "unable to create file " << pathInRepo);
                return false;
            }

            sha1_init(&hashContext);
            return true;
        }

        void onDone() override
        {
            const bool is200Response = (responseCode() == 200);
            if (!file && is200Response) {
                // if the server defines a zero-byte file, we will never call
                // gotBodyData, so create the file here
                // this ensures all the logic below works as expected
                createOutputFile();
            }

            if (file) {
                file->close();
            }

            if (is200Response) {
                std::string hash =
                    strutils::encodeHex(sha1_result(&hashContext), HASH_LENGTH);
                _directory->didUpdateFile(fileName, hash, contentSize());
            } else if (responseCode() == 404) {
                SG_LOG(SG_TERRASYNC, SG_WARN,
                       "terrasync file not found on server: "
                           << fileName << " for " << _directory->absolutePath());
                _directory->didFailToUpdateFile(
                    fileName, HTTPRepository::REPO_ERROR_FILE_NOT_FOUND);
            } else {
                SG_LOG(SG_TERRASYNC, SG_WARN,
                       "terrasync file download error on server: "
                           << fileName << " for " << _directory->absolutePath()
                           << "\n\tserver responded: " << responseCode() << "/"
                           << responseReason());
                _directory->didFailToUpdateFile(fileName,
                                                HTTPRepository::REPO_ERROR_HTTP);
                // should we every retry here?
            }

            _directory->repository()->finishedRequest(
                this, HTTPRepoPrivate::RequestFinish::Done);
        }

      void onFail() override {
        HTTPRepository::ResultCode code = HTTPRepository::REPO_ERROR_SOCKET;

        // -1 means request cancelled locally
        if (responseCode() == -1) {
            if (ioFailureOccurred) {
                // cancelled by code above due to IO error
                code = HTTPRepository::REPO_ERROR_IO;
            } else {
                code = HTTPRepository::REPO_ERROR_CANCELLED;
            }
        }

        if (file) {
          file->close();
        }

        file.reset();
        if (pathInRepo.exists()) {
          pathInRepo.remove();
        }

        if (_directory) {
          _directory->didFailToUpdateFile(fileName, code);

          const auto doRetry = code == HTTPRepository::REPO_ERROR_SOCKET
                                   ? HTTPRepoPrivate::RequestFinish::Retry
                                   : HTTPRepoPrivate::RequestFinish::Done;
          _directory->repository()->finishedRequest(this, doRetry);
        }
      }

      void prepareForRetry() override {
        HTTP::Request::prepareForRetry();
        file.reset();
      }

    private:
        static std::string makeUrl(HTTPDirectory* d, const std::string& file)
        {
            return d->url() + "/" + file;
        }

        std::string fileName; // if empty, we're getting the directory itself
        SGPath pathInRepo;
        simgear::sha1nfo hashContext;
        std::unique_ptr<SGBinaryFile> file;

        /// becuase we cancel() in the case of an IO failure, we need to a way to distuinguish
        /// user initated cancellation and IO-failure cancellation in onFail. This flag lets us do that
        bool ioFailureOccurred = false;
    };

    class DirGetRequest : public HTTPRepoGetRequest
    {
    public:
        DirGetRequest(HTTPDirectory* d, const std::string& targetHash) :
            HTTPRepoGetRequest(d, makeUrl(d)),
            _isRootDir(false),
            _targetHash(targetHash)
        {
            sha1_init(&hashContext);
        }

        void setIsRootDir()
        {
            _isRootDir = true;
        }

        bool isRootDir() const
        {
            return _isRootDir;
        }

        void prepareForRetry() override {
          body.clear();
          sha1_init(&hashContext);
          HTTP::Request::prepareForRetry();
        }

      protected:
        void gotBodyData(const char *s, int n) override {
          body += std::string(s, n);
          sha1_write(&hashContext, s, n);
        }

        void onDone() override {
          SG_LOG(SG_TERRASYNC, SG_DEBUG, "onDone(): url()=" << url() << " _directory=" << _directory
                << " responseCode()=" << responseCode());
          if (responseCode() == 200) {
            std::string hash =
                strutils::encodeHex(sha1_result(&hashContext), HASH_LENGTH);
            if (!_targetHash.empty() && (hash != _targetHash)) {
              SG_LOG(SG_TERRASYNC, SG_ALERT,
                     "Checksum error getting dirIndex for:"
                         << _directory->relativePath() << "; expected "
                         << _targetHash << " but received " << hash
                         << " url()=" << url()
                         );

              _directory->failedToUpdate(HTTPRepository::REPO_ERROR_CHECKSUM);

              // don't retry checkums failures
              _directory->repository()->finishedRequest(
                  this, HTTPRepoPrivate::RequestFinish::Done);
              return;
            }

            std::string curHash = _directory->hashForPath(path());
            if (hash != curHash) {
              simgear::Dir d(_directory->absolutePath());
              if (!d.exists()) {
                if (!d.create(0700)) {
                  throw sg_io_exception("Unable to create directory", d.path());
                }
              }

              // dir index data has changed, so write to disk and update
              // the hash accordingly
              sg_ofstream of(pathInRepo(), std::ios::trunc | std::ios::out |
                                               std::ios::binary);
              if (!of.is_open()) {
                throw sg_io_exception(
                    "Failed to open directory index file for writing",
                    pathInRepo());
              }

              of.write(body.data(), body.size());
              of.close();
              _directory->dirIndexUpdated(hash);

              SG_LOG(SG_TERRASYNC, SG_DEBUG, "from url()=" << url() << " have updated _directory: " << _directory);
              //SG_LOG(SG_TERRASYNC, SG_INFO, "updated dir index " << _directory->absolutePath());
            }

            _directory->repository()->totalDownloaded += contentSize();

            try {
              // either way we've confirmed the index is valid so update
              // children now
              SGTimeStamp st;
              st.stamp();
              _directory->updateChildrenBasedOnHash();
              SG_LOG(SG_TERRASYNC, SG_DEBUG,
                     "after update of:" << _directory->absolutePath()
                                        << " child update took:"
                                        << st.elapsedMSec());
            } catch (sg_exception &) {
              _directory->failedToUpdate(HTTPRepository::REPO_ERROR_IO);
            }
          } else if (responseCode() == 404) {
            _directory->failedToUpdate(
                HTTPRepository::REPO_ERROR_FILE_NOT_FOUND);
          } else {
            _directory->failedToUpdate(HTTPRepository::REPO_ERROR_HTTP);
          }

          _directory->repository()->finishedRequest(
              this, HTTPRepoPrivate::RequestFinish::Done);
        }

        void onFail() override {
          SG_LOG(SG_TERRASYNC, SG_ALERT, "onFail(): url()=" << url() << " _directory=" << _directory
                << " responseCode()=" << responseCode());
          HTTPRepository::ResultCode code = HTTPRepository::REPO_ERROR_SOCKET;
          if (responseCode() == -1) {
            code = HTTPRepository::REPO_ERROR_CANCELLED;
          }

          SG_LOG(SG_TERRASYNC, SG_WARN,
                 "Socket failure getting directory: " << url());
          if (_directory) {
            _directory->failedToUpdate(code);
            const auto doRetry = code == HTTPRepository::REPO_ERROR_SOCKET
                                     ? HTTPRepoPrivate::RequestFinish::Retry
                                     : HTTPRepoPrivate::RequestFinish::Done;
            _directory->repository()->finishedRequest(this, doRetry);
          }
        }

    private:
        static std::string makeUrl(HTTPDirectory* d)
        {
            return d->url() + "/.dirindex";
        }

        SGPath pathInRepo() const
        {
            SGPath p(_directory->absolutePath());
            p.append(".dirindex");
            return p;
        }

        simgear::sha1nfo hashContext;
        std::string body;
        bool _isRootDir; ///< is this the repository root?
        std::string _targetHash;
    };

    HTTPRepoPrivate::~HTTPRepoPrivate()
    {
        // take a copy since cancelRequest will fail and hence remove
        // remove activeRequests, invalidating any iterator to it.
        RequestVector copyOfActive(activeRequests);
        RequestVector::iterator rq;
        for (rq = copyOfActive.begin(); rq != copyOfActive.end(); ++rq) {
            http->cancelRequest(*rq, "Repository object deleted");
        }

        flushHashCaches();
        directories.clear(); // wil delete them all
    }

    HTTP::Request_ptr HTTPRepoPrivate::updateFile(HTTPDirectory* dir, const std::string& name, size_t sz)
    {
        RepoRequestPtr r(new FileGetRequest(dir, name));
        r->setContentSize(sz);
        makeRequest(r);
        return r;
    }

    HTTP::Request_ptr HTTPRepoPrivate::updateDir(HTTPDirectory* dir, const std::string& hash, size_t sz)
    {
        RepoRequestPtr r(new DirGetRequest(dir, hash));
        r->setContentSize(sz);
        makeRequest(r);
        return r;
    }

    class DirectoryWithPath
    {
    public:
        DirectoryWithPath(const std::string& p) : path(p) {}
        bool operator()(const HTTPDirectory_ptr& entry) const
        { return entry->relativePath() == path; }
    private:
        std::string path;
    };

    HTTPDirectory* HTTPRepoPrivate::getOrCreateDirectory(const std::string& path)
    {
        DirectoryWithPath p(path);
        auto it = std::find_if(directories.begin(), directories.end(), p);
        if (it != directories.end()) {
            return it->get();
        }

        directories.emplace_back(new HTTPDirectory(this, path));
        return directories.back().get();
    }

    bool HTTPRepoPrivate::deleteDirectory(const std::string& relPath, const SGPath& absPath)
    {
        DirectoryWithPath p(relPath);
        auto it = std::find_if(directories.begin(), directories.end(), p);
        if (it != directories.end()) {
          assert((*it)->absolutePath() == absPath);
          directories.erase(it);
        } else {
            // we encounter this code path when deleting an orphaned directory
        }

        Dir dir(absPath);
        bool result = dir.remove(true);
        return result;
    }

    void HTTPRepoPrivate::makeRequest(RepoRequestPtr req)
    {
        if (activeRequests.size() > 4) {
          queuedRequests.push_back(req);
        } else {
            activeRequests.push_back(req);
            http->makeRequest(req);
        }
    }

    void HTTPRepoPrivate::finishedRequest(const RepoRequestPtr &req,
                                          RequestFinish retryRequest) {
      auto it = std::find(activeRequests.begin(), activeRequests.end(), req);
      // in some cases, we clear the active
      // and queued request vectors, so the ::find above can fail
      if (it != activeRequests.end()) {
        activeRequests.erase(it);
      }

      if (retryRequest == HTTPRepoPrivate::RequestFinish::Retry) {
        SG_LOG(SG_TERRASYNC, SG_INFO, "Retrying request for:" << req->url());
        req->prepareForRetry();
        queuedRequests.push_back(req);
      }

      if (!queuedRequests.empty()) {
        RepoRequestPtr rr = queuedRequests.front();
        queuedRequests.erase(queuedRequests.begin());
        activeRequests.push_back(rr);
        http->makeRequest(rr);
      }

      if (countDirtyHashCaches() > 32) {
          flushHashCaches();
      }
      checkForComplete();
    }

    void HTTPRepoPrivate::failedToGetRootIndex(HTTPRepository::ResultCode st)
    {
        if (st == HTTPRepository::REPO_ERROR_FILE_NOT_FOUND) {
            status = HTTPRepository::REPO_ERROR_NOT_FOUND;
        } else {
            simgear::reportFailure(simgear::LoadFailure::NetworkError, simgear::ErrorCode::TerraSync,
                                   "failed to get TerraSync repository root:" + innerResultCodeAsString(st),
                                   sg_location{baseUrl});
            SG_LOG(SG_TERRASYNC, SG_WARN, "Failed to get root of repo:" << baseUrl << " " << st);
            status = st;
        }
    }

    void HTTPRepoPrivate::failedToUpdateChild(const SGPath& relativePath,
                                              HTTPRepository::ResultCode fileStatus)
    {
      if (fileStatus == HTTPRepository::REPO_ERROR_CANCELLED) {
        // if we were cancelled, don't report or log
        return;
      } else {
        SG_LOG(SG_TERRASYNC, SG_WARN,
               "failed to update entry:" << relativePath << " status/code: "
                                         << innerResultCodeAsString(fileStatus)
                                         << "/" << fileStatus);

        simgear::reportFailure(simgear::LoadFailure::NetworkError, simgear::ErrorCode::TerraSync,
                               "failed to update entry:" + innerResultCodeAsString(fileStatus),
                               sg_location{relativePath});
      }

      HTTPRepository::Failure f;
      f.path = relativePath;
      f.error = fileStatus;
      failures.push_back(f);

      if (failures.size() >= maxPermittedFailures) {
        SG_LOG(SG_TERRASYNC, SG_WARN,
               "Repo:" << baseUrl << " exceeded failure count ("
                       << failures.size() << "), abandoning");

        status = HTTPRepository::REPO_PARTIAL_UPDATE;

        queuedRequests.clear();
        auto copyOfActiveRequests = activeRequests;
        for (auto rq : copyOfActiveRequests) {
          http->cancelRequest(rq,
                              "Abandoning repo sync due to multiple failures");
        }
      }
    }

    void HTTPRepoPrivate::updatedChildSuccessfully(const SGPath &relativePath) {
      if (failures.empty()) {
        return;
      }

      // find and remove any existing failures for that path
      failures.erase(
          std::remove_if(failures.begin(), failures.end(),
                         [relativePath](const HTTPRepository::Failure &f) {
                           return f.path == relativePath;
                         }),
          failures.end());
    }

    void HTTPRepoPrivate::scheduleUpdateOfChildren(HTTPDirectory* dir)
    {
      auto updateChildTask = [dir](const HTTPRepoPrivate *) {
        dir->updateChildrenBasedOnHash();
        return ProcessDone;
      };

      addTask(updateChildTask);
    }

    void HTTPRepoPrivate::addTask(RepoProcessTask task) {
      pendingTasks.push_back(task);
    }

    int HTTPRepoPrivate::countDirtyHashCaches() const
    {
        int result = rootDir->isHashCacheDirty() ? 1 : 0;
        for (const auto& dir : directories) {
            if (dir->isHashCacheDirty()) {
                ++result;
            }
        }

        return result;
    }

    void HTTPRepoPrivate::flushHashCaches()
    {
        rootDir->writeHashCache();
        for (const auto& dir : directories) {
            dir->writeHashCache();
        }
    }

} // of namespace simgear
