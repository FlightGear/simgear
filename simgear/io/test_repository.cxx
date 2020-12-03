#include <cassert>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>

#include <simgear/simgear_config.h>

#include "HTTPClient.hxx"
#include "HTTPRepository.hxx"
#include "HTTPTestApi_private.hxx"
#include "test_HTTP.hxx"

#include <simgear/misc/strutils.hxx>
#include <simgear/misc/sg_hash.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/callback.hxx>

#include <simgear/io/sg_file.hxx>

using namespace simgear;

using TestApi = simgear::HTTP::TestApi;

std::string dataForFile(const std::string& parentName, const std::string& name, int revision)
{
    if (name == "zeroByteFile") {
        return {};
    }

    std::ostringstream os;
    // random content but which definitely depends on our tree location
    // and revision.
    for (int i=0; i<100; ++i) {
        os << i << parentName << "_" << name << "_" << revision;
    }

    return os.str();
}

std::string hashForData(const std::string& d)
{
    simgear::sha1nfo info;
    sha1_init(&info);
    sha1_write(&info, d.data(), d.size());
    return strutils::encodeHex(sha1_result(&info), HASH_LENGTH);
}

class TestRepoEntry;
using AccessCallback = std::function<void(TestRepoEntry &entry)>;

class TestRepoEntry
{
public:
    TestRepoEntry(TestRepoEntry* parent, const std::string& name, bool isDir);
    ~TestRepoEntry();

    TestRepoEntry* parent;
    std::string name;

    std::string indexLine() const;

    std::string hash() const;

    std::vector<TestRepoEntry*> children;

    size_t sizeInBytes() const
    {
        return data().size();
    }

    bool isDir;
    int revision; // for files
    int requestCount;
    bool getWillFail;
    bool returnCorruptData;

    AccessCallback accessCallback;

    void clearRequestCounts();

    void clearFailFlags();

    void setGetWillFail(bool b)
    {
        getWillFail = b;
    }

    void setReturnCorruptData(bool d)
    {
        returnCorruptData = d;
    }

    std::string pathInRepo() const
    {
        return parent ? (parent->pathInRepo() + "/" + name) : name;
    }

    std::string data() const;

    void defineFile(const std::string& path, int rev = 1)
    {
        string_list pathParts = strutils::split(path, "/");
        if (pathParts.size() == 1) {
            children.push_back(new TestRepoEntry(this, pathParts.front(), false));
            children.back()->revision = rev;
        } else {
            // recurse
            TestRepoEntry* c = childEntry(pathParts.front());
            if (!c) {
                // define a new directory child
                c = new TestRepoEntry(this, pathParts.front(), true);
                children.push_back(c);
            }

            size_t frontPartLength = pathParts.front().size();
            c->defineFile(path.substr(frontPartLength + 1), rev);
        }
    }

    TestRepoEntry* findEntry(const std::string& path)
    {
        if (path.empty()) {
            return this;
        }

        string_list pathParts = strutils::split(path, "/");
        TestRepoEntry* entry = childEntry(pathParts.front());
        if (pathParts.size() == 1) {
            return entry; // might be NULL
        }

        if (!entry) {
            std::cerr << "bad path: " << path << std::endl;
            return NULL;
        }

        size_t part0Length = pathParts.front().size() + 1;
        return entry->findEntry(path.substr(part0Length));
    }

    TestRepoEntry* childEntry(const std::string& name) const
    {
        assert(isDir);
        for (size_t i=0; i<children.size(); ++i) {
            if (children[i]->name == name) {
                return children[i];
            }
        }

        return NULL;
    }

    void removeChild(const std::string& name)
    {
        std::vector<TestRepoEntry*>::iterator it;
        for (it = children.begin(); it != children.end(); ++it) {
            if ((*it)->name == name) {
                delete *it;
                children.erase(it);
                return;
            }
        }
        std::cerr << "child not found:" << name << std::endl;
    }
};

TestRepoEntry::TestRepoEntry(TestRepoEntry* pr, const std::string& nm, bool d) :
    parent(pr), name(nm), isDir(d)
{
    revision = 2;
    requestCount = 0;
    getWillFail = false;
    returnCorruptData = false;
}

