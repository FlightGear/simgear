// -*- coding: utf-8 -*-
//
// ResourceProxy.cxx --- Unified access to real files or embedded resources
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

#include <simgear_config.h>

#include <algorithm>            // std::find()
#include <ios>                  // std::streamsize
#include <istream>
#include <limits>               // std::numeric_limits
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>              // std::size_t
#include <cassert>

#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/structure/exception.hxx>
#include "EmbeddedResourceManager.hxx"
#include "ResourceProxy.hxx"

using std::string;
using std::vector;
using std::shared_ptr;
using std::unique_ptr;


namespace simgear
{

ResourceProxy::ResourceProxy(const SGPath& realRoot, const string& virtualRoot,
                             bool useEmbeddedResourcesByDefault)
  : _realRoot(realRoot),
    _virtualRoot(normalizeVirtualRoot(virtualRoot)),
    _useEmbeddedResourcesByDefault(useEmbeddedResourcesByDefault)
{ }

SGPath
ResourceProxy::getRealRoot() const
{ return _realRoot; }

void
ResourceProxy::setRealRoot(const SGPath& realRoot)
{ _realRoot = realRoot; }

string
ResourceProxy::getVirtualRoot() const
{ return _virtualRoot; }

void
ResourceProxy::setVirtualRoot(const string& virtualRoot)
{ _virtualRoot = normalizeVirtualRoot(virtualRoot); }

bool
ResourceProxy::getUseEmbeddedResources() const
{ return _useEmbeddedResourcesByDefault; }

void
ResourceProxy::setUseEmbeddedResources(bool useEmbeddedResources)
{ _useEmbeddedResourcesByDefault = useEmbeddedResources; }

// Static method: normalize the 'virtualRoot' argument of the constructor
//
// The argument must start with a slash and mustn't contain any '.' or '..'
// component. The return value never ends with a slash.
string
ResourceProxy::normalizeVirtualRoot(const string& path)
{
  ResourceProxy::checkPath(__func__, path, false /* allowStartWithColon */);
  string res = path;

  // Make sure 'res' doesn't end with a '/'.
  while (!res.empty() && res.back() == '/') {
    res.pop_back();    // This will ease path concatenation
  }

  return res;
}

// Static method
void
ResourceProxy::checkPath(const string& callerMethod, const string& path,
                         bool allowStartWithColon) {
  if (path.empty()) {
    throw sg_format_exception(
      "Invalid empty path for ResourceProxy::" + callerMethod + "(): '" +
      path + "'", path);
  } else if (allowStartWithColon &&
             !simgear::strutils::starts_with(path, ":/") && path[0] != '/') {
    throw sg_format_exception(
      "Invalid path for ResourceProxy::" + callerMethod + "(): it should "
      "start with either ':/' or '/'", path);
  } else if (!allowStartWithColon && path[0] != '/') {
    throw sg_format_exception(
      "Invalid path for ResourceProxy::" + callerMethod + "(): it should "
      "start with a slash ('/')", path);
  } else {
    const vector<string> components = simgear::strutils::split(path, "/");
    auto find = [&components](const string& s) -> bool {
      return (std::find(components.begin(), components.end(), s) !=
              components.end());
    };

    if (find(".") || find("..")) {
      throw sg_format_exception(
        "Invalid path for ResourceProxy::" + callerMethod + "(): "
        "'.' and '..' components are not allowed", path);
    }
  }
}

unique_ptr<std::istream>
ResourceProxy::getIStream(const string& path, bool fromEmbeddedResource) const
{
  ResourceProxy::checkPath(__func__, path, false /* allowStartWithColon */);
  assert(!path.empty() && path.front() == '/');

  if (fromEmbeddedResource) {
    const auto& embeddedResMgr = simgear::EmbeddedResourceManager::instance();
    return embeddedResMgr->getIStream(
      _virtualRoot + path,
      "");                // fetch the default-locale version of the resource
  } else {
    const SGPath sgPath = _realRoot / path.substr(std::size_t(1));
    return unique_ptr<std::istream>(new sg_ifstream(sgPath));
  }
}

unique_ptr<std::istream>
ResourceProxy::getIStream(const string& path) const
{
  return getIStream(path, _useEmbeddedResourcesByDefault);
}

unique_ptr<std::istream>
ResourceProxy::getIStreamDecideOnPrefix(const string& path) const
{
  ResourceProxy::checkPath(__func__, path, true /* allowStartWithColon */);

  // 'path' is non-empty
  if (path.front() == '/') {
    return getIStream(path, false /* fromEmbeddedResource */);
  } else if (path.front() == ':') {
    assert(path.size() >= 2 && path[1] == '/');
    // Skip the leading ':'
    return getIStream(path.substr(std::size_t(1)),
                      true /* fromEmbeddedResource */);
  } else {
    // The checkPath() call should make it impossible to reach this point.
    std::abort();
  }
}

string
ResourceProxy::getString(const string& path, bool fromEmbeddedResource) const
{
  string result;

  ResourceProxy::checkPath(__func__, path, false /* allowStartWithColon */);
  assert(!path.empty() && path.front() == '/');

  if (fromEmbeddedResource) {
    const auto& embeddedResMgr = simgear::EmbeddedResourceManager::instance();
    // Fetch the default-locale version of the resource
    result = embeddedResMgr->getString(_virtualRoot + path, "");
  } else {
    const SGPath sgPath = _realRoot / path.substr(std::size_t(1));
    result.reserve(sgPath.sizeInBytes());
    const unique_ptr<std::istream> streamp = getIStream(path,
                                                        fromEmbeddedResource);
    std::streamsize nbCharsRead;

    // Allocate a buffer
    static constexpr std::size_t bufSize = 65536;
    static_assert(bufSize <= std::numeric_limits<std::streamsize>::max(),
                  "Type std::streamsize is unexpectedly small");
    static_assert(bufSize <= std::numeric_limits<string::size_type>::max(),
                  "Type std::string::size_type is unexpectedly small");
    unique_ptr<char[]> buf(new char[bufSize]);

    do {
      streamp->read(buf.get(), bufSize);
      nbCharsRead = streamp->gcount();

      if (nbCharsRead > 0) {
        result.append(buf.get(), nbCharsRead);
      }
    } while (*streamp);

    // streamp->fail() would *not* indicate an error, due to the semantics
    // of std::istream::read().
    if (streamp->bad()) {
      throw sg_io_exception("Error reading from file", sg_location(path));
    }
  }

  return result;
}

string
ResourceProxy::getString(const string& path) const
{
  return getString(path, _useEmbeddedResourcesByDefault);
}

string
ResourceProxy::getStringDecideOnPrefix(const string& path) const
{
  string result;

  ResourceProxy::checkPath(__func__, path, true /* allowStartWithColon */);

  // 'path' is non-empty
  if (path.front() == '/') {
    result = getString(path, false /* fromEmbeddedResource */);
  } else if (path.front() == ':') {
    assert(path.size() >= 2 && path[1] == '/');
    // Skip the leading ':'
    result = getString(path.substr(std::size_t(1)),
                       true /* fromEmbeddedResource */);
  } else {
    // The checkPath() call should make it impossible to reach this point.
    std::abort();
  }

  return result;
}

} // of namespace simgear
