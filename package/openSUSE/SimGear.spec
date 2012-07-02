Summary:    Simulator Construction Gear
Name:       SimGear
Version:    2.8.0
Release:    1
License:    LGPL
URL:        http://www.flightgear.org
Group:      Amusements/Games/3D/Simulation
Source:     http://mirrors.ibiblio.org/pub/mirrors/flightgear/ftp/Source/simgear-%{version}.tar.bz2
BuildRoot:  %{_tmppath}/%{name}-%{version}-build

BuildRequires: gcc, gcc-c++, cmake
BuildRequires: freealut, freealut-devel
BuildRequires: libopenal1-soft, openal-soft
BuildRequires: plib-devel >= 1.8.5
BuildRequires: libOpenSceneGraph-devel >= 3.0
BuildRequires: zlib, zlib-devel
BuildRequires: libjpeg62, libjpeg62-devel
BuildRequires: boost-devel >= 1.37
BuildRequires: subversion-devel, libapr1-devel
Requires:      OpenSceneGraph-plugins >= 3.0

%description
This package contains a tools and libraries useful for constructing
simulation and visualization applications such as FlightGear or TerraGear.

%package devel
Group:         Development/Libraries/Other
Summary:       Development header files for SimGear
Requires:      SimGear = %{version}
Requires:      plib-devel

%description devel
Development headers and libraries for building applications against SimGear.

%prep
%setup -T -q -n simgear-%{version} -b 0

%build
export CFLAGS="$RPM_OPT_FLAGS"
export CXXFLAGS="$RPM_OPT_FLAGS"
# build SHARED simgear libraries
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -DSIMGEAR_SHARED:BOOL=ON -DENABLE_TESTS:BOOL=OFF -DJPEG_FACTORY:BOOL=ON
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr (-, root, root, -)
%doc AUTHORS COPYING ChangeLog NEWS README
%{_libdir}/libSimGear*.so.*

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}/simgear
%{_includedir}/simgear/*
%{_libdir}/libSimGear*.so

%changelog
* Mon Jul 02 2012 thorstenb@flightgear.org
- Initial version