TestRepoEntry::~TestRepoEntry()
{
    for (size_t i=0; i<children.size(); ++i) {
        delete children[i];
    }
}

std::string TestRepoEntry::data() const
{
    if (isDir) {
        std::ostringstream os;
        os << "version:1\n";
        os << "path:" << pathInRepo() << "\n";
        for (size_t i=0; i<children.size(); ++i) {
            os << children[i]->indexLine() << "\n";
        }
        return os.str();
    } else {
        return dataForFile(parent->name, name, revision);
    }
}

std::string TestRepoEntry::indexLine() const
{
    std::ostringstream os;
    os << (isDir ? "d:" : "f:") << name << ":" << hash()
        << ":" << sizeInBytes();
    return os.str();
}

std::string TestRepoEntry::hash() const
{
    simgear::sha1nfo info;
    sha1_init(&info);
    std::string d(data());
    sha1_write(&info, d.data(), d.size());
    return strutils::encodeHex(sha1_result(&info), HASH_LENGTH);
}

void TestRepoEntry::clearRequestCounts()
{
    requestCount = 0;
    if (isDir) {
        for (size_t i=0; i<children.size(); ++i) {
            children[i]->clearRequestCounts();
        }
    }
}

void TestRepoEntry::clearFailFlags()
{
    getWillFail = false;
    returnCorruptData = false;

    if (isDir) {
        for (size_t i=0; i<children.size(); ++i) {
            children[i]->clearFailFlags();
        }
    }
}

TestRepoEntry* global_repo = NULL;

class TestRepositoryChannel : public TestServerChannel
{
public:

    virtual void processRequestHeaders()
    {
        state = STATE_IDLE;
        if (path.find("/repo/") == 0) {
//            std::cerr << "get for:" << path << std::endl;

            std::string repoPath = path.substr(6);
            bool lookingForDir = false;
            std::string::size_type suffix = repoPath.find(".dirindex");
            if (suffix != std::string::npos) {
                lookingForDir = true;
                if (suffix > 0) {
                    // trim the preceeding '/' as well, for non-root dirs
                    suffix--;
                }

                repoPath = repoPath.substr(0, suffix);
            }

            if (repoPath.find("/") == 0) { // trim leading /
                repoPath = repoPath.substr(1);
            }

            TestRepoEntry* entry = global_repo->findEntry(repoPath);
            if (!entry) {
                sendErrorResponse(404, false, "unknown repo path:" + repoPath);
                return;
            }

            if (entry->isDir != lookingForDir) {
                sendErrorResponse(404, false, "mismatched path type:" + repoPath);
                return;
            }

            if (entry->accessCallback) {
              entry->accessCallback(*entry);
            }

            if (entry->getWillFail) {
                sendErrorResponse(404, false, "entry marked to fail explicitly:" + repoPath);
                return;
            }

            entry->requestCount++;

            std::string content;
            bool closeSocket = false;
            size_t contentSize = 0;

            if (entry->returnCorruptData) {
                content = dataForFile("!$£$!" + entry->parent->name,
                                      "corrupt_" + entry->name,
                                      entry->revision);
                contentSize = content.size();
            } else {
              content = entry->data();
              contentSize = content.size();
            }

            std::stringstream d;
            d << "HTTP/1.1 " << 200 << " " << reasonForCode(200) << "\r\n";
            d << "Content-Length:" << contentSize << "\r\n";
            d << "\r\n"; // final CRLF to terminate the headers
            d << content;
            push(d.str().c_str());

            if (closeSocket) {
              closeWhenDone();
            }
        } else {
            sendErrorResponse(404, false, "");
        }
    }
};

std::string test_computeHashForPath(const SGPath& p)
{
    if (!p.exists())
        return std::string();
    sha1nfo info;
    sha1_init(&info);
    char* buf = static_cast<char*>(malloc(1024 * 1024));
	assert(buf);
    size_t readLen;

    SGBinaryFile f(p);
    f.open(SG_IO_IN);

    while ((readLen = f.read(buf, 1024 * 1024)) > 0) {
        sha1_write(&info, buf, readLen);
    }

	f.close();
	free(buf);

    std::string hashBytes((char*) sha1_result(&info), HASH_LENGTH);
    return strutils::encodeHex(hashBytes);
}

