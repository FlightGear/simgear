// DAVMultiStatus.hxx -- parser for WebDAV MultiStatus XML data
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


#ifndef SG_IO_DAVMULTISTATUS_HXX
#define SG_IO_DAVMULTISTATUS_HXX

#include <string>
#include <vector>
#include <memory> // for auto_ptr

namespace simgear
{

class DAVCollection;

class DAVResource
{
public:
    DAVResource(const std::string& url);
    virtual ~DAVResource() { }
  
    typedef enum {
        Unknown = 0,
        Collection = 1
    } Type;
        
    const Type type() const
        { return _type; }
    
    const std::string& url() const
        { return _url; }

    std::string name() const;

    /**
     * SVN servers use this field to expose the head revision
     * of the resource, which is useful
     */
    const std::string& versionName() const
      { return _versionName; }

    void setVersionName(const std::string& aVersion);

    DAVCollection* container() const
        { return _container; }
    
    virtual bool isCollection() const
        { return false; }

    void setVersionControlledConfiguration(const std::string& vcc);
    const std::string& versionControlledConfiguration() const
        { return _vcc; }
    
    void setMD5(const std::string& md5Hex);
    const std::string& md5() const
        { return _md5; }
protected:
    friend class DAVCollection;
  
    Type _type;
    std::string _url;
    std::string _versionName;
    std::string _vcc;
    std::string _md5;
    DAVCollection* _container;
};

typedef std::vector<DAVResource*> DAVResourceList;

class DAVCollection : public DAVResource
{
public:
  DAVCollection(const std::string& url);
  virtual ~DAVCollection();
  
  DAVResourceList contents() const;
  
  void addChild(DAVResource* res);
  void removeChild(DAVResource* res);
  
  DAVCollection* createChildCollection(const std::string& name);
  
  /**
   * find the collection member with the specified URL, or return NULL
   * if no such member of this collection exists.
   */
  DAVResource* childWithUrl(const std::string& url) const;
     
  /**
   * find the collection member with the specified name, or return NULL
   */
  DAVResource* childWithName(const std::string& name) const;
   
  /**
   * wrapper around URL manipulation
   */
  std::string urlForChildWithName(const std::string& name) const;
  
  virtual bool isCollection() const
  { return true; }
private:
  DAVResourceList _contents;
};

class DAVMultiStatus
{
public:
    DAVMultiStatus();
    ~DAVMultiStatus();
    
    // incremental XML parsing
    void parseXML(const char* data, int size);
    
    void finishParse();
    
    bool isValid() const;
    
    DAVResource* resource();
  
  class DAVMultiStatusPrivate;
private:
    std::auto_ptr<DAVMultiStatusPrivate> _d;
};

} // of namespace simgear

#endif // of SG_IO_DAVMULTISTATUS_HXX
