// -*- coding: utf-8 -*-
//
// EmbeddedResourceManager.hxx --- Manager class for resources embedded in an
//                                 executable
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

#ifndef FG_EMBEDDEDRESOURCEMANAGER_HXX
#define FG_EMBEDDEDRESOURCEMANAGER_HXX

#include <string>
#include <memory>               // std::unique_ptr, std::shared_ptr
#include <unordered_map>
#include <vector>
#include <utility>              // std::forward()
#include <cstddef>              // std::size_t

#include <simgear/structure/exception.hxx>
#include "EmbeddedResource.hxx"

namespace simgear
{

class EmbeddedResourceManager
{
public:
  EmbeddedResourceManager(const EmbeddedResourceManager&) = delete;
  EmbeddedResourceManager& operator=(const EmbeddedResourceManager&) = delete;
  EmbeddedResourceManager(EmbeddedResourceManager&&) = delete;
  EmbeddedResourceManager& operator=(EmbeddedResourceManager&&) = delete;
  // The instance is created by createInstance() -> private constructor
  // but it should be deleted by its owning std::unique_ptr -> public destructor
  ~EmbeddedResourceManager() = default;

  // Static creator
  static const std::unique_ptr<EmbeddedResourceManager>& createInstance();

  // Singleton accessor
  static const std::unique_ptr<EmbeddedResourceManager>& instance();

  // Return the currently-selected “locale”[*] for resource fetching.
  //
  // [*] For instance: std::string("")      for the default locale
  //                   std::string("fr")    for French
  //                   std::string("fr_FR") for French from France
  std::string getLocale() const;
  // Select the locale for which resources will be returned in the future, for
  // the getResourceOrNullPtr(), getResource(), getString(), getStreambuf()
  // and getIStream() overloads that don't have a 'locale' parameter.
  // May be called several times. Return the previously-selected locale.
  //
  // If you just want to fetch one or two resources in a particular “locale”
  // (language), it is simpler to use an overload of one of the
  // getResourceOrNullPtr(), getResource(), ..., getIStream() methods that has
  // a 'locale' parameter.
  std::string selectLocale(const std::string& locale);

  // Add a resource for the specified locale to the embedded resource manager.
  // This method acts as a sink for its second argument (the std::unique_ptr
  // typically has to be std::move()d). If 'locale' is empty, the resource is
  // added for the default locale.
  void addResource(const std::string& virtualPath,
                   std::unique_ptr<const AbstractEmbeddedResource> resourcePtr,
                   const std::string& locale = std::string());

  // Get access to a resource.
  //
  // Fetch the resource for the selected locale (cf. selectLocale()), with
  // fallback behavior[1]. If no resource is found for the given
  // 'virtualPath', return a null
  // std::shared_ptr<const AbstractEmbeddedResource>.
  //
  // [1] This means that for instance, if the selected locale is 'es_ES', the
  //     resource is first looked up for the 'es_ES' “locale”; then, if not
  //     found, for 'es'; and finally, if still not found, for the default
  //     locale ''.
  std::shared_ptr<const AbstractEmbeddedResource> getResourceOrNullPtr(
    const std::string& virtualPath) const;
  // Same as the previous overload, except the resource is fetched for the
  // specified locale (with fallback behavior) instead of for the selected
  // locale. Use an empty 'locale' parameter to fetch the resource for the
  // default locale.
  std::shared_ptr<const AbstractEmbeddedResource> getResourceOrNullPtr(
    const std::string& virtualPath,
    const std::string& locale) const;

  // Same overloads as for getResourceOrNullPtr(), except that if the resource
  // isn't found, then an sg_exception is raised. These methods never return
  // a null or empty std::shared_ptr<const AbstractEmbeddedResource>.
  template <typename ...Args>
  std::shared_ptr<const AbstractEmbeddedResource> getResource(
    const std::string& virtualPath, Args&& ...args) const
  {
    const auto resPtr = getResourceOrNullPtr(virtualPath,
                                             std::forward<Args>(args)...);

    if (!resPtr) {
      throw sg_exception("No embedded resource found at virtual path '" +
                         virtualPath + "'");
    }

    return resPtr;
  }

  // Get a resource contents in the form of an std::string. Raise an
  // sg_exception if no resource is found for the specified 'virtualPath'.
  //
  // The returned std::string is a copy of the resource contents (possibly
  // transparently decompressed, cf. simgear::ZlibEmbeddedResource).
  template <typename ...Args>
  std::string getString(const std::string& virtualPath, Args&& ...args) const
  {
    return getResource(virtualPath, std::forward<Args>(args)...)->str();
  }

  // Get access to a resource via an std::streambuf instance. Raise an
  // sg_exception if no resource is found for the specified 'virtualPath'.
  //
  // This allows one to incrementally process the resource contents without
  // ever making a copy of it (including incremental, transparent
  // decompression if the resource happens to be compressed---cf.
  // simgear::ZlibEmbeddedResource).
  template <typename ...Args>
  std::unique_ptr<std::streambuf> getStreambuf(const std::string& virtualPath,
                                               Args&& ...args) const
  {
    return getResource(virtualPath, std::forward<Args>(args)...)->streambuf();
  }

  // Get access to a resource via an std::istream instance. Raise an
  // sg_exception if no resource is found for the specified 'virtualPath'.
  //
  // The same remarks made for getStreambuf() apply here too.
  template <typename ...Args>
  std::unique_ptr<std::istream> getIStream(const std::string& virtualPath,
                                           Args&& ...args) const
  {
    return getResource(virtualPath, std::forward<Args>(args)...)->istream();
  }

private:
  // Constructor called from createInstance() only
  explicit EmbeddedResourceManager();

  class Impl;
  const std::unique_ptr<Impl> p; // Pimpl idiom
};

// Explicit template instantiations
template
std::shared_ptr<const AbstractEmbeddedResource>
EmbeddedResourceManager::getResource(const std::string& virtualPath) const;

template
std::string
EmbeddedResourceManager::getString(const std::string& virtualPath) const;

template
std::unique_ptr<std::streambuf>
EmbeddedResourceManager::getStreambuf(const std::string& virtualPath) const;

template
std::unique_ptr<std::istream>
EmbeddedResourceManager::getIStream(const std::string& virtualPath) const;

// MSVC doesn't recognize these as template instantiations of what we defined
// above (this seems to be with “Visual Studio 14 2015”), therefore only
// include them with other compilers.
#ifndef _MSC_VER
template
std::shared_ptr<const AbstractEmbeddedResource>
EmbeddedResourceManager::getResource(const std::string& virtualPath,
                                     const std::string& locale) const;
template
std::string
EmbeddedResourceManager::getString(const std::string& virtualPath,
                                   const std::string& locale) const;
template
std::unique_ptr<std::streambuf>
EmbeddedResourceManager::getStreambuf(const std::string& virtualPath,
                                      const std::string& locale) const;
template
std::unique_ptr<std::istream>
EmbeddedResourceManager::getIStream(const std::string& virtualPath,
                                    const std::string& locale) const;
#endif  // #ifndef _MSC_VER

} // of namespace simgear

#endif  // of FG_EMBEDDEDRESOURCEMANAGER_HXX