void verifyFileState(const SGPath& fsRoot, const std::string& relPath)
{
    TestRepoEntry* entry = global_repo->findEntry(relPath);
    if (!entry) {
        throw sg_error("Missing test repo entry", relPath);
    }

    SGPath p(fsRoot);
    p.append(relPath);
    if (!p.exists()) {
        throw sg_error("Missing file system entry", relPath);
    }

    std::string hashOnDisk = test_computeHashForPath(p);
    if (hashOnDisk != entry->hash()) {
        throw sg_error("Checksum mismatch", relPath);
    }
}

void verifyFileNotPresent(const SGPath& fsRoot, const std::string& relPath)
{
    SGPath p(fsRoot);
    p.append(relPath);
    if (p.exists()) {
        throw sg_error("Present file system entry", relPath);
    }
}

void verifyRequestCount(const std::string& relPath, int count)
{
    TestRepoEntry* entry = global_repo->findEntry(relPath);
    if (!entry) {
        throw sg_error("Missing test repo entry", relPath);
    }

    if (entry->requestCount != count) {
        throw sg_exception("Bad request count", relPath);
    }
}

void createFile(const SGPath& basePath, const std::string& relPath, int revision)
{
    string_list comps = strutils::split(relPath, "/");

    SGPath p(basePath);
    p.append(relPath);

    simgear::Dir d(p.dir());
    d.create(0700);

    std::string prName = comps.at(comps.size() - 2);
    {
        sg_ofstream f(p, std::ios::trunc | std::ios::out);
        f << dataForFile(prName, comps.back(), revision);
    }
}

TestServer<TestRepositoryChannel> testServer;

void waitForUpdateComplete(HTTP::Client* cl, HTTPRepository* repo)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  20000) {
        cl->update();
        testServer.poll();

        repo->process();
        if (!repo->isDoingSync()) {
            return;
        }
        SGTimeStamp::sleepForMSec(15);
    }

    std::cerr << "timed out" << std::endl;
}

void runForTime(HTTP::Client *cl, HTTPRepository *repo, int msec = 15) {
  SGTimeStamp start(SGTimeStamp::now());
  while (start.elapsedMSec() < msec) {
    cl->update();
    testServer.poll();
    repo->process();
    SGTimeStamp::sleepForMSec(1);
  }
}

void testBasicClone(HTTP::Client* cl)
{
    std::unique_ptr<HTTPRepository> repo;
    SGPath p(simgear::Dir::current().path());
    p.append("http_repo_basic");
    simgear::Dir pd(p);
    pd.removeChildren();

    repo.reset(new HTTPRepository(p, cl));
    repo->setBaseUrl("http://localhost:2000/repo");
    repo->update();

    waitForUpdateComplete(cl, repo.get());

    verifyFileState(p, "fileA");
    verifyFileState(p, "dirA/subdirA/fileAAA");
    verifyFileState(p, "dirC/subdirA/subsubA/fileCAAA");
    verifyFileState(p, "dirA/subdirA/zeroByteFile");

    global_repo->findEntry("fileA")->revision++;
    global_repo->findEntry("dirB/subdirA/fileBAA")->revision++;
    global_repo->defineFile("dirC/fileCA"); // new file
    global_repo->findEntry("dirB/subdirA")->removeChild("fileBAB");
    global_repo->findEntry("dirA")->removeChild("subdirA"); // remove a dir

    repo->update();

    // verify deltas
    waitForUpdateComplete(cl, repo.get());

    verifyFileState(p, "fileA");
    verifyFileState(p, "dirC/fileCA");

    std::cout << "Passed test: basic clone and update" << std::endl;
}

