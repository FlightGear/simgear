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

#include "HTTPRepository.hxx"

#include <simgear_config.h>

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
#include <simgear/misc/sgstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/timing/timestamp.hxx>

#include <simgear/misc/sg_hash.hxx>

namespace simgear
{

    class HTTPDirectory;

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
        size_t _contentSize;
    };

    typedef SGSharedPtr<HTTPRepoGetRequest> RepoRequestPtr;

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
    HTTPDirectory* rootDir;
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
    bool deleteDirectory(const std::string& path);

    typedef std::vector<HTTPDirectory*> DirectoryVector;
    DirectoryVector directories;

};

class HTTPDirectory
{
    struct ChildInfo
    {
        enum Type
        {
            FileType,
            DirectoryType
        };

        ChildInfo(Type ty, const std::string & nameData, const std::string & hashData) :
            type(ty),
            name(nameData),
            hash(hashData),
            sizeInBytes(0)
        {
        }

        ChildInfo(const ChildInfo& other) :
            type(other.type),
            name(other.name),
            hash(other.hash),
            sizeInBytes(other.sizeInBytes)
        { }

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
        size_t sizeInBytes;
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
        if (_relativePath.str().empty()) {
            return _repository->baseUrl;
        }

        return _repository->baseUrl + "/" + _relativePath.str();
    }

    void dirIndexUpdated(const std::string& hash)
    {
        SGPath fpath(_relativePath);
        fpath.append(".dirindex");
        _repository->updatedFileContents(fpath, hash);

        children.clear();
        parseDirIndex(children);
        std::sort(children.begin(), children.end());
    }

    void failedToUpdate(HTTPRepository::ResultCode status)
    {
        if (_relativePath.isNull()) {
            // root dir failed
            _repository->failedToGetRootIndex(status);
        } else {
            _repository->failedToUpdateChild(_relativePath, status);
        }
    }

    void updateChildrenBasedOnHash()
    {
        //SG_LOG(SG_TERRASYNC, SG_DEBUG, "updated children for:" << relativePath());

        string_list indexNames = indexChildren(),
            toBeUpdated, orphans;
        simgear::Dir d(absolutePath());
        PathList fsChildren = d.children(0);
        PathList::const_iterator it = fsChildren.begin();


        for (; it != fsChildren.end(); ++it) {
            ChildInfo info(it->isDir() ? ChildInfo::DirectoryType : ChildInfo::FileType,
                           it->file(), "");
            std::string hash = hashForChild(info);

            ChildInfoList::iterator c = findIndexChild(it->file());
            if (c == children.end()) {
                SG_LOG(SG_TERRASYNC, SG_DEBUG, "is orphan '" << it->file() << "'" );
                orphans.push_back(it->file());
            } else if (c->hash != hash) {
                SG_LOG(SG_TERRASYNC, SG_DEBUG, "hash mismatch'" << it->file() );
                // file exists, but hash mismatch, schedule update
                if (!hash.empty()) {
                    SG_LOG(SG_TERRASYNC, SG_DEBUG, "file exists but hash is wrong for:" << it->file() );
                    SG_LOG(SG_TERRASYNC, SG_DEBUG, "on disk:" << hash << " vs in info:" << c->hash);
                }

                toBeUpdated.push_back(it->file() );
            } else {
                // file exists and hash is valid. If it's a directory,
                // perform a recursive check.
                SG_LOG(SG_TERRASYNC, SG_DEBUG, "file exists hash is good:" << it->file() );
                if (c->type == ChildInfo::DirectoryType) {
                    SGPath p(relativePath());
                    p.append(it->file());
                    HTTPDirectory* childDir = _repository->getOrCreateDirectory(p.str());
                    childDir->updateChildrenBasedOnHash();
                }
            }

            // remove existing file system children from the index list,
            // so we can detect new children
            // https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Erase-Remove
            indexNames.erase(std::remove(indexNames.begin(), indexNames.end(), it->file()), indexNames.end());
        } // of real children iteration

        // all remaining names in indexChilden are new children
        toBeUpdated.insert(toBeUpdated.end(), indexNames.begin(), indexNames.end());

        removeOrphans(orphans);
        scheduleUpdates(toBeUpdated);
    }

    void removeOrphans(const string_list& orphans)
    {
        string_list::const_iterator it;
        for (it = orphans.begin(); it != orphans.end(); ++it) {
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

    void scheduleUpdates(const string_list& names)
    {
        string_list::const_iterator it;
        for (it = names.begin(); it != names.end(); ++it) {
            ChildInfoList::iterator cit = findIndexChild(*it);
            if (cit == children.end()) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "scheduleUpdate, unknown child:" << *it);
                continue;
            }

            SG_LOG(SG_TERRASYNC,SG_DEBUG, "scheduling update for " << *it );
            if (cit->type == ChildInfo::FileType) {
                _repository->updateFile(this, *it, cit->sizeInBytes);
            } else {
                SGPath p(relativePath());
                p.append(*it);
                HTTPDirectory* childDir = _repository->getOrCreateDirectory(p.str());
                _repository->updateDir(childDir, cit->hash, cit->sizeInBytes);
            }
        }
    }

