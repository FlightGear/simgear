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

#include <iostream>
#include <cassert>
#include <algorithm>
#include <sstream>
#include <map>
#include <set>
#include <fstream>
#include <limits>
#include <cstdlib>

#include <fcntl.h>

#include "simgear/debug/logstream.hxx"
#include "simgear/misc/strutils.hxx"

#include <simgear/misc/sg_dir.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/io/untar.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/timing/timestamp.hxx>

#include <simgear/misc/sg_hash.hxx>

namespace simgear
{

    class HTTPDirectory;
    using HTTPDirectory_ptr = std::unique_ptr<HTTPDirectory>;

    class HTTPRepoGetRequest : public HTTP::Request
    {
    public:
        HTTPRepoGetRequest(HTTPDirectory* d, const std::string& u) :
            HTTP::Request(u),
            _directory(d)
        {
        }

        virtual void cancel();

        size_t contentSize() const
        {
            return _contentSize;
        }

        void setContentSize(size_t sz)
        {
            _contentSize = sz;
        }
    protected:
        HTTPDirectory* _directory;
        size_t _contentSize = 0;
    };

    typedef SGSharedPtr<HTTPRepoGetRequest> RepoRequestPtr;

    std::string innerResultCodeAsString(HTTPRepository::ResultCode code)
    {
        switch (code) {
        case HTTPRepository::REPO_NO_ERROR:     return "no error";
        case HTTPRepository::REPO_ERROR_NOT_FOUND:  return "not found";
        case HTTPRepository::REPO_ERROR_SOCKET:     return "socket error";
        case HTTPRepository::SVN_ERROR_XML:        return "malformed XML";
        case HTTPRepository::SVN_ERROR_TXDELTA:        return "malformed XML";
        case HTTPRepository::REPO_ERROR_IO:         return "I/O error";
        case HTTPRepository::REPO_ERROR_CHECKSUM:         return "checksum verification error";
        case HTTPRepository::REPO_ERROR_FILE_NOT_FOUND: return "file not found";
        case HTTPRepository::REPO_ERROR_HTTP:   return "HTTP-level error";
        case HTTPRepository::REPO_ERROR_CANCELLED:   return "cancelled";
        case HTTPRepository::REPO_PARTIAL_UPDATE:   return "partial update (incomplete)";
        }

        return "Unknown response code";
    }

class HTTPRepoPrivate
{
public:
    struct HashCacheEntry
    {
        std::string filePath;
        time_t modTime;
        size_t lengthBytes;
        std::string hashHex;

    };

    typedef std::vector<HashCacheEntry> HashCache;
    HashCache hashes;
    bool hashCacheDirty;

    struct Failure
    {
        SGPath path;
        HTTPRepository::ResultCode error;
    };

    typedef std::vector<Failure> FailureList;
    FailureList failures;

    HTTPRepoPrivate(HTTPRepository* parent) :
        hashCacheDirty(false),
        p(parent),
        isUpdating(false),
        status(HTTPRepository::REPO_NO_ERROR),
        totalDownloaded(0)
    { ; }

    ~HTTPRepoPrivate();

    HTTPRepository* p; // link back to outer
    HTTP::Client* http;
    std::string baseUrl;
    SGPath basePath;
    bool isUpdating;
    HTTPRepository::ResultCode status;
    HTTPDirectory_ptr rootDir;
    size_t totalDownloaded;

    HTTP::Request_ptr updateFile(HTTPDirectory* dir, const std::string& name,
                                 size_t sz);
    HTTP::Request_ptr updateDir(HTTPDirectory* dir, const std::string& hash,
                                size_t sz);

    std::string hashForPath(const SGPath& p);
    void updatedFileContents(const SGPath& p, const std::string& newHash);
    void parseHashCache();
    std::string computeHashForPath(const SGPath& p);
    void writeHashCache();

    void failedToGetRootIndex(HTTPRepository::ResultCode st);
    void failedToUpdateChild(const SGPath& relativePath,
                             HTTPRepository::ResultCode fileStatus);

    typedef std::vector<RepoRequestPtr> RequestVector;
    RequestVector queuedRequests,
        activeRequests;

    void makeRequest(RepoRequestPtr req);
    void finishedRequest(const RepoRequestPtr& req);

    HTTPDirectory* getOrCreateDirectory(const std::string& path);
    bool deleteDirectory(const std::string& relPath, const SGPath& absPath);

    typedef std::vector<HTTPDirectory_ptr> DirectoryVector;
    DirectoryVector directories;

    SGPath installedCopyPath;
};

class HTTPDirectory
{
    struct ChildInfo
    {
        enum Type
        {
            FileType,
            DirectoryType,
            TarballType
        };