void testUpdateNoChanges(HTTP::Client* cl)
{
	std::unique_ptr<HTTPRepository> repo;
	SGPath p(simgear::Dir::current().path());
	p.append("http_repo_basic"); // same as before

	global_repo->clearRequestCounts();

	repo.reset(new HTTPRepository(p, cl));
	repo->setBaseUrl("http://localhost:2000/repo");
	repo->update();

	waitForUpdateComplete(cl, repo.get());

	verifyFileState(p, "fileA");
	verifyFileState(p, "dirC/subdirA/subsubA/fileCAAA");

	verifyRequestCount("dirA", 0);
	verifyRequestCount("dirB", 0);
	verifyRequestCount("dirB/subdirA", 0);
	verifyRequestCount("dirB/subdirA/fileBAA", 0);
	verifyRequestCount("dirC", 0);
	verifyRequestCount("dirC/fileCA", 0);

	std::cout << "Passed test:no changes update" << std::endl;

}

void testModifyLocalFiles(HTTP::Client* cl)
{
    std::unique_ptr<HTTPRepository> repo;
    SGPath p(simgear::Dir::current().path());
    p.append("http_repo_modify_local_2");
    simgear::Dir pd(p);
    if (pd.exists()) {
        pd.removeChildren();
    }

    repo.reset(new HTTPRepository(p, cl));
    repo->setBaseUrl("http://localhost:2000/repo");
    repo->update();

    waitForUpdateComplete(cl, repo.get());
    verifyFileState(p, "dirB/subdirA/fileBAA");

    SGPath modFile(p);
    modFile.append("dirB/subdirA/fileBAA");
    {
        sg_ofstream of(modFile, std::ios::out | std::ios::trunc);
        of << "complete nonsense";
        of.close();
    }

    global_repo->clearRequestCounts();
    repo->update();
    waitForUpdateComplete(cl, repo.get());
    verifyFileState(p, "dirB/subdirA/fileBAA");
    verifyRequestCount("dirB", 0);
    verifyRequestCount("dirB/subdirA", 0);
    verifyRequestCount("dirB/subdirA/fileBAA", 1);

    std::cout << "Passed test: identify and fix locally modified files" << std::endl;
}


void testMergeExistingFileWithoutDownload(HTTP::Client* cl)
{
    std::unique_ptr<HTTPRepository> repo;
    SGPath p(simgear::Dir::current().path());
    p.append("http_repo_merge_existing");
    simgear::Dir pd(p);
    if (pd.exists()) {
        pd.removeChildren();
    }

    repo.reset(new HTTPRepository(p, cl));
    repo->setBaseUrl("http://localhost:2000/repo");

    createFile(p, "dirC/fileCB", 4); // should match
    createFile(p, "dirC/fileCC", 3); // mismatch

    global_repo->defineFile("dirC/fileCB", 4);
    global_repo->defineFile("dirC/fileCC", 10);

    // new sub-tree
    createFile(p, "dirD/fileDA", 4);
    createFile(p, "dirD/subdirDA/fileDAA", 6);
    createFile(p, "dirD/subdirDB/fileDBA", 6);

    global_repo->defineFile("dirD/fileDA", 4);
    global_repo->defineFile("dirD/subdirDA/fileDAA", 6);
    global_repo->defineFile("dirD/subdirDB/fileDBA", 6);

    repo->update();
    waitForUpdateComplete(cl, repo.get());
    verifyFileState(p, "dirC/fileCB");
    verifyFileState(p, "dirC/fileCC");
    verifyRequestCount("dirC/fileCB", 0);
    verifyRequestCount("dirC/fileCC", 1);

    verifyRequestCount("dirD/fileDA", 0);
    verifyRequestCount("dirD/subdirDA/fileDAA", 0);
    verifyRequestCount("dirD/subdirDB/fileDBA", 0);

    std::cout << "Passed test: merge existing files with matching hash" << std::endl;
}

void testLossOfLocalFiles(HTTP::Client* cl)
{
    std::unique_ptr<HTTPRepository> repo;
    SGPath p(simgear::Dir::current().path());
    p.append("http_repo_lose_local");
    simgear::Dir pd(p);
    if (pd.exists()) {
        pd.removeChildren();
    }

    repo.reset(new HTTPRepository(p, cl));
    repo->setBaseUrl("http://localhost:2000/repo");
    repo->update();
    waitForUpdateComplete(cl, repo.get());
    verifyFileState(p, "dirB/subdirA/fileBAA");

    SGPath lostPath(p);
    lostPath.append("dirB/subdirA");
    simgear::Dir lpd(lostPath);
    lpd.remove(true);

    global_repo->clearRequestCounts();

    repo->update();
    waitForUpdateComplete(cl, repo.get());
    verifyFileState(p, "dirB/subdirA/fileBAA");

    verifyRequestCount("dirB", 0);
    verifyRequestCount("dirB/subdirA", 1);
    verifyRequestCount("dirB/subdirA/fileBAC", 1);

    std::cout << "Passed test: lose and replace local files" << std::endl;
}

