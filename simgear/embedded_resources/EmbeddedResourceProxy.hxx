// -*- coding: utf-8 -*-
//
// EmbeddedResourceProxy.hxx --- Unified access to real files or embedded
//                               resources
// Copyright (C) 2017  Florent Rougon
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA.

#ifndef FG_EMBEDDEDRESOURCEPROXY_HXX
#define FG_EMBEDDEDRESOURCEPROXY_HXX

#include <istream>
#include <memory>
#include <string>

#include <simgear/misc/sg_path.hxx>

// The EmbeddedResourceProxy class allows one to access real files or embedded
// resources in a unified way. When using it, one can switch from one data
// source to the other with minimal code changes, possibly even at runtime (in
// which case there is obviously no code change at all).
//
// Sample usage of the EmbeddedResourceProxy class (from FlightGear):
//
//   simgear::EmbeddedResourceProxy proxy(globals->get_fg_root(), "/FGData");
//   std::string s = proxy.getString("/some/path");
//   std::unique_ptr<std::istream> streamp = proxy.getIStream("/some/path");
//
// The methods getString(const std::string& path) and
// getIStream(const std::string& path) of EmbeddedResourceProxy decide whether
// to use embedded resources or real files depending on the boolean value
// passed to EmbeddedResourceProxy::setUseEmbeddedResources() (also available
// as an optional parameter to the EmbeddedResourceProxy constructor,
// defaulting to true). It is often most convenient to set this boolean once
// and then don't worry about it anymore (it is stored as a data member of
// EmbeddedResourceProxy). Otherwise, if you want to fetch resources some
// times from real files, other times from embedded resources, you may use the
// following methods:
//
//   // Retrieve contents using embedded resources
//   std:string s = proxy.getString("/some/path", true);
//   std:string s = proxy.getStringDecideOnPrefix(":/some/path");
//
//   // Retrieve contents using real files
//   std:string s = proxy.getString("/some/path", false);
//   std:string s = proxy.getStringDecideOnPrefix("/some/path");
//
// You can do exactly the same with EmbeddedResourceProxy::getIStream() and
// EmbeddedResourceProxy::getIStreamDecideOnPrefix(), except they return an
// std::unique_ptr<std::istream> instead of an std::string.
//
// Given how the 'proxy' object was constructed above, each of these calls
// will fetch data from either the real file $FG_ROOT/some/path or the
// embedded resource whose virtual path is '/FGData/some/path' (more
// precisely: the default-locale version of this resource).
//
// The 'path' argument of EmbeddedResourceProxy's methods getString(),
// getIStream(), getStringDecideOnPrefix() and getIStreamDecideOnPrefix()
// must:
//
//   - use UTF-8 encoding;
//
//   - start with:
//       * either '/' or ':/' for the 'DecideOnPrefix' variants;
//       * only '/' for the other methods.
//
//   - have its components separated by slashes;
//
//   - not contain any '.' or '..' component.
//
// For the 'DecideOnPrefix' variants:
//
//   - if the path starts with a slash ('/'), a real file access is done;
//
//   - if, on the other hand, it starts with ':/', EmbeddedResourceProxy uses
//     the embedded resource whose virtual path is the specified path without
//     its leading ':' (more precisely: the default-locale version of this
//     resource).
namespace simgear
{

class EmbeddedResourceProxy
{
public:
  // 'virtualRoot' must start with a '/', e.g: '/FGData'. Whether it ends
  // with a '/' doesn't make a difference.
  explicit EmbeddedResourceProxy(const SGPath& realRoot,
                                 const std::string& virtualRoot,
                                 bool useEmbeddedResourcesByDefault = true);

  // Getters and setters for the corresponding data members
  SGPath getRealRoot() const;
  void setRealRoot(const SGPath& realRoot);

  std::string getVirtualRoot() const;
  void setVirtualRoot(const std::string& virtualRoot);

  bool getUseEmbeddedResources() const;
  void setUseEmbeddedResources(bool useEmbeddedResources);

  // Get an std::istream to read from a file or from an embedded resource.
  std::unique_ptr<std::istream>
  getIStream(const std::string& path, bool fromEmbeddedResource) const;

  std::unique_ptr<std::istream>
  getIStream(const std::string& path) const;

  std::unique_ptr<std::istream>
  getIStreamDecideOnPrefix(const std::string& path) const;

  // Get a file or embedded resource contents as a string.
  std::string
  getString(const std::string& path, bool fromEmbeddedResource) const;

  std::string
  getString(const std::string& path) const;

  std::string
  getStringDecideOnPrefix(const std::string& path) const;

private:
  // Check that 'path' starts with either ':/' or '/', and doesn't contain any
  // '..' component ('path' may only start with ':/' if 'allowStartWithColon'
  // is true).
  static void
  checkPath(const std::string& callerMethod, const std::string& path,
            bool allowStartWithColon);

  // Normalize the 'virtualRoot' argument of the constructor. The argument
  // must start with a '/' and mustn't contain any '.' or '..' component. The
  // return value never ends with a '/'.
  static std::string
  normalizeVirtualRoot(const std::string& path);

  SGPath _realRoot;
  std::string _virtualRoot;
  bool _useEmbeddedResourcesByDefault;
};

} // of namespace simgear

#endif  // of FG_EMBEDDEDRESOURCEPROXY_HXX