    SGPath absolutePath() const
    {
        SGPath r(_repository->basePath);
        r.append(_relativePath.str());
        return r;
    }

    SGPath relativePath() const
    {
        return _relativePath;
    }

    void didUpdateFile(const std::string& file, const std::string& hash, size_t sz)
    {
        // check hash matches what we expected
        ChildInfoList::iterator it = findIndexChild(file);
        if (it == children.end()) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "updated file but not found in dir:" << _relativePath << " " << file);
        } else {
            SGPath fpath(_relativePath);
            fpath.append(file);

            if (it->hash != hash) {
                _repository->failedToUpdateChild(_relativePath, HTTPRepository::REPO_ERROR_CHECKSUM);
            } else {
                _repository->updatedFileContents(fpath, hash);
                _repository->totalDownloaded += sz;
                //SG_LOG(SG_TERRASYNC, SG_INFO, "did update:" << fpath);
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

        std::ifstream indexStream( p.c_str(), std::ios::in );

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
                    SG_LOG(SG_TERRASYNC, SG_WARN, "malformed .dirindex file: missing version number in line '" << line << "'" );
                    break;
                }
                if( tokens[1] != "1" ) {
                    SG_LOG(SG_TERRASYNC, SG_WARN, "invalid .dirindex file: wrong version number '" << tokens[1] << "' (expected 1)" );
                    break;
                }
                continue; // version is good, continue
            }

            if( typeData == "path" ) {
                continue; // ignore path, next line
            }

            if( tokens.size() < 3 ) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "malformed .dirindex file: not enough tokens in line '" << line << "' (ignoring line)" );
                continue;
            }

            if (typeData != "f" && typeData != "d" ) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "malformed .dirindex file: invalid type in line '" << line << "', expected 'd' or 'f', (ignoring line)" );
                continue;
            }
            children.push_back(ChildInfo(typeData == "f" ? ChildInfo::FileType : ChildInfo::DirectoryType, tokens[1], tokens[2]));

            if (tokens.size() > 3) {
                children.back().setSize(tokens[3]);
            }
        }

        return true;
    }

    void removeChild(const std::string& name)
    {
        SGPath p(absolutePath());
        p.append(name);
        bool ok;

        SGPath fpath(_relativePath);
        fpath.append(name);

        if (p.isDir()) {
            ok = _repository->deleteDirectory(fpath.str());
        } else {
            // remove the hash cache entry
            _repository->updatedFileContents(fpath, std::string());
            ok = p.remove();
        }

        if (!ok) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "removal failed for:" << p);
            throw sg_io_exception("Failed to remove existing file/dir:", p);
        }
    }

    std::string hashForChild(const ChildInfo& child) const
    {
        SGPath p(absolutePath());
        p.append(child.name);
        if (child.type == ChildInfo::DirectoryType) {
            p.append(".dirindex");
        }
        return _repository->hashForPath(p);
    }

  HTTPRepoPrivate* _repository;
  SGPath _relativePath; // in URL and file-system space


};