void testAbandonMissingFiles(HTTP::Client* cl)
{
    std::unique_ptr<HTTPRepository> repo;
    SGPath p(simgear::Dir::current().path());
    p.append("http_repo_missing_files");
    simgear::Dir pd(p);
    if (pd.exists()) {
        pd.removeChildren();
    }

    global_repo->defineFile("dirA/subdirE/fileAEA");
    global_repo->findEntry("dirA/subdirE/fileAEA")->setGetWillFail(true);

    repo.reset(new HTTPRepository(p, cl));
    repo->setBaseUrl("http://localhost:2000/repo");
    repo->update();
    waitForUpdateComplete(cl, repo.get());
    if (repo->failure() != HTTPRepository::REPO_PARTIAL_UPDATE) {
        throw sg_exception("Bad result from missing files test");
    }

    global_repo->findEntry("dirA/subdirE/fileAEA")->setGetWillFail(false);
}

void testAbandonCorruptFiles(HTTP::Client* cl)
{
    std::unique_ptr<HTTPRepository> repo;
    SGPath p(simgear::Dir::current().path());
    p.append("http_repo_corrupt_files");
    simgear::Dir pd(p);
    if (pd.exists()) {
        pd.removeChildren();
    }

    global_repo->defineFile("dirB/subdirG/fileBGA");
    global_repo->findEntry("dirB/subdirG/fileBGA")->setReturnCorruptData(true);

    repo.reset(new HTTPRepository(p, cl));
    repo->setBaseUrl("http://localhost:2000/repo");
    repo->update();
    waitForUpdateComplete(cl, repo.get());
    if (repo->failure() != HTTPRepository::REPO_PARTIAL_UPDATE) {
      std::cerr << "Got failure state:" << repo->failure() << std::endl;
      throw sg_exception("Bad result from corrupt files test");
    }

    auto failedFiles = repo->failures();
    if (failedFiles.size() != 1) {
      throw sg_exception("Bad result from corrupt files test");
    }

    if (failedFiles.front().path.utf8Str() != "dirB/subdirG/fileBGA") {
      throw sg_exception("Bad path from corrupt files test:" +
                         failedFiles.front().path.utf8Str());
    }

    repo.reset();
    if (cl->hasActiveRequests()) {
        cl->debugDumpRequests();
        throw sg_exception("Connection still has requests active");
    }

    std::cout << "Passed test: detect corrupted download" << std::endl;
}

void modifyBTree()
{
    std::cout << "Modifying sub-tree" << std::endl;

    global_repo->findEntry("dirB/subdirA/fileBAC")->revision++;
    global_repo->defineFile("dirB/subdirZ/fileBZA");
    global_repo->findEntry("dirB/subdirB/fileBBB")->revision++;
}

void testServerModifyDuringSync(HTTP::Client* cl)
{
    std::unique_ptr<HTTPRepository> repo;
    SGPath p(simgear::Dir::current().path());
    p.append("http_repo_server_modify_during_sync");
    simgear::Dir pd(p);
    if (pd.exists()) {
        pd.removeChildren();
    }

    global_repo->clearRequestCounts();
    global_repo->clearFailFlags();

    repo.reset(new HTTPRepository(p, cl));
    repo->setBaseUrl("http://localhost:2000/repo");

    global_repo->findEntry("dirA/fileAA")->accessCallback =
        [](const TestRepoEntry &r) {
          std::cout << "Modifying sub-tree" << std::endl;
          global_repo->findEntry("dirB/subdirA/fileBAC")->revision++;
          global_repo->defineFile("dirB/subdirZ/fileBZA");
          global_repo->findEntry("dirB/subdirB/fileBBB")->revision++;
        };

    repo->update();
    waitForUpdateComplete(cl, repo.get());

    global_repo->findEntry("dirA/fileAA")->accessCallback = AccessCallback{};

    if (repo->failure() != HTTPRepository::REPO_PARTIAL_UPDATE) {
      throw sg_exception("Bad result from modify during sync test");
    }

    std::cout << "Passed test modify server during sync" << std::endl;

}

