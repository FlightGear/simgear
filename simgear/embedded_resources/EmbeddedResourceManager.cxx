// -*- coding: utf-8 -*-
//
// EmbeddedResourceManager.cxx --- Manager class for resources embedded in an
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

#include <simgear_config.h>

#include <memory>
#include <utility>              // std::move()
#include <string>
#include <vector>
#include <cstdlib>
#include <cassert>

#include <simgear/structure/exception.hxx>
#include "EmbeddedResource.hxx"
#include "EmbeddedResourceManager.hxx"
#include "EmbeddedResourceManager_private.hxx"

using std::string;
using std::shared_ptr;
using std::unique_ptr;

namespace simgear
{

static unique_ptr<EmbeddedResourceManager> staticInstance;

// ***************************************************************************
// *                      EmbeddedResourceManager::Impl                      *
// ***************************************************************************
EmbeddedResourceManager::Impl::Impl()
  : dirty(true)
{ }

void
EmbeddedResourceManager::Impl::rehash()
{
  // Update the list of resource pools to search when looking up a resource.
  // This allows to optimize resource lookup: no need to parse, split and hash
  // the same locale string every time to find the corresponding resource
  // pools.
  poolSearchList = listOfResourcePoolsToSearch(selectedLocale);
  dirty = false;
}

string
EmbeddedResourceManager::Impl::getLocale() const
{
  return selectedLocale;
}

string
EmbeddedResourceManager::Impl::selectLocale(const std::string& locale)
{
  string previousLocale = std::move(selectedLocale);
  selectedLocale = locale;
  dirty = true;

  return previousLocale;
}

// Static method
std::vector<string>
EmbeddedResourceManager::Impl::localesSearchList(const string& locale)
{
  std::vector<string> result;

  if (locale.empty()) {
    result.push_back(string()); // only the default locale
  } else {
    std::size_t sepIdx = locale.find_first_of('_');

    if (sepIdx == string::npos) {
      // Try the given “locale” first (e.g., fr), then the default locale
      result = std::vector<string>({locale, string()});
    } else {
      string langCode = locale.substr(0, sepIdx);
      // Try the given “locale” first (e.g., fr_FR), then the language code
      // (e.g., fr) and finally the default locale
      result = std::vector<string>({locale, langCode, string()});
    }
  }

  return result;
}

auto
EmbeddedResourceManager::Impl::listOfResourcePoolsToSearch(
  const string& locale) const
  -> std::vector< shared_ptr<ResourcePool> >
{
  std::vector<string> searchedLocales = localesSearchList(locale);
  std::vector< shared_ptr<ResourcePool> > result;

  for (const string& loc: searchedLocales) {
    auto poolPtrIt = localeToResourcePoolMap.find(loc);
    // Don't store pointers to empty resource pools in 'result'. This
    // optimizes resource fetching a little bit, but requires that all
    // resources are added before this method is called.
    if (poolPtrIt != localeToResourcePoolMap.end()) {
      // Copy a shared_ptr<ResourcePool>
      result.push_back(poolPtrIt->second);
    }
  }

  return result;
}

// Static method
shared_ptr<const AbstractEmbeddedResource>
EmbeddedResourceManager::Impl::lookupResourceInPools(
  const string& virtualPath,
  const std::vector< shared_ptr<ResourcePool> >& aPoolSearchList)
{
  // Search the provided resource pools in proper order. For instance, the one
  // for 'fr_FR', then the one for 'fr' and finally the one for the default
  // locale. Return the first resource found in one of these pools.
  for (const shared_ptr<ResourcePool>& poolPtr: aPoolSearchList) {
    auto resourcePtrIt = poolPtr->find(virtualPath);

    if (resourcePtrIt != poolPtr->end()) {
      // Copy a shared_ptr<const AbstractEmbeddedResource>
      return resourcePtrIt->second;
    }
  }

  return shared_ptr<const AbstractEmbeddedResource>(); // null shared_ptr object
}

void
EmbeddedResourceManager::Impl::addResource(
  const string& virtualPath,
  unique_ptr<const AbstractEmbeddedResource> resourcePtr,
  const string& locale)
{
  // Find the resource pool corresponding to the specified locale
  shared_ptr<ResourcePool>& resPoolPtr = localeToResourcePoolMap[locale];
  if (!resPoolPtr) {
    resPoolPtr.reset(new ResourcePool());
  }

  auto emplaceRetval = resPoolPtr->emplace(virtualPath, std::move(resourcePtr));

  if (!emplaceRetval.second) {
    const string localeDescr =
      (locale.empty()) ? "the default locale" : "locale '" + locale + "'";
    throw sg_error(
      "Virtual path already in use for " + localeDescr +
      " in the EmbeddedResourceManager: '" + virtualPath + "'");
  }

  dirty = true;
}

// ***************************************************************************
// *                         EmbeddedResourceManager                         *
// ***************************************************************************
EmbeddedResourceManager::EmbeddedResourceManager()
  : p(unique_ptr<Impl>(new Impl))
{ }

const unique_ptr<EmbeddedResourceManager>&
EmbeddedResourceManager::createInstance()
{
  staticInstance.reset(new EmbeddedResourceManager);
  return staticInstance;
}

const unique_ptr<EmbeddedResourceManager>&
EmbeddedResourceManager::instance()
{
  return staticInstance;
}

string
EmbeddedResourceManager::getLocale() const
{
  return p->getLocale();
}

string
EmbeddedResourceManager::selectLocale(const std::string& locale)
{
  return p->selectLocale(locale);
}

void
EmbeddedResourceManager::addResource(
  const string& virtualPath,
  unique_ptr<const AbstractEmbeddedResource> resourcePtr,
  const string& locale)
{
  p->addResource(virtualPath, std::move(resourcePtr), locale);
}

shared_ptr<const AbstractEmbeddedResource>
EmbeddedResourceManager::getResourceOrNullPtr(const string& virtualPath) const
{
  if (p->dirty) {
    p->rehash();                // update p->poolSearchList
  }

  // Use the selected locale
  return p->lookupResourceInPools(virtualPath, p->poolSearchList);
}

shared_ptr<const AbstractEmbeddedResource>
EmbeddedResourceManager::getResourceOrNullPtr(const string& virtualPath,
                                              const string& locale) const
{
  // In this overload, we don't use the cached list of pools
  // (p->poolSearchList), therefore there is no need to check the 'dirty' flag
  // or to rehash().
  return p->lookupResourceInPools(virtualPath,
                                  p->listOfResourcePoolsToSearch(locale));
}

} // of namespace simgear