HTTPRepository::HTTPRepository(const SGPath& base, HTTP::Client *cl) :
    _d(new HTTPRepoPrivate(this))
{
    _d->http = cl;
    _d->basePath = base;
    _d->rootDir = new HTTPDirectory(_d.get(), "");
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
    _d->updateDir(_d->rootDir, std::string(), 0);
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
        result += (*r)->contentSize() - (*r)->responseBytesReceived();
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
        _directory->repository()->http->cancelRequest(this, "Reposiotry cancelled");
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
            //SG_LOG(SG_TERRASYNC, SG_INFO, "will GET file " << url());
        }

    protected:
        virtual void gotBodyData(const char* s, int n)
        {
            if (!file.get()) {
                file.reset(new SGBinaryFile(pathInRepo.str()));
                if (!file->open(SG_IO_OUT)) {
                  SG_LOG(SG_TERRASYNC, SG_WARN, "unable to create file " << pathInRepo);
                  _directory->repository()->http->cancelRequest(this, "Unable to create output file");
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
                SG_LOG(SG_TERRASYNC, SG_DEBUG, "got file " << fileName << " in " << _directory->absolutePath());
            } else if (responseCode() == 404) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "terrasync file not found on server: " << fileName << " for " << _directory->absolutePath());
                _directory->didFailToUpdateFile(fileName, HTTPRepository::REPO_ERROR_FILE_NOT_FOUND);
            } else {
                SG_LOG(SG_TERRASYNC, SG_WARN, "terrasync file download error on server: " << fileName << " for " << _directory->absolutePath() << ": " << responseCode() );
                _directory->didFailToUpdateFile(fileName, HTTPRepository::REPO_ERROR_HTTP);
            }

            _directory->repository()->finishedRequest(this);
        }

        virtual void onFail()
        {
            file.reset();
            if (pathInRepo.exists()) {
                pathInRepo.remove();
            }
            
            if (_directory) {
                _directory->didFailToUpdateFile(fileName, HTTPRepository::REPO_ERROR_SOCKET);
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
        std::auto_ptr<SGBinaryFile> file;
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
           //SG_LOG(SG_TERRASYNC, SG_INFO, "will GET dir " << url());
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
                    std::ofstream of(pathInRepo().c_str(), std::ios::trunc | std::ios::out);
                    if (!of.is_open()) {
                        throw sg_io_exception("Failed to open directory index file for writing", pathInRepo().c_str());
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
                    SG_LOG(SG_TERRASYNC, SG_INFO, "after update of:" << _directory->absolutePath() << " child update took:" << st.elapsedMSec());
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
        DirectoryVector::iterator it;
        for (it=directories.begin(); it != directories.end(); ++it) {
            delete *it;
        }

        RequestVector::iterator r;
        for (r=activeRequests.begin(); r != activeRequests.end(); ++r) {
            (*r)->cancel();
        }
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
        HashEntryWithPath(const std::string& p) : path(p) {}
        bool operator()(const HTTPRepoPrivate::HashCacheEntry& entry) const
        { return entry.filePath == path; }
    private:
        std::string path;
    };

    std::string HTTPRepoPrivate::hashForPath(const SGPath& p)
    {
        HashCache::iterator it = std::find_if(hashes.begin(), hashes.end(), HashEntryWithPath(p.str()));
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
            return std::string();
        sha1nfo info;
        sha1_init(&info);
        char* buf = static_cast<char*>(malloc(1024 * 1024));
        size_t readLen;
        SGBinaryFile f(p.str());
        if (!f.open(SG_IO_IN)) {
            throw sg_io_exception("Couldn't open file for compute hash", p);
        }
        while ((readLen = f.read(buf, 1024 * 1024)) > 0) {
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
        HashCache::iterator it = std::find_if(hashes.begin(), hashes.end(), HashEntryWithPath(p.str()));
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
        entry.filePath = p.str();
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

        std::ofstream stream(cachePath.c_str(),std::ios::out | std::ios::trunc);
        HashCache::const_iterator it;
        for (it = hashes.begin(); it != hashes.end(); ++it) {
            stream << it->filePath << ":" << it->modTime << ":"
            << it->lengthBytes << ":" << it->hashHex << "\n";
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

        std::ifstream stream(cachePath.c_str(), std::ios::in);

        while (!stream.eof()) {
            std::string line;
            std::getline(stream,line);
            line = simgear::strutils::strip(line);
            if( line.empty() || line[0] == '#' )
                continue;

            string_list tokens = simgear::strutils::split( line, ":" );
            if( tokens.size() < 4 ) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "invalid entry in '" << cachePath.str() << "': '" << line << "' (ignoring line)");
                continue;
            }
            const std::string nameData = simgear::strutils::strip(tokens[0]);
            const std::string timeData = simgear::strutils::strip(tokens[1]);
            const std::string sizeData = simgear::strutils::strip(tokens[2]);
            const std::string hashData = simgear::strutils::strip(tokens[3]);

            if (nameData.empty() || timeData.empty() || sizeData.empty() || hashData.empty() ) {
                SG_LOG(SG_TERRASYNC, SG_WARN, "invalid entry in '" << cachePath.str() << "': '" << line << "' (ignoring line)");
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
        bool operator()(const HTTPDirectory* entry) const
        { return entry->relativePath().str() == path; }
    private:
        std::string path;
    };

    HTTPDirectory* HTTPRepoPrivate::getOrCreateDirectory(const std::string& path)
    {
        DirectoryWithPath p(path);
        DirectoryVector::iterator it = std::find_if(directories.begin(), directories.end(), p);
        if (it != directories.end()) {
            return *it;
        }

        HTTPDirectory* d = new HTTPDirectory(this, path);
        directories.push_back(d);
        return d;
    }

    bool HTTPRepoPrivate::deleteDirectory(const std::string& path)
    {
        DirectoryWithPath p(path);
        DirectoryVector::iterator it = std::find_if(directories.begin(), directories.end(), p);
        if (it != directories.end()) {
            HTTPDirectory* d = *it;
            directories.erase(it);
            Dir dir(d->absolutePath());
            bool result = dir.remove(true);
            delete d;

            // update the hash cache too
            updatedFileContents(path, std::string());

            return result;
        }

        return false;
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
        if (it == activeRequests.end()) {
            throw sg_exception("lost request somehow", req->url());
        }
        activeRequests.erase(it);

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
        SG_LOG(SG_TERRASYNC, SG_WARN, "Failed to get root of repo:" << baseUrl);
        status = st;
    }

    void HTTPRepoPrivate::failedToUpdateChild(const SGPath& relativePath,
                                              HTTPRepository::ResultCode fileStatus)
    {
        Failure f;
        f.path = relativePath;
        f.error = fileStatus;
        failures.push_back(f);

        SG_LOG(SG_TERRASYNC, SG_WARN, "failed to update entry:" << relativePath << " code:" << fileStatus);
    }



} // of namespace simgear