void testDestroyDuringSync(HTTP::Client* cl)
{
    std::unique_ptr<HTTPRepository> repo;
    SGPath p(simgear::Dir::current().path());
    p.append("http_repo_destory_during_sync");
    simgear::Dir pd(p);
    if (pd.exists()) {
        pd.removeChildren();
    }

    global_repo->clearRequestCounts();
    global_repo->clearFailFlags();

    repo.reset(new HTTPRepository(p, cl));
    repo->setBaseUrl("http://localhost:2000/repo");

    repo->update();

    // would ideally spin slightly here

    repo.reset();

    if (cl->hasActiveRequests()) {
        throw sg_exception("destory of repo didn't clean up requests");
    }

    std::cout << "Passed test destory during sync" << std::endl;
}

void testCopyInstalledChildren(HTTP::Client* cl)
{
    std::unique_ptr<HTTPRepository> repo;
    SGPath p(simgear::Dir::current().path());
    p.append("http_repo_copy_installed_children");
    simgear::Dir pd(p);
    if (pd.exists()) {
        pd.removeChildren();
    }

    // setup installation data
    SGPath p2(simgear::Dir::current().path());
    p2.append("http_repo_copy_installed_children_install");
    simgear::Dir pd2(p2);
    if (pd2.exists()) {
        pd2.removeChildren();
    } else {
        pd2.create(0700);
    }

    // fill in 'install' tree data
    createFile(p, "dirJ/fileJA", 2);
    createFile(p, "dirJ/fileJB", 3);
    createFile(p, "dirJ/fileJC", 1);

    global_repo->defineFile("dirJ/fileJA", 2);
    global_repo->defineFile("dirJ/fileJB", 3);
    global_repo->defineFile("dirJ/fileJC", 3); // newer
    global_repo->defineFile("dirJ/fileJD", 3); // not present in install

    global_repo->clearRequestCounts();
    global_repo->clearFailFlags();

    repo.reset(new HTTPRepository(p, cl));
    repo->setBaseUrl("http://localhost:2000/repo");
    repo->setInstalledCopyPath(p2);
    repo->update();

    // verify correct files were downloaded, only dirs

    waitForUpdateComplete(cl, repo.get());
    verifyFileState(p, "dirJ/fileJA");
    verifyFileState(p, "dirJ/fileJB");
    verifyFileState(p, "dirJ/fileJC");
    verifyFileState(p, "dirJ/fileJD");

    verifyRequestCount("dirJ/fileJA", 0);
    verifyRequestCount("dirJ/fileJB", 0);
    verifyRequestCount("dirJ/fileJC", 1);
    verifyRequestCount("dirJ/fileJD", 1);

    std::cout << "passed Copy installed children" << std::endl;
}

void testRetryAfterSocketFailure(HTTP::Client *cl) {
  global_repo->clearRequestCounts();
  global_repo->clearFailFlags();

  std::unique_ptr<HTTPRepository> repo;
  SGPath p(simgear::Dir::current().path());
  p.append("http_repo_retry_after_socket_fail");
  simgear::Dir pd(p);
  if (pd.exists()) {
    pd.removeChildren();
  }

  repo.reset(new HTTPRepository(p, cl));
  repo->setBaseUrl("http://localhost:2000/repo");

  int aaFailsRemaining = 2;
  int subdirBAFailsRemaining = 2;
  TestApi::setResponseDoneCallback(
      cl, [&aaFailsRemaining, &subdirBAFailsRemaining](int curlResult,
                                                       HTTP::Request_ptr req) {
        if (req->url() == "http://localhost:2000/repo/dirA/fileAA") {
          if (aaFailsRemaining == 0)
            return false;

          --aaFailsRemaining;
          TestApi::markRequestAsFailed(req, 56, "Simulated socket failure");
          return true;
        } else if (req->url() ==
                   "http://localhost:2000/repo/dirB/subdirA/.dirindex") {
          if (subdirBAFailsRemaining == 0)
            return false;

          --subdirBAFailsRemaining;
          TestApi::markRequestAsFailed(req, 56, "Simulated socket failure");
          return true;
        } else {
          return false;
        }
      });

  repo->update();
  waitForUpdateComplete(cl, repo.get());

  if (repo->failure() != HTTPRepository::REPO_NO_ERROR) {
    throw sg_exception("Bad result from retry socket failure test");
  }

  verifyFileState(p, "dirA/fileAA");
  verifyFileState(p, "dirB/subdirA/fileBAA");
  verifyFileState(p, "dirB/subdirA/fileBAC");

  verifyRequestCount("dirA/fileAA", 3);
  verifyRequestCount("dirB/subdirA", 3);
  verifyRequestCount("dirB/subdirA/fileBAC", 1);
}

