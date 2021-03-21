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

#ifndef SG_PACKAGE_PACKAGE_HXX
#define SG_PACKAGE_PACKAGE_HXX

#include <set>
#include <vector>

#include <simgear/props/props.hxx>
#include <simgear/misc/strutils.hxx>

#include <simgear/structure/function_list.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include <simgear/package/PackageCommon.hxx>

typedef std::set<std::string> string_set;

namespace simgear
{

namespace pkg
{
class Package : public SGReferenced
{
public:

    typedef std::function<void(Package*, Install*)> InstallCallback;

    /**
     * get or create an install for the package
     */
    InstallRef install();

    InstallRef
    existingInstall(const InstallCallback& cb = InstallCallback()) const;

    /**
     * Mark this package for installation, but don't actually start the
     * download process. This creates the on-disk placeholder, so
     * the package will appear an eededing to be updated.
     */
    InstallRef markForInstall();

    bool isInstalled() const;

    /**
     * package ID
     */
    std::string id() const;

    /**
     * Variant IDs
     */
    string_list variants() const;

    /**
     * All variants without a parent, i.e top-level variants in this package.
     * Often this is a single-element list matching id() above, but when
     * packages contain multiple primary aircraft, this will have multiple
     * elements.
     */
    string_list primaryVariants() const;

    /**
     * Fully-qualified ID, including our catalog'd ID
     */
    std::string qualifiedId() const;

    /**
     * Fully-qualified ID, including our catalog'd ID
     */
    std::string qualifiedVariantId(const unsigned int variantIndex) const;

    /**
     *
     */
    unsigned int indexOfVariant(const std::string& vid) const;

    /**
     * human-readable name - note this is probably not localised,
     * although this is not ruled out for the future.
     */
    std::string name() const;

    /**
     * Human readable name of a variant
     */
    std::string nameForVariant(const std::string& vid) const;

    std::string nameForVariant(const unsigned int vIndex) const;

    /**
     * syntactic sugar to get the localised description of the main aircraft
     */
    std::string description() const;

    /**
     * access the raw property data in the package
     */
    SGPropertyNode* properties() const;

    /**
     * hex-encoded MD5 sum of the download files
     */
    std::string md5() const;

    std::string getLocalisedProp(const std::string& aName, const unsigned int vIndex = 0) const;

    unsigned int revision() const;

    size_t fileSizeBytes() const;

    CatalogRef catalog() const;

    bool matches(const SGPropertyNode* aFilter) const;

    string_set tags() const;

    /**
     * @brief hasTag - efficently check if a tag is defined or not
     * @param tag
     * @return
     */
    bool hasTag(const std::string& tag) const;
    
    /**
     * download URLs for the package
     */
    string_list downloadUrls() const;
    string_list thumbnailUrls() const;

    struct Thumbnail
    {
        std::string url;
        std::string path;
    };

    Thumbnail thumbnailForVariant(unsigned int vIndex) const;

    /**
     * information about a preview image
     */
    struct Preview {
        enum class Type
        {
            UNKNOWN,
            PANEL,
            INTERIOR,
            EXTERIOR

            // NIGHT / GROUND as modifiers? does this add any
            // actual value for GUIs?
        };

        Preview(const std::string& url, const std::string& path, Type ty = Type::UNKNOWN);

        std::string url;
        std::string path;
        Type type = Type::UNKNOWN;
    };

    typedef std::vector<Preview> PreviewVec;

    /**
     * retrieve all the thumbnails for a variant
     */
    PreviewVec previewsForVariant(unsigned int vIndex) const;

    /**
     * Packages we depend upon.
     * If the dependency list cannot be satisifed for some reason,
     * this will raise an sg_exception.
     */
    PackageList dependencies() const;

    /**
     * Name of the package directory on disk. This may or may not be the
     * same as the primary ID, depending on the aircraft author
     */
    std::string dirName() const;

    /**
     * Return the parent variant of a variant. This will be the emtpy string if
     * the variant is primary (top-level), otherwise the local (non-qualified)
     * ID. This allows establishing a heirarchy of variants within the package.
     * Note at present most code assumes a maxiumum two-level deep heirarchy
     * (parents and children)
     */
    std::string parentIdForVariant(unsigned int variantIndex) const;

    Type type() const;

    static std::string directoryForType(Type type);

    /**
     @brief key files provided by this package. Optional list of externally interesting files
     within this package, relative to the package root.
     */
    string_list providesPaths() const;

    bool doesProvidePath(const std::string& p) const;

private:
    SGPath pathOnDisk() const;

    friend class Catalog;
    friend class Root;

    Package(const SGPropertyNode* aProps, CatalogRef aCatalog);

    void initWithProps(const SGPropertyNode* aProps);

    void updateFromProps(const SGPropertyNode* aProps);

    /**
     * @brief check the Package passes some basic consistence checks
     */
    bool validate() const;
    
    std::string getLocalisedString(const SGPropertyNode* aRoot, const char* aName, bool* isLocalized = nullptr) const;

    PreviewVec previewsFromProps(const SGPropertyNode_ptr& ptr) const;

    SGPropertyNode_ptr propsForVariant(const unsigned int vIndex, const char* propName = nullptr) const;

    bool matchesDescription(const std::string &search) const;
    
    SGPropertyNode_ptr m_props;
    Type m_type;
    std::string m_id;
    string_set m_tags;
    Catalog* m_catalog = nullptr; // non-owning ref, explicitly
    string_list m_variants;
    string_list m_provides;

    mutable function_list<InstallCallback> _install_cb;
};




} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_PACKAGE_HXX
