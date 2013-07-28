// DAVCollectionMirror.hxx - mirror a DAV collection to the local filesystem
//
// Copyright (C) 2013  James Turner <zakalawe@mac.com>
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


#ifndef SG_IO_DAVCOLLECTIONMIRROR_HXX
#define SG_IO_DAVCOLLECTIONMIRROR_HXX

#include <string>
#include <vector>
#include <memory>

#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/io/DAVMultiStatus.hxx>

namespace simgear  {

class Dir;
namespace HTTP { class Request; }

// forward decls
class DAVMirror;
class SVNRepository;
class SVNDirectory;

typedef std::vector<SVNDirectory*> DirectoryList;

class SVNDirectory
{
public:
  // init from local
  SVNDirectory(SVNRepository *repo, const SGPath& path);
  ~SVNDirectory();
  
  void setBaseUrl(const std::string& url);
  
  // init from a collection
  SVNDirectory(SVNDirectory* pr, DAVCollection* col);

  void beginUpdateReport();
  void updateReportComplete();
  
  bool isDoingSync() const;
  
  std::string url() const;
  
  std::string name() const;
      
  DAVResource* addChildFile(const std::string& fileName);
  SVNDirectory* addChildDirectory(const std::string& dirName);
      
 // void updateChild(DAVResource* child);
  void deleteChildByName(const std::string& name);
  
  SGPath fsPath() const
    { return localPath; }
  
  simgear::Dir fsDir() const;
  
  std::string repoPath() const;
    
  SVNRepository* repository() const;
  DAVCollection* collection() const
    { return dav; }
  
  std::string cachedRevision() const
      { return _cachedRevision; }
  
  void mergeUpdateReportDetails(unsigned int depth, string_list& items);
  
  SVNDirectory* parent() const;
  SVNDirectory* child(const std::string& dirName) const;
private:
  
  void parseCache();
  void writeCache();
  
  DirectoryList::iterator findChildDir(const std::string& dirName);
      
  SGPath localPath;
  DAVCollection* dav;
  SVNRepository* repo;
  
  std::string _cachedRevision;
  bool _doingUpdateReport;
  
  SVNDirectory* _parent;
  DirectoryList _children;
};

} // of namespace simgear

#endif // of SG_IO_DAVCOLLECTIONMIRROR_HXX