void testPersistentSocketFailure(HTTP::Client *cl) {
  global_repo->clearRequestCounts();
  global_repo->clearFailFlags();

  std::unique_ptr<HTTPRepository> repo;
  SGPath p(simgear::Dir::current().path());
  p.append("http_repo_persistent_socket_fail");
  simgear::Dir pd(p);
  if (pd.exists()) {
    pd.removeChildren();
  }

  repo.reset(new HTTPRepository(p, cl));
  repo->setBaseUrl("http://localhost:2000/repo");

  TestApi::setResponseDoneCallback(
      cl, [](int curlResult, HTTP::Request_ptr req) {
        const auto url = req->url();
        if (url.find("http://localhost:2000/repo/dirB") == 0) {
          TestApi::markRequestAsFailed(req, 56, "Simulated socket failure");
          return true;
        }

        return false;
      });

  repo->update();
  waitForUpdateComplete(cl, repo.get());

  if (repo->failure() != HTTPRepository::REPO_PARTIAL_UPDATE) {
    throw sg_exception("Bad result from retry socket failure test");
  }

  verifyFileState(p, "dirA/fileAA");
  verifyRequestCount("dirA/fileAA", 1);

  verifyRequestCount("dirD/fileDA", 1);
  verifyRequestCount("dirD/subdirDA/fileDAA", 1);
  verifyRequestCount("dirD/subdirDB/fileDBA", 1);
}

int main(int argc, char* argv[])
{
  sglog().setLogLevels( SG_ALL, SG_INFO );

  HTTP::Client cl;
  cl.setMaxConnections(1);

    global_repo = new TestRepoEntry(NULL, "root", true);
    global_repo->defineFile("fileA");
    global_repo->defineFile("fileB");
    global_repo->defineFile("dirA/fileAA");
    global_repo->defineFile("dirA/fileAB");
    global_repo->defineFile("dirA/fileAC");
    global_repo->defineFile("dirA/subdirA/fileAAA");
    global_repo->defineFile("dirA/subdirA/fileAAB");
    global_repo->defineFile("dirA/subdirA/zeroByteFile");

    global_repo->defineFile("dirB/subdirA/fileBAA");
    global_repo->defineFile("dirB/subdirA/fileBAB");
    global_repo->defineFile("dirB/subdirA/fileBAC");
    global_repo->defineFile("dirB/subdirB/fileBBA");
    global_repo->defineFile("dirB/subdirB/fileBBB");
    global_repo->defineFile("dirC/subdirA/subsubA/fileCAAA");

    testBasicClone(&cl);
	testUpdateNoChanges(&cl);

    testModifyLocalFiles(&cl);

    testLossOfLocalFiles(&cl);

    testMergeExistingFileWithoutDownload(&cl);

    testAbandonMissingFiles(&cl);

    testAbandonCorruptFiles(&cl);

    testServer.disconnectAll();
    cl.clearAllConnections();

    testServerModifyDuringSync(&cl);
    testDestroyDuringSync(&cl);

    testServer.disconnectAll();
    cl.clearAllConnections();

    testCopyInstalledChildren(&cl);
    testRetryAfterSocketFailure(&cl);
    testPersistentSocketFailure(&cl);

    std::cout << "all tests passed ok" << std::endl;
    return 0;
}