        ChildInfo(Type ty, const std::string & nameData, const std::string & hashData) :
            type(ty),
            name(nameData),
            hash(hashData)
        {
        }

		    ChildInfo(const ChildInfo& other) = default;

        void setSize(const std::string & sizeData)
        {
            sizeInBytes = ::strtol(sizeData.c_str(), NULL, 10);
        }

        bool operator<(const ChildInfo& other) const
        {
            return name < other.name;
        }

        Type type;
        std::string name, hash;
		size_t sizeInBytes = 0;
		SGPath path; // absolute path on disk
    };

    typedef std::vector<ChildInfo> ChildInfoList;
    ChildInfoList children;

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
        if (_relativePath.empty()) {
            return _repository->baseUrl;
        }

        return _repository->baseUrl + "/" + _relativePath;
    }

    void dirIndexUpdated(const std::string& hash)
    {
        SGPath fpath(absolutePath());
        fpath.append(".dirindex");
        _repository->updatedFileContents(fpath, hash);

        children.clear();
        parseDirIndex(children);
        std::sort(children.begin(), children.end());
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

		char* buf = nullptr;
		size_t bufSize = 0;

		for (const auto& child : children) {
			if (child.type != ChildInfo::FileType)
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

			if (bufSize < cp.sizeInBytes()) {
				bufSize = cp.sizeInBytes();
				free(buf);
				buf = (char*) malloc(bufSize);
				if (!buf) {
					continue;
				}
			}

			src.read(buf, cp.sizeInBytes());
			dst.write(buf, cp.sizeInBytes());
			src.close();
			dst.close();

		}

		free(buf);
    }

    void updateChildrenBasedOnHash()
    {
      copyInstalledChildren();

      ChildInfoList toBeUpdated;

      simgear::Dir d(absolutePath());
      PathList fsChildren = d.children(0);
      PathList orphans = d.children(0);

      ChildInfoList::const_iterator it;
      for (it=children.begin(); it != children.end(); ++it) {
        // Check if the file exists
        PathList::const_iterator p = std::find_if(fsChildren.begin(), fsChildren.end(), LocalFileMatcher(*it));
        if (p == fsChildren.end()) {
          // File or directory does not exist on local disk, so needs to be updated.
          toBeUpdated.push_back(ChildInfo(*it));
        } else if (hashForChild(*it) != it->hash) {
          // File/directory exists, but hash doesn't match.
          toBeUpdated.push_back(ChildInfo(*it));
          orphans.erase(std::remove(orphans.begin(), orphans.end(), *p), orphans.end());
        } else {
          // File/Directory exists and hash is valid.
          orphans.erase(std::remove(orphans.begin(), orphans.end(), *p), orphans.end());

          if (it->type == ChildInfo::DirectoryType) {
              // If it's a directory,perform a recursive check.
              HTTPDirectory* childDir = childDirectory(it->name);
              childDir->updateChildrenBasedOnHash();
          }
        }
      }

      // We now have a list of entries that need to be updated, and a list
      // of orphan files that should be removed.
      removeOrphans(orphans);
      scheduleUpdates(toBeUpdated);
    }

    HTTPDirectory* childDirectory(const std::string& name)
    {
        std::string childPath = relativePath().empty() ? name : relativePath() + "/" + name;
        return _repository->getOrCreateDirectory(childPath);
    }

