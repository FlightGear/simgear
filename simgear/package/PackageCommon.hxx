// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) 2021  James Turner - james@flightgear.org


#pragma once

#include <vector>

#include <simgear/structure/SGSharedPtr.hxx>

namespace simgear {

namespace pkg {

enum Type {
    AircraftPackage = 0,
    AIModelPackage,
    AddOnPackage,
    LibraryPackage, ///< common files for use by other package(s) (eg, the DavePack)

    // if you extend this enum, extend the static_typeNames string array
    // in Package.cxx file as well.

    AnyPackageType = 9999
};

// forward decls
class Install;
class Catalog;
class Package;
class Root;

typedef SGSharedPtr<Package> PackageRef;
typedef SGSharedPtr<Catalog> CatalogRef;
typedef SGSharedPtr<Install> InstallRef;

typedef std::vector<PackageRef> PackageList;
typedef std::vector<CatalogRef> CatalogList;

} // namespace pkg

} // namespace simgear
