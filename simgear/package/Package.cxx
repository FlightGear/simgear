// Copyright (C) 2013  James Turner - zakalawe@mac.com
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <simgear_config.h>

#include <simgear/package/Package.hxx>

#include <algorithm>
#include <cassert>

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>

#include <simgear/package/Catalog.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Root.hxx>

namespace simgear {

namespace pkg {

Package::Package(const SGPropertyNode* aProps, CatalogRef aCatalog) :
    m_catalog(aCatalog.get())
{
    initWithProps(aProps);
}

void Package::initWithProps(const SGPropertyNode* aProps)
{
    m_props = const_cast<SGPropertyNode*>(aProps);
// cache tag values
    for (auto c : aProps->getChildren("tag")) {
      m_tags.insert (strutils::lowercase (c->getStringValue()));
    }

    m_id = m_props->getStringValue("id");

    m_variants.push_back(m_id);
    for (auto var : m_props->getChildren("variant")) {
        m_variants.push_back(var->getStringValue("id"));
    }
}

void Package::updateFromProps(const SGPropertyNode* aProps)
{
    m_tags.clear();
    m_variants.clear();
    initWithProps(aProps);
}

bool Package::matches(const SGPropertyNode* aFilter) const
{
    const std::string& filter_name = aFilter->getNameString();

    if (filter_name == "any-of") {
        const int anyChildren = aFilter->nChildren();
        for (int j = 0; j < anyChildren; j++) {
            const SGPropertyNode* anyChild = aFilter->getChild(j);
            if (matches(anyChild)) {
                return true;
            }
        }

        return false; // none of our children matched
    } else if (filter_name.empty() || (filter_name == "all-of")) {
        const int allChildren = aFilter->nChildren();
        for (int j = 0; j < allChildren; j++) {
            const SGPropertyNode* allChild = aFilter->getChild(j);
            if (!matches(allChild)) {
                return false;
            }
        }

        return true; // all of our children matched
    }

    if (strutils::starts_with(filter_name, "rating-")) {
        int minRating = aFilter->getIntValue();
        std::string rname = aFilter->getName() + 7;
        int ourRating = m_props->getChild("rating")->getIntValue(rname, 0);
        return (ourRating >= minRating);
    }

    if (filter_name == "tag") {
        const std::string tag = strutils::lowercase (aFilter->getStringValue());
        return (m_tags.find(tag) != m_tags.end());
    }

    if (filter_name == "installed") {
        return (isInstalled() == aFilter->getBoolValue());
    }

    bool handled = false;
    // substring search of name, description, across variants too
    if ((filter_name == "text") || (filter_name == "name")) {
        handled = true;
      const std::string n = strutils::lowercase (aFilter->getStringValue());

      const size_t pos = strutils::lowercase (name()).find(n);
      if (pos != std::string::npos) {
        return true;
      }

      for (auto var : m_props->getChildren("variant")) {
          if (var->hasChild("name")) {
              const std::string variantName = strutils::lowercase (var->getStringValue("name"));
              size_t pos = variantName.find(n);
              if (pos != std::string::npos) {
                  return true;
              }
          }
      }
    }

    if ((filter_name == "text") || (filter_name == "description")) {
        handled = true;
        if (matchesDescription(aFilter->getStringValue())) {
            return true;
        }
    }

    if (!handled) {
      SG_LOG(SG_GENERAL, SG_WARN, "unknown filter term:" << filter_name);
    }

    return false;
}

bool Package::matchesDescription(const std::string &search) const
{
    const std::string n = strutils::lowercase (search);

    bool localized;
    const auto d = strutils::lowercase (getLocalisedString(m_props, "description", &localized));
    if (d.find(n) != std::string::npos) {
        return true;
    }

    // try non-localized description too, if the abovce was a localized one
    if (localized) {
        const std::string baseDesc = m_props->getStringValue("description");
        const auto pos = strutils::lowercase (baseDesc).find(n);
        if (pos != std::string::npos) {
            return true;
        }
    }

    // try each variant's description
    for (auto var : m_props->getChildren("variant")) {
        const auto vd = strutils::lowercase (getLocalisedString(var, "description", &localized));
        if (!vd.empty()) {
            if (vd.find(n) != std::string::npos) {
                return true;
            }
        }

        if (localized) {
            // try non-localized variant description
            const std::string vd = strutils::lowercase (var->getStringValue("description"));
            if (vd.find(n) != std::string::npos) {
                return true;
            }
        }
    } // of variant iteration

    return false;
}

bool Package::isInstalled() const
{
    // anything to check for? look for a valid revision file?
    return pathOnDisk().exists();
}

SGPath Package::pathOnDisk() const
{
    SGPath p(m_catalog->installRoot());
    p.append("Aircraft");
    p.append(dirName());
    return p;
}

InstallRef Package::install()
{
    InstallRef ins = existingInstall();
    if (ins) {
      // if there's updates, treat this as a 'start update' request
      if (ins->hasUpdate()) {
        m_catalog->root()->scheduleToUpdate(ins);
      }

        return ins;
    }

  // start a new install
    ins = new Install(this, pathOnDisk());
    m_catalog->root()->scheduleToUpdate(ins);

    _install_cb(this, ins);

    return ins;
}

InstallRef Package::markForInstall() {
  InstallRef ins = existingInstall();
  if (ins) {
    return ins;
  }

  const auto pd = pathOnDisk();

  Dir dir(pd);
  if (!dir.create(0700)) {
    SG_LOG(SG_IO, SG_ALERT,
           "Package::markForInstall: couldn't create directory at:" << pd);
    return {};
  }

  ins = new Install{this, pd};
  _install_cb(this, ins); // not sure if we should trigger the callback for this

  // repeat for dependencies to be kind
  for (auto dep : dependencies()) {
    dep->markForInstall();
  }

  return ins;
}

InstallRef Package::existingInstall(const InstallCallback& cb) const
{
    InstallRef install;
    try {
        install = m_catalog->root()->existingInstallForPackage(const_cast<Package*>(this));
    } catch (std::exception& ) {
      return {};
    }

  if( cb )
  {
    _install_cb.push_back(cb);

    if( install )
      cb(const_cast<Package*>(this), install);
  }

  return install;
}

std::string Package::id() const
{
    return m_id;
}

CatalogRef Package::catalog() const
{
    return {m_catalog};
}

std::string Package::qualifiedId() const
{
    return m_catalog->id() + "." + id();
}

std::string Package::qualifiedVariantId(const unsigned int variantIndex) const
{
    if (variantIndex >= m_variants.size()) {
        throw sg_range_exception("invalid variant index " + std::to_string(variantIndex));
    }
    return m_catalog->id() + "." + m_variants[variantIndex];
}

std::string Package::md5() const
{
    return m_props->getStringValue("md5");
}

std::string Package::dirName() const
{
    std::string r(m_props->getStringValue("dir"));
    if (r.empty())
        throw sg_exception("missing dir property on catalog package entry for " + m_id);
    return r;
}

unsigned int Package::revision() const
{
    if (!m_props) {
        return 0;
    }

    return m_props->getIntValue("revision");
}

std::string Package::name() const
{
    return m_props->getStringValue("name");
}

size_t Package::fileSizeBytes() const
{
    return m_props->getIntValue("file-size-bytes");
}

std::string Package::description() const
{
    return getLocalisedProp("description", 0);
}

string_set Package::tags() const
{
    return m_tags;
}

bool Package::hasTag(const std::string& tag) const
{
    return m_tags.find(tag) != m_tags.end();
}

SGPropertyNode* Package::properties() const
{
    return m_props.ptr();
}

string_list Package::thumbnailUrls() const
{
    string_list urls;
    const Thumbnail& thumb(thumbnailForVariant(0));
    if (!thumb.url.empty()) {
        urls.push_back(thumb.url);
    }
    return urls;
}

string_list Package::downloadUrls() const
{
    string_list r;
    if (!m_props) {
        return r;
    }

    for (auto dl : m_props->getChildren("url")) {
        r.push_back(dl->getStringValue());
    }
    return r;
}

std::string Package::getLocalisedProp(const std::string& aName, const unsigned int vIndex) const
{
    return getLocalisedString(propsForVariant(vIndex, aName.c_str()), aName.c_str());
}

std::string Package::getLocalisedString(const SGPropertyNode* aRoot, const char* aName, bool* isLocalized) const
{
    // we used to place localised strings under /sim/<locale>/name - but this
    // potentially pollutes the /sim namespace
    // we now check first in /sim/localized/<locale>/name first
    const auto& locale = m_catalog->root()->getLocale();
    if (isLocalized) *isLocalized = false;

    if (locale.empty()) {
        return aRoot->getStringValue(aName);
    }

    const SGPropertyNode* localeRoot;
    if (aRoot->hasChild("localized")) {
        localeRoot = aRoot->getChild("localized")->getChild(locale);
    } else {
        // old behaviour where locale nodes are directly beneath /sim
        localeRoot = aRoot->getChild(locale);
    }

    if (localeRoot && localeRoot->hasChild(aName)) {
        if (isLocalized) *isLocalized = true;
        return localeRoot->getStringValue(aName);
    }

    return aRoot->getStringValue(aName);
}

PackageList Package::dependencies() const
{
    PackageList result;

    for (auto dep : m_props->getChildren("depends")) {
        std::string depName = dep->getStringValue("id");
        unsigned int rev = dep->getIntValue("revision", 0);

    // prefer local hangar package if possible, in case someone does something
    // silly with naming. Of course flightgear's aircraft search doesn't know
    // about hangars, so names still need to be unique.
        PackageRef depPkg = m_catalog->getPackageById(depName);
        if (!depPkg) {
            Root* rt = m_catalog->root();
            depPkg = rt->getPackageById(depName);
            if (!depPkg) {
                throw sg_exception("Couldn't satisfy dependency of " + id() + " : " + depName);
            }
        }

        if (depPkg->revision() < rev) {
            throw sg_range_exception("Couldn't find suitable revision of " + depName);
        }

    // forbid recursive dependency graphs, we don't need that level
    // of complexity for aircraft resources
        assert(depPkg->dependencies() == PackageList());

        result.push_back(depPkg);
    }

    return result;
}

string_list Package::variants() const
{
    return m_variants;
}

std::string Package::nameForVariant(const std::string& vid) const
{
    if (vid == id()) {
        return name();
    }

    for (auto var : m_props->getChildren("variant")) {
        if (vid == var->getStringValue("id")) {
            return var->getStringValue("name");
        }
    }


    throw sg_exception("Unknow variant +" + vid + " in package " + id());
}

unsigned int Package::indexOfVariant(const std::string& vid) const
{
    // accept fully-qualified IDs here
    std::string actualId = vid;
    size_t lastDot = vid.rfind('.');
    if (lastDot != std::string::npos) {
        std::string catalogId = vid.substr(0, lastDot);
        if (catalogId != catalog()->id()) {
            throw sg_exception("Bad fully-qualified ID:" + vid + ", package mismatch" );
        }
        actualId = vid.substr(lastDot + 1);
    }

    string_list::const_iterator it = std::find(m_variants.begin(), m_variants.end(), actualId);
    if (it == m_variants.end()) {
        throw sg_exception("Unknow variant " + vid + " in package " + id());
    }

    return std::distance(m_variants.begin(), it);
}

std::string Package::nameForVariant(const unsigned int vIndex) const
{
    return propsForVariant(vIndex, "name")->getStringValue("name");
}

SGPropertyNode_ptr Package::propsForVariant(const unsigned int vIndex, const char* propName) const
{
    if (vIndex == 0) {
        return m_props;
    }

    // offset by minus one to allow for index 0 being the primary
    SGPropertyNode_ptr var = m_props->getChild("variant", vIndex - 1);
    if (var) {
        if (!propName || var->hasChild(propName)) {
            return var;
        }

        return m_props;
    }

    throw sg_exception("Unknown variant in package " + id());
}

std::string Package::parentIdForVariant(unsigned int variantIndex) const
{
    const std::string parentId = propsForVariant(variantIndex)->getStringValue("variant-of");
    if ((variantIndex == 0) || (parentId == "_package_")) {
        return std::string();
    }

    if (parentId.empty()) {
        // this is a variant without a variant-of, so assume its parent is
        // the first primary
        return m_variants.front();
    }

    assert(indexOfVariant(parentId) >= 0);
    return parentId;
}

string_list Package::primaryVariants() const
{
    string_list result;
    for (unsigned int v = 0; v < m_variants.size(); ++v) {
        const auto pr = parentIdForVariant(v);
        if (pr.empty()) {
            result.push_back(m_variants.at(v));
        }
    }
    assert(!result.empty());
    assert(result.front() == id());
    return result;
}

Package::Thumbnail Package::thumbnailForVariant(unsigned int vIndex) const
{
    SGPropertyNode_ptr var = propsForVariant(vIndex);
    // allow for variants without distinct thumbnails
    if (!var->hasChild("thumbnail") || !var->hasChild("thumbnail-path")) {
        var = m_props;
    }

    return {var->getStringValue("thumbnail"), var->getStringValue("thumbnail-path")};
}

Package::PreviewVec Package::previewsForVariant(unsigned int vIndex) const
{
    SGPropertyNode_ptr var = propsForVariant(vIndex);
    return previewsFromProps(var);
}

Package::Preview::Type previewTypeFromString(const std::string& s)
{
    if (s == "exterior") return Package::Preview::Type::EXTERIOR;
    if (s == "interior") return Package::Preview::Type::INTERIOR;
    if (s == "panel") return Package::Preview::Type::PANEL;
    return Package::Preview::Type::UNKNOWN;
}

Package::Preview::Preview(const std::string& aUrl, const std::string& aPath, Type aType) :
    url(aUrl),
    path(aPath),
    type(aType)
{
}

Package::PreviewVec Package::previewsFromProps(const SGPropertyNode_ptr& ptr) const
{
    PreviewVec result;

    for (auto thumbNode : ptr->getChildren("preview")) {
        Preview t(thumbNode->getStringValue("url"),
                    thumbNode->getStringValue("path"),
                    previewTypeFromString(thumbNode->getStringValue("type")));
        result.push_back(t);
    }

    return result;
}

bool Package::validate() const
{
    if (m_id.empty())
        return false;

    std::string nm(m_props->getStringValue("name"));
    if (nm.empty())
        return false;

    std::string dir(m_props->getStringValue("dir"));
    if (dir.empty())
        return false;

    return true;
}


} // of namespace pkg

} // of namespace simgear
