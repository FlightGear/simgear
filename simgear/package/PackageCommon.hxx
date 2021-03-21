// Copyright (C) 2021 James Turner - james@flightgear.org
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