    void removeOrphans(const PathList orphans)
    {
        PathList::const_iterator it;
        for (it = orphans.begin(); it != orphans.end(); ++it) {
            if (it->file() == ".dirindex") continue;
            if (it->file() == ".hash") continue;
            removeChild(*it);
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
            if (it->type == ChildInfo::FileType) {
                _repository->updateFile(this, it->name, it->sizeInBytes);
            } else if (it->type == ChildInfo::DirectoryType){
                HTTPDirectory* childDir = childDirectory(it->name);
                _repository->updateDir(childDir, it->hash, it->sizeInBytes);
            } else if (it->type == ChildInfo::TarballType) {
                // Download a tarball just as a file.
                _repository->updateFile(this, it->name, it->sizeInBytes);
            } else {
                SG_LOG(SG_TERRASYNC, SG_ALERT, "Coding error!  Unknown Child type to schedule update");
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

    void didUpdateFile(const std::string& file, const std::string& hash, size_t sz)
    {
        // check hash matches what we expected
        auto it = findIndexChild(file);
        if (it == children.end()) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "updated file but not found in dir:" << _relativePath << " " << file);
        } else {
            if (it->hash != hash) {
                SG_LOG(SG_TERRASYNC, SG_INFO, "Checksum error for " << absolutePath() << "/" << file << " " << it->hash << " " << hash);
                // we don't erase the file on a hash mismatch, because if we're syncing during the
                // middle of a server-side update, the downloaded file may actually become valid.
                _repository->failedToUpdateChild(_relativePath, HTTPRepository::REPO_ERROR_CHECKSUM);
            } else {
                _repository->updatedFileContents(it->path, hash);
                _repository->totalDownloaded += sz;
                SGPath p = SGPath(absolutePath(), file);

                if ((p.extension() == "tgz") || (p.extension() == "zip")) {
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
                    // If this is a tarball, then extract it.
                    SGBinaryFile f(p);
                    if (! f.open(SG_IO_IN)) SG_LOG(SG_TERRASYNC, SG_ALERT, "Unable to open " << p << " to extract");

                    SG_LOG(SG_TERRASYNC, SG_INFO, "Extracting " << absolutePath() << "/" << file << " to " << p.dir());
                    SGPath extractDir = p.dir();
                    ArchiveExtractor ex(extractDir);

                    uint8_t* buf = (uint8_t*) alloca(128);
                    while (!f.eof()) {
                      size_t bufSize = f.read((char*) buf, 128);
                      ex.extractBytes(buf, bufSize);
                    }

                    ex.flush();
                    if (! ex.isAtEndOfArchive()) {
                      SG_LOG(SG_TERRASYNC, SG_ALERT, "Corrupt tarball " << p);
                    }

                    if (ex.hasError()) {
                      SG_LOG(SG_TERRASYNC, SG_ALERT, "Error extracting " << p);
                    }

                  } else {
                    SG_LOG(SG_TERRASYNC, SG_ALERT, "Unable to remove old file/directory " << removePath);
                  } // of pathAvailable
                } // of handling tgz files
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

            ChildInfo ci = ChildInfo(ChildInfo::FileType, tokens[1], tokens[2]);
            if (typeData == "d") ci.type = ChildInfo::DirectoryType;
            if (typeData == "t") ci.type = ChildInfo::TarballType;

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
            _repository->updatedFileContents(path, std::string());
            ok = path.remove();
        }

        if (!ok) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "removal failed for:" << path);
            throw sg_io_exception("Failed to remove existing file/dir:", path);
        }
    }

    std::string hashForChild(const ChildInfo& child) const
    {
		    SGPath p(child.path);
        if (child.type == ChildInfo::DirectoryType) p.append(".dirindex");
        if (child.type == ChildInfo::TarballType) p.concat(".tgz");  // For tarballs the hash is against the tarball file itself
        return _repository->hashForPath(p);
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
    _d->parseHashCache();
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

void HTTPRepository::setInstalledCopyPath(const SGPath& copyPath)
{
    _d->installedCopyPath = copyPath;
}

std::string HTTPRepository::resultCodeAsString(ResultCode code)
{
    return innerResultCodeAsString(code);
}

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
        virtual void gotBodyData(const char* s, int n)
        {
            if (!file.get()) {
                file.reset(new SGBinaryFile(pathInRepo));
                if (!file->open(SG_IO_OUT)) {
                  SG_LOG(SG_TERRASYNC, SG_WARN, "unable to create file " << pathInRepo);
                    _directory->repository()->http->cancelRequest(this, "Unable to create output file:" + pathInRepo.utf8Str());
                }

                sha1_init(&hashContext);
            }

            sha1_write(&hashContext, s, n);
            file->write(s, n);
        }

        virtual void onDone()
        {
            file->close();
            if (responseCode() == 200) {
                std::string hash = strutils::encodeHex(sha1_result(&hashContext), HASH_LENGTH);
                _directory->didUpdateFile(fileName, hash, contentSize());
            } else if (responseCode() == 404) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "terrasync file not found on server: " << fileName << " for " << _directory->absolutePath());
                _directory->didFailToUpdateFile(fileName, HTTPRepository::REPO_ERROR_FILE_NOT_FOUND);
            } else {
                SG_LOG(SG_TERRASYNC, SG_WARN, "terrasync file download error on server: " << fileName << " for " << _directory->absolutePath() <<
                       "\n\tserver responded: " << responseCode() << "/" << responseReason());
                _directory->didFailToUpdateFile(fileName, HTTPRepository::REPO_ERROR_HTTP);
            }

            _directory->repository()->finishedRequest(this);
        }

        virtual void onFail()
        {
            HTTPRepository::ResultCode code = HTTPRepository::REPO_ERROR_SOCKET;
            if (responseCode() == -1) {
                code = HTTPRepository::REPO_ERROR_CANCELLED;
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
                _directory->repository()->finishedRequest(this);
            }
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

    protected:
        virtual void gotBodyData(const char* s, int n)
        {
            body += std::string(s, n);
            sha1_write(&hashContext, s, n);
        }

        virtual void onDone()
        {
            if (responseCode() == 200) {
                std::string hash = strutils::encodeHex(sha1_result(&hashContext), HASH_LENGTH);
                if (!_targetHash.empty() && (hash != _targetHash)) {
                    _directory->failedToUpdate(HTTPRepository::REPO_ERROR_CHECKSUM);
                    _directory->repository()->finishedRequest(this);
                    return;
                }

                std::string curHash = _directory->repository()->hashForPath(path());
                if (hash != curHash) {
                    simgear::Dir d(_directory->absolutePath());
                    if (!d.exists()) {
                        if (!d.create(0700)) {
                            throw sg_io_exception("Unable to create directory", d.path());
                        }
                    }

                    // dir index data has changed, so write to disk and update
                    // the hash accordingly
                    sg_ofstream of(pathInRepo(), std::ios::trunc | std::ios::out | std::ios::binary);
                    if (!of.is_open()) {
                        throw sg_io_exception("Failed to open directory index file for writing", pathInRepo());
                    }

                    of.write(body.data(), body.size());
                    of.close();
                    _directory->dirIndexUpdated(hash);

                    //SG_LOG(SG_TERRASYNC, SG_INFO, "updated dir index " << _directory->absolutePath());
                }

                _directory->repository()->totalDownloaded += contentSize();

                try {
                    // either way we've confirmed the index is valid so update
                    // children now
                    SGTimeStamp st;
                    st.stamp();
                    _directory->updateChildrenBasedOnHash();
                    SG_LOG(SG_TERRASYNC, SG_DEBUG, "after update of:" << _directory->absolutePath() << " child update took:" << st.elapsedMSec());
                } catch (sg_exception& ) {
                    _directory->failedToUpdate(HTTPRepository::REPO_ERROR_IO);
                }
            } else if (responseCode() == 404) {
                _directory->failedToUpdate(HTTPRepository::REPO_ERROR_FILE_NOT_FOUND);
            } else {
                _directory->failedToUpdate(HTTPRepository::REPO_ERROR_HTTP);
            }

            _directory->repository()->finishedRequest(this);
        }

        virtual void onFail()
        {
            if (_directory) {
                _directory->failedToUpdate(HTTPRepository::REPO_ERROR_SOCKET);
                _directory->repository()->finishedRequest(this);
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


    class HashEntryWithPath
    {
    public:
        HashEntryWithPath(const SGPath& p) : path(p.utf8Str()) {}
        bool operator()(const HTTPRepoPrivate::HashCacheEntry& entry) const
        { return entry.filePath == path; }
    private:
        std::string path;
    };

    std::string HTTPRepoPrivate::hashForPath(const SGPath& p)
    {
        HashCache::iterator it = std::find_if(hashes.begin(), hashes.end(), HashEntryWithPath(p));
        if (it != hashes.end()) {
            // ensure data on disk hasn't changed.
            // we could also use the file type here if we were paranoid
            if ((p.sizeInBytes() == it->lengthBytes) && (p.modTime() == it->modTime)) {
                return it->hashHex;
            }

            // entry in the cache, but it's stale so remove and fall through
            hashes.erase(it);
        }

        std::string hash = computeHashForPath(p);
        updatedFileContents(p, hash);
        return hash;
    }

    std::string HTTPRepoPrivate::computeHashForPath(const SGPath& p)
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
        std::string hashBytes((char*) sha1_result(&info), HASH_LENGTH);
        return strutils::encodeHex(hashBytes);
    }

    void HTTPRepoPrivate::updatedFileContents(const SGPath& p, const std::string& newHash)
    {
        // remove the existing entry
        HashCache::iterator it = std::find_if(hashes.begin(), hashes.end(), HashEntryWithPath(p));
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
        entry.filePath = p.utf8Str();
        entry.hashHex = newHash;
        entry.modTime = p2.modTime();
        entry.lengthBytes = p2.sizeInBytes();
        hashes.push_back(entry);

        hashCacheDirty = true;
    }

    void HTTPRepoPrivate::writeHashCache()
    {
        if (!hashCacheDirty) {
            return;
        }

        SGPath cachePath = basePath;
        cachePath.append(".hashes");
        sg_ofstream stream(cachePath, std::ios::out | std::ios::trunc | std::ios::binary);
        HashCache::const_iterator it;
        for (it = hashes.begin(); it != hashes.end(); ++it) {
            stream << it->filePath << "*" << it->modTime << "*"
            << it->lengthBytes << "*" << it->hashHex << "\n";
        }
        stream.close();
        hashCacheDirty = false;
    }

    void HTTPRepoPrivate::parseHashCache()
    {
        hashes.clear();
        SGPath cachePath = basePath;
        cachePath.append(".hashes");
        if (!cachePath.exists()) {
            return;
        }

        sg_ifstream stream(cachePath, std::ios::in);

        while (!stream.eof()) {
            std::string line;
            std::getline(stream,line);
            line = simgear::strutils::strip(line);
            if( line.empty() || line[0] == '#' )
                continue;

			string_list tokens = simgear::strutils::split(line, "*");
            if( tokens.size() < 4 ) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "invalid entry in '" << cachePath << "': '" << line << "' (ignoring line)");
                continue;
            }
            const std::string nameData = simgear::strutils::strip(tokens[0]);
            const std::string timeData = simgear::strutils::strip(tokens[1]);
            const std::string sizeData = simgear::strutils::strip(tokens[2]);
            const std::string hashData = simgear::strutils::strip(tokens[3]);

            if (nameData.empty() || timeData.empty() || sizeData.empty() || hashData.empty() ) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "invalid entry in '" << cachePath << "': '" << line << "' (ignoring line)");
                continue;
            }

            HashCacheEntry entry;
            entry.filePath = nameData;
            entry.hashHex = hashData;
            entry.modTime = strtol(timeData.c_str(), NULL, 10);
            entry.lengthBytes = strtol(sizeData.c_str(), NULL, 10);
            hashes.push_back(entry);
        }
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
            HTTPDirectory* d = it->get();
			assert(d->absolutePath() == absPath);
            directories.erase(it);
		} else {
			// we encounter this code path when deleting an orphaned directory
		}

		Dir dir(absPath);
		bool result = dir.remove(true);

		// update the hash cache too
		updatedFileContents(absPath, std::string());

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

    void HTTPRepoPrivate::finishedRequest(const RepoRequestPtr& req)
    {
        RequestVector::iterator it = std::find(activeRequests.begin(), activeRequests.end(), req);
        // in some cases, for example a checksum failure, we clear the active
        // and queued request vectors, so the ::find above can fail
        if (it != activeRequests.end()) {
            activeRequests.erase(it);
        }

        if (!queuedRequests.empty()) {
            RepoRequestPtr rr = queuedRequests.front();
            queuedRequests.erase(queuedRequests.begin());
            activeRequests.push_back(rr);
            http->makeRequest(rr);
        }

        writeHashCache();

        if (activeRequests.empty() && queuedRequests.empty()) {
            isUpdating = false;
        }
    }

    void HTTPRepoPrivate::failedToGetRootIndex(HTTPRepository::ResultCode st)
    {
        if (st == HTTPRepository::REPO_ERROR_FILE_NOT_FOUND) {
            status = HTTPRepository::REPO_ERROR_NOT_FOUND;
        } else {
            SG_LOG(SG_TERRASYNC, SG_WARN, "Failed to get root of repo:" << baseUrl << " " << st);
            status = st;
        }
    }

    void HTTPRepoPrivate::failedToUpdateChild(const SGPath& relativePath,
                                              HTTPRepository::ResultCode fileStatus)
    {
        if (fileStatus == HTTPRepository::REPO_ERROR_CHECKSUM) {
            // stop updating, and mark repository as failed, becuase this
            // usually indicates we need to start a fresh update from the
            // root.
            // (we could issue a retry here, but we leave that to higher layers)
            status = fileStatus;

            queuedRequests.clear();

            RequestVector copyOfActive(activeRequests);
            RequestVector::iterator rq;
            for (rq = copyOfActive.begin(); rq != copyOfActive.end(); ++rq) {
                http->cancelRequest(*rq, "Repository updated failed due to checksum error");
            }

            SG_LOG(SG_TERRASYNC, SG_WARN, "failed to update repository:" << baseUrl
                   << "\n\tchecksum failure for: " << relativePath
                   << "\n\tthis typically indicates the remote repository is corrupt or was being updated during the sync");
        } else if (fileStatus == HTTPRepository::REPO_ERROR_CANCELLED) {
            // if we were cancelled, don't report or log
            return;
        }

        Failure f;
        f.path = relativePath;
        f.error = fileStatus;
        failures.push_back(f);

        SG_LOG(SG_TERRASYNC, SG_WARN, "failed to update entry:" << relativePath << " status/code: "
               << innerResultCodeAsString(fileStatus) << "/" << fileStatus);
    }

} // of namespace simgear
