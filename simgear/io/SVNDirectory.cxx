
#include "SVNDirectory.hxx"

#include <cassert>
#include <fstream>
#include <iostream>
#include <boost/foreach.hpp>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/DAVMultiStatus.hxx>
#include <simgear/io/SVNRepository.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/io/SVNReportParser.hxx>
#include <simgear/package/md5.h>
#include <simgear/structure/exception.hxx>

using std::string;
using std::cout;
using std::endl;
using namespace simgear;

typedef std::vector<HTTP::Request_ptr> RequestVector;
typedef std::map<std::string, DAVResource*> DAVResourceMap;


const char* DAV_CACHE_NAME = ".terrasync_cache";
const char* CACHE_VERSION_4_TOKEN = "terrasync-cache-4";

// important: with the Google servers, setting this higher than '1' causes
// server internal errors (500, the connection is closed). In other words we
// can only specify update report items one level deep at most and no more.
// (the root and its direct children, not NOT grand-children)
const unsigned int MAX_UPDATE_REPORT_DEPTH = 1;

enum LineState
{
  LINESTATE_HREF = 0,
  LINESTATE_VERSIONNAME
};

SVNDirectory::SVNDirectory(SVNRepository *r, const SGPath& path) :
  localPath(path),
  dav(NULL),
  repo(r),
  _doingUpdateReport(false),
  _parent(NULL)
{
  if (path.exists()) {
    parseCache();
  } 
  
  // don't create dir here, repo might not exist at all
}
  
SVNDirectory::SVNDirectory(SVNDirectory* pr, DAVCollection* col) :
  dav(col),
  repo(pr->repository()),
  _doingUpdateReport(false),
  _parent(pr)
{
  assert(col->container());
  assert(!col->url().empty());
  assert(_parent);

  localPath = pr->fsDir().file(col->name());
  if (!localPath.exists()) {
    Dir d(localPath);
    d.create(0755);
    writeCache();
  } else {
    parseCache();
  }
}

SVNDirectory::~SVNDirectory()
{
    // recursive delete our child directories
    BOOST_FOREACH(SVNDirectory* d, _children) {
        delete d;
    }
}

void SVNDirectory::parseCache()
{
  SGPath p(localPath);
  p.append(DAV_CACHE_NAME);
  if (!p.exists()) {
    return;
  }
    
  char href[1024];
  char versionName[128];
  LineState lineState = LINESTATE_HREF;
  std::ifstream file(p.c_str());
    if (!file.is_open()) {
        SG_LOG(SG_IO, SG_WARN, "unable to open cache file for reading:" << p);
        return;
    }
  bool doneSelf = false;
    
  file.getline(href, 1024);
  if (strcmp(CACHE_VERSION_4_TOKEN, href)) {
    SG_LOG(SG_IO, SG_WARN, "invalid cache file [missing header token]:" << p << " '" << href << "'");
    return;
  }
    
    std::string vccUrl;
    file.getline(href, 1024);
    vccUrl = href;
    
  while (!file.eof()) {
    if (lineState == LINESTATE_HREF) {
      file.getline(href, 1024);
      lineState = LINESTATE_VERSIONNAME;
    } else {
      assert(lineState == LINESTATE_VERSIONNAME);
      file.getline(versionName, 1024);
      lineState = LINESTATE_HREF;
      char* hrefPtr = href;
    
      if (!doneSelf) {
        if (!dav) {
          dav = new DAVCollection(hrefPtr);
          dav->setVersionName(versionName);
        } else {
          assert(string(hrefPtr) == dav->url());
        }
        
        if (!vccUrl.empty()) {
            dav->setVersionControlledConfiguration(vccUrl);
        }
        
        _cachedRevision = versionName;
        doneSelf = true;
      } else {
        DAVResource* child = addChildDirectory(hrefPtr)->collection();
          string s = strutils::strip(versionName);
          if (!s.empty()) {
              child->setVersionName(versionName);
          }
      } // of done self test
    } // of line-state switching 
  } // of file get-line loop
}
  
void SVNDirectory::writeCache()
{
  SGPath p(localPath);
  if (!p.exists()) {
      Dir d(localPath);
      d.create(0755);
  }
  
  p.append(string(DAV_CACHE_NAME) + ".new");
    
  std::ofstream file(p.c_str(), std::ios::trunc);
// first, cache file version header
  file << CACHE_VERSION_4_TOKEN << '\n';
 
// second, the repository VCC url
  file << dav->versionControlledConfiguration() << '\n';
      
// third, our own URL, and version
  file << dav->url() << '\n' << _cachedRevision << '\n';
  
  BOOST_FOREACH(DAVResource* child, dav->contents()) {
    if (child->isCollection()) {
        file << child->name() << '\n' << child->versionName() << "\n";
    }
  } // of child iteration
    
    file.close();
    
// approximately atomic delete + rename operation
    SGPath cacheName(localPath);
    cacheName.append(DAV_CACHE_NAME);
    if (cacheName.exists()) {
        cacheName.remove();
    }
    p.rename(cacheName);
}

