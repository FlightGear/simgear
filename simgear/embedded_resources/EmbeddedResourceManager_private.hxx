// -*- coding: utf-8 -*-
//
// EmbeddedResourceManager_private.hxx --- Private implementation class for
//                                         SimGear's EmbeddedResourceManager
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

#ifndef FG_EMBEDDEDRESOURCEMANAGERPRIVATE_HXX
#define FG_EMBEDDEDRESOURCEMANAGERPRIVATE_HXX

#include <string>
#include <memory>               // std::unique_ptr, std::shared_ptr
#include <unordered_map>
#include <vector>

#include "EmbeddedResource.hxx"

namespace simgear
{

class EmbeddedResourceManager::Impl
{
public:
  explicit Impl();

  // Each “locale” for which addResource() has been used has an associated
  // resource pool, that is a sort of directory of all resources declared in
  // this locale. The resource pool for a given locale (e.g., 'fr' or 'de_DE')
  // maps resource virtual paths to the corresponding resource descriptors
  // (via std::shared_ptr<const AbstractEmbeddedResource> instances).
  //
  // Note: for optimal lookup performance, a tree would probably be better,
  //       since the expected use for each key here is to store a virtual
  //       path. But such an optimization is likely unneeded in most cases.
  typedef std::unordered_map< std::string,
                              std::shared_ptr<const AbstractEmbeddedResource> >
          ResourcePool;

  // Return the list of “locales” to scan to implement fallback behavior when
  // fetching a resource for the specified locale. This list will be searched
  // from left to right. Examples:
  //
  //   ""      -> [""]
  //   "fr"    -> ["fr", ""]
  //   "fr_FR" -> ["fr_FR", "fr", ""]
  static std::vector<std::string> localesSearchList(const std::string& locale);
  // Same as localesSearchList(), except it returns the resource pools instead
  // of the “locale” strings, and only those pools that are not empty.
  std::vector< std::shared_ptr<ResourcePool> > listOfResourcePoolsToSearch(
    const std::string& locale) const;
  // Look up, in each of the pools referred to by 'poolSearchList', the
  // resource associated to 'virtualPath'. Return the first match.
  static std::shared_ptr<const AbstractEmbeddedResource> lookupResourceInPools(
    const std::string& virtualPath,
    const std::vector< std::shared_ptr<ResourcePool> >& poolSearchList);

  // Recompute p->poolSearchList. This method is automatically called whenever
  // needed (lazily), so it doesn't need to be part of the public interface.
  void rehash();

  // Implement the corresponding EmbeddedResourceManager public methods
  std::string getLocale() const;
  std::string selectLocale(const std::string& locale);

  // Ditto
  void addResource(const std::string& virtualPath,
                   std::unique_ptr<const AbstractEmbeddedResource> resourcePtr,
                   const std::string& locale);

  std::string selectedLocale;
  // Each call to rehash() updates this member to contain precisely the
  // (ordered) list of pools to search for a resource in the selected
  // “locale”. This allows relatively cheap resource lookups, assuming the
  // desired “locale” doesn't change all the time.
  std::vector< std::shared_ptr<ResourcePool> > poolSearchList;
  // Indicate whether 'poolSearchList' must be updated (i.e., resources have
  // been added or the selected locale was changed without rehash() being
  // called afterwards).
  bool dirty;

  // Maps each “locale name” to the corresponding resource pool.
  std::unordered_map< std::string,
                      std::shared_ptr<ResourcePool> > localeToResourcePoolMap;
};

} // of namespace simgear

#endif  // of FG_EMBEDDEDRESOURCEMANAGERPRIVATE_HXX