void SVNDirectory::setBaseUrl(const string& url)
{
    if (_parent) {
        SG_LOG(SG_IO, SG_ALERT, "setting base URL on non-root directory " << url);
        return;
    }
    
    if (dav && (url == dav->url())) {
        return;
    }
    
    dav = new DAVCollection(url);
}

std::string SVNDirectory::url() const
{
  if (!_parent) {
    return repo->baseUrl();
  }
  
  return _parent->url() + "/" + name();
}

std::string SVNDirectory::name() const
{
    return dav->name();
} 

DAVResource*
SVNDirectory::addChildFile(const std::string& fileName)
{
    DAVResource* child = NULL;
    child = new DAVResource(dav->urlForChildWithName(fileName));
    dav->addChild(child);
  
    writeCache();
    return child;
}

SVNDirectory*
SVNDirectory::addChildDirectory(const std::string& dirName)
{
    if (dav->childWithName(dirName)) {
        // existing child, let's remove it
        deleteChildByName(dirName);
    }
    
    DAVCollection* childCol = dav->createChildCollection(dirName);
    SVNDirectory* child = new SVNDirectory(this, childCol);
    childCol->setVersionName(child->cachedRevision());
    _children.push_back(child);
    writeCache();
    return child;
}

void SVNDirectory::deleteChildByName(const std::string& nm)
{
    DAVResource* child = dav->childWithName(nm);
    if (!child) {
//        std::cerr << "ZZZ: deleteChildByName: unknown:" << nm << std::endl;
        return;
    }

    SGPath path = fsDir().file(nm);
    
    if (child->isCollection()) {
        Dir d(path);
        d.remove(true);
    
        DirectoryList::iterator it = findChildDir(nm);
        if (it != _children.end()) {
            SVNDirectory* c = *it;
    //        std::cout << "YYY: deleting SVNDirectory for:" << nm << std::endl;
            delete c;
            _children.erase(it);
        }
    } else {
        path.remove();
    }

    dav->removeChild(child);
    delete child;

    writeCache();
}
  
bool SVNDirectory::isDoingSync() const
{
  if (_doingUpdateReport) {
      return true;
  } 

  BOOST_FOREACH(SVNDirectory* child, _children) {
      if (child->isDoingSync()) {
          return true;
      } // of children
  }
    
  return false;
}

void SVNDirectory::beginUpdateReport()
{
    _doingUpdateReport = true;
    _cachedRevision.clear();
    writeCache();
}

void SVNDirectory::updateReportComplete()
{
    _cachedRevision = dav->versionName();
    _doingUpdateReport = false;
    writeCache();
    
    SVNDirectory* pr = parent();
    if (pr) {
        pr->writeCache();
    }    
}

SVNRepository* SVNDirectory::repository() const
{
    return repo;
}

void SVNDirectory::mergeUpdateReportDetails(unsigned int depth, 
    string_list& items)
{
    // normal, easy case: we are fully in-sync at a revision
    if (!_cachedRevision.empty()) {
        std::ostringstream os;
        os << "<S:entry rev=\"" << _cachedRevision << "\" depth=\"infinity\">"
            << repoPath() << "</S:entry>";
        items.push_back(os.str());
        return;
    }
    
    Dir d(localPath);
    if (depth >= MAX_UPDATE_REPORT_DEPTH) {
        SG_LOG(SG_IO, SG_INFO, localPath << "exceeded MAX_UPDATE_REPORT_DEPTH, cleaning");
        d.removeChildren();
        return;
    }
    
    PathList cs = d.children(Dir::NO_DOT_OR_DOTDOT | Dir::INCLUDE_HIDDEN | Dir::TYPE_DIR);
    BOOST_FOREACH(SGPath path, cs) {
        SVNDirectory* c = child(path.file());
        if (!c) {
            // ignore this child, if it's an incomplete download,
            // it will be over-written on the update anyway
            //std::cerr << "unknown SVN child" << path << std::endl;
        } else {
            // recurse down into children
            c->mergeUpdateReportDetails(depth+1, items);
        }
    } // of child dir iteration
}

std::string SVNDirectory::repoPath() const
{
    if (!_parent) {
        return "/";
    }
    
    // find the length of the repository base URL, then
    // trim that off our repo URL - job done!
    size_t baseUrlLen = repo->baseUrl().size();
    return dav->url().substr(baseUrlLen + 1);
}

SVNDirectory* SVNDirectory::parent() const
{
    return _parent;
}

SVNDirectory* SVNDirectory::child(const std::string& dirName) const
{
    BOOST_FOREACH(SVNDirectory* d, _children) {
        if (d->name() == dirName) {
            return d;
        }
    }
    
    return NULL;
}

DirectoryList::iterator
SVNDirectory::findChildDir(const std::string& dirName)
{
    DirectoryList::iterator it;
    for (it=_children.begin(); it != _children.end(); ++it) {
        if ((*it)->name() == dirName) {
            return it;
        }
    }
    return it;
}

simgear::Dir SVNDirectory::fsDir() const
{
    return Dir(localPath);
}
