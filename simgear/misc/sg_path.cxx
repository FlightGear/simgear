// sg_path.cxx -- routines to abstract out path separator differences
//               between MacOS and the rest of the world
//
// Written by Curtis L. Olson, started April 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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
// $Id$


#include <simgear/compiler.h>

#include <simgear_config.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/io/iostreams/sgstream.hxx>

#include <cstring>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <fstream>
#include <cstdlib>

#if !defined(SG_WINDOWS)
#  include <sys/types.h>
#  include <utime.h>
#endif

#if defined(SG_WINDOWS)
#  include <direct.h>
#  include <sys/utime.h>
#  include <Shlwapi.h>
#endif

#include "sg_path.hxx"
#include <simgear/misc/sg_dir.hxx>

using std::string;
using simgear::strutils::starts_with;

// Static data members used by SGPath::addAllowedPathPattern(),
// SGPath::validate(), etc.
string_list SGPath::read_allowed_paths;
string_list SGPath::write_allowed_paths;

/**
 * define directory path separators
 */

static const char sgDirPathSep = '/';
static const char sgDirPathSepBad = '\\';

#if defined(SG_WINDOWS)
const char SGPath::pathListSep[] = ";"; // this is null-terminated
#else
const char SGPath::pathListSep[] = ":"; // ditto
#endif

#if defined(SG_WINDOWS)
#include <ShlObj.h>         // for CSIDL
// TODO: replace this include file with the official <versionhelpers.h> header
// included in the Windows 8.1 SDK
#include "sgversionhelpers.hxx"

#define ENABLE_OLD_PATH_API 1

static SGPath pathForCSIDL(int csidl, const SGPath& def)
{
	typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, PWSTR, int, BOOL);
	static GetSpecialFolderPath SHGetSpecialFolderPath = NULL;

	// lazy open+resolve of shell32
	if (!SHGetSpecialFolderPath) {
		HINSTANCE shellDll = ::LoadLibrary("shell32");
		SHGetSpecialFolderPath = (GetSpecialFolderPath) GetProcAddress(shellDll, "SHGetSpecialFolderPathW");
	}

	if (!SHGetSpecialFolderPath){
		return def;
	}

	wchar_t path[MAX_PATH];
	if (SHGetSpecialFolderPath(0, path, csidl, false)) {
		return SGPath(std::wstring(path), def.getPermissionChecker());
	}

	return def;
}

static SGPath pathForKnownFolder(REFKNOWNFOLDERID folderId, const SGPath& def)
{
    typedef HRESULT (WINAPI*PSHGKFP)(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR*);

    HINSTANCE shellDll = LoadLibrary(TEXT("shell32"));
    if (shellDll != NULL) {
        PSHGKFP pSHGetKnownFolderPath = (PSHGKFP) GetProcAddress(shellDll, "SHGetKnownFolderPath");
        if (pSHGetKnownFolderPath != NULL) {
            // system call will allocate dynamic memory... which we must release when done
            wchar_t* localFolder = 0;

			if (pSHGetKnownFolderPath(folderId, KF_FLAG_DONT_VERIFY, NULL, &localFolder) == S_OK) {
                SGPath folder_path = SGPath(localFolder, def.getPermissionChecker());
                // release dynamic memory
                CoTaskMemFree(static_cast<void*>(localFolder));

                return folder_path;
            }
        }

        FreeLibrary(shellDll);
    }

    return def;
}

#elif __APPLE__

// defined in CocoaHelpers.mm
SGPath appleSpecialFolder(int dirType, int domainMask, const SGPath& def);

#else
static SGPath getXDGDir( const std::string& name,
                         const SGPath& def,
                         const std::string& fallback )
{
  // http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html

  // $XDG_CONFIG_HOME defines the base directory relative to which user specific
  // configuration files should be stored. If $XDG_CONFIG_HOME is either not set
  // or empty, a default equal to $HOME/.config should be used.
  const SGPath user_dirs = SGPath::fromEnv( "XDG_CONFIG_HOME",
                                            SGPath::home() / ".config")
                         / "user-dirs.dirs";

  // Format is XDG_xxx_DIR="$HOME/yyy", where yyy is a shell-escaped
  // homedir-relative path, or XDG_xxx_DIR="/yyy", where /yyy is an absolute
  // path. No other format is supported.
  const std::string XDG_ID = "XDG_" + name + "_DIR=\"";

  sg_ifstream user_dirs_file( user_dirs );
  std::string line;
  while( std::getline(user_dirs_file, line).good() )
  {
    if( !starts_with(line, XDG_ID) || *line.rbegin() != '"' )
      continue;

    // Extract dir from XDG_<name>_DIR="<dir>"
    line = line.substr(XDG_ID.length(), line.length() - XDG_ID.length() - 1 );

    const std::string HOME = "$HOME";
    if( starts_with(line, HOME) )
      return SGPath::home(def)
           / simgear::strutils::unescape(line.substr(HOME.length()));

    return SGPath(line, def.getPermissionChecker());
  }

  if( def.isNull() )
    return SGPath::home(def) / fallback;

  return def;
}
#endif

// For windows, replace "\" by "/".
void
SGPath::fix()
{
    string::size_type sz = path.size();
    for ( string::size_type i = 0; i < sz; ++i ) {
        if ( path[i] == sgDirPathSepBad ) {
            path[i] = sgDirPathSep;
        }
    }
    // drop trailing "/"
    while ((sz>1)&&(path[sz-1]==sgDirPathSep))
    {
        path.resize(--sz);
    }
}


// default constructor
SGPath::SGPath(PermissionChecker validator)
    : path(""),
    _permission_checker(validator),
    _cached(false),
    _rwCached(false),
    _cacheEnabled(true),
    _existsCached(false)
{
}


// create a path based on "path"
SGPath::SGPath( const std::string& p, PermissionChecker validator )
    : path(p),
    _permission_checker(validator),
    _cached(false),
    _rwCached(false),
    _cacheEnabled(true),
    _existsCached(false)
{
    fix();
}

// create a path based on "path"
SGPath::SGPath(const std::wstring& p, PermissionChecker validator) :
	_permission_checker(validator),
	_cached(false),
	_rwCached(false),
	_cacheEnabled(true)
{
	path = simgear::strutils::convertWStringToUtf8(p);
	fix();
}


// create a path based on "path" and a "subpath"
SGPath::SGPath( const SGPath& p,
                const std::string& r,
                PermissionChecker validator )
    : path(p.path),
    _permission_checker(validator),
    _cached(false),
    _rwCached(false),
    _cacheEnabled(p._cacheEnabled),
    _existsCached(false)
{
    append(r);
    fix();
}

SGPath SGPath::fromLocal8Bit(const char *name)
{
    return SGPath(simgear::strutils::convertWindowsLocal8BitToUtf8(name));
}

SGPath SGPath::fromUtf8(const std::string& bytes, PermissionChecker p)
{
    return SGPath(bytes, p);
}

// set path
void SGPath::set( const string& p ) {
    path = p;
    fix();
    _cached = false;
    _rwCached = false;
}

//------------------------------------------------------------------------------
void SGPath::setPermissionChecker(PermissionChecker validator)
{
  _permission_checker = validator;
  _rwCached = false;
}

//------------------------------------------------------------------------------
SGPath::PermissionChecker SGPath::getPermissionChecker() const
{
  return _permission_checker;
}

//------------------------------------------------------------------------------
void SGPath::set_cached(bool cached)
{
  _cacheEnabled = cached;
  _cached = false;
}

// ***************************************************************************
// *                    Access permissions for Nasal code                    *
// ***************************************************************************

// Static member function
void SGPath::clearListOfAllowedPaths(bool write)
{
    string_list& allowed_paths(write ? write_allowed_paths : read_allowed_paths);
    allowed_paths.clear();
}

// Static member function
void SGPath::addAllowedPathPattern(const string& pattern, bool write)
{
    string_list& allowed_paths(write ? write_allowed_paths : read_allowed_paths);
    allowed_paths.push_back(pattern);
}

// Static member function
const string_list& SGPath::getListOfAllowedPaths(bool write)
{
    return write ? write_allowed_paths : read_allowed_paths;
}

SGPath SGPath::validate(bool write) const
{
    // Normalize the path (prevents ../../.. or symlink trickery)
    const string normed_path = realpath().utf8Str();

    const string_list& allowed_paths{
        write ? write_allowed_paths : read_allowed_paths};
    string::size_type star_pos;

    // Check against each allowed pattern
    for (const auto& path: allowed_paths) {
        star_pos = path.find('*');
        if (star_pos == string::npos) {
            if (!(path.compare(normed_path))) {
                return fromUtf8(normed_path);
            }
        } else {
            if ((path.size()-1 <= normed_path.size()) /* long enough to be a potential match */
                && !(path.substr(0, star_pos)
                     .compare(normed_path.substr(0, star_pos))) /* before-star parts match */
                && !(path.substr(star_pos+1, path.size()-star_pos-1)
                     .compare(normed_path.substr(star_pos+1+normed_path.size()-path.size(),
                                                 path.size()-star_pos-1))) /* after-star parts match */) {
                return fromUtf8(normed_path);
            }
        }
    }

    // No match found
    return SGPath();
}

// append another piece to the existing path
void SGPath::append( const string& p ) {
    if ( path.empty() ) {
        path = p;
    } else {
    if ( p[0] != sgDirPathSep ) {
        path += sgDirPathSep;
    }
        path += p;
    }
    fix();
    _cached = false;
    _rwCached = false;
}

//------------------------------------------------------------------------------
SGPath SGPath::operator/( const std::string& p ) const
{
  SGPath ret = *this;
  ret.append(p);
  return ret;
}

#if defined(ENABLE_OLD_PATH_API)
//add a new path component to the existing path string
void SGPath::add( const string& p ) {
    append( SGPath::pathListSep[0] + p );
}
#endif

// concatenate a string to the end of the path without inserting a
// path separator
void SGPath::concat( const string& p ) {
    if ( path.empty() ) {
        path = p;
    } else {
        path += p;
    }
    fix();
    _cached = false;
    _rwCached = false;
}

std::string SGPath::local8BitStr() const
{
    return simgear::strutils::convertUtf8ToWindowsLocal8Bit(path);
}

// Get the file part of the path (everything after the last path sep)
string SGPath::file() const
{
    string::size_type index = path.rfind(sgDirPathSep);
    if (index != string::npos) {
        return path.substr(index + 1);
    } else {
        return path;
    }
}


// get the directory part of the path.
string SGPath::dir() const {
    int index = path.rfind(sgDirPathSep);
    if (index >= 0) {
        return path.substr(0, index);
    } else {
        return "";
    }
}

SGPath SGPath::dirPath() const
{
	return SGPath::fromUtf8(dir());
}

// get the base part of the path (everything but the extension.)
string SGPath::base() const
{
    string::size_type index = path.rfind(".");
    string::size_type lastSep = path.rfind(sgDirPathSep);

// tolerate dots inside directory names
    if ((lastSep != string::npos) && (index < lastSep)) {
        return path;
    }

    if (index != string::npos) {
        return path.substr(0, index);
    } else {
        return path;
    }
}

string SGPath::file_base() const
{
    string::size_type index = path.rfind(sgDirPathSep);
    if (index == string::npos) {
        index = 0; // no separator in the name
    } else {
        ++index; // skip past the separator
    }

    string::size_type firstDot = path.find(".", index);
    if (firstDot == string::npos) {
        return path.substr(index); // no extensions
    }

    return path.substr(index, firstDot - index);
}

// get the extension (everything after the final ".")
// but make sure no "/" follows the "." character (otherwise it
// is has to be a directory name containing a ".").
string SGPath::extension() const {
    int index = path.rfind(".");
    if ((index >= 0)  && (path.find("/", index) == string::npos)) {
        return path.substr(index + 1);
    } else {
        return "";
    }
}

string SGPath::lower_extension() const {
    return simgear::strutils::lowercase (extension());
}

string SGPath::complete_lower_extension() const
{
    string::size_type index = path.rfind(sgDirPathSep);
    if (index == string::npos) {
        index = 0; // no separator in the name
    } else {
        ++index; // skip past the separator
    }

    string::size_type firstDot = path.find(".", index);
    if ((firstDot != string::npos)  && (path.find(sgDirPathSep, firstDot) == string::npos)) {
        return simgear::strutils::lowercase (path.substr(firstDot + 1));
    } else {
        return "";
    }
}

//------------------------------------------------------------------------------
void SGPath::updateAttrsIfNull() const
{
  if (_cached && _cacheEnabled) {
    return;
  }

  if (path.empty()) {
	  _exists = false;
      _canWrite = _canRead = false;
	  return;
  }

#if defined(SG_WINDOWS)
  struct _stat buf ;
  bool remove_trailing = false;
  std::wstring statPath(wstr());
  if ((path.length() > 1) && (path.back() == '/')) {
	  statPath.pop_back();
  }

  if (_wstat(statPath.c_str(), &buf ) < 0) {
    _exists = false;
    _canRead = false;

      // check parent directory for write-ability
      std::wstring parentPath = simgear::strutils::convertUtf8ToWString(dir());
      struct _stat parentBuf;
      if (_wstat(parentPath.c_str(), &parentBuf) >= 0) {
          _canWrite = parentBuf.st_mode & _S_IWRITE;
      } else {
          _canWrite = false;
      }
  } else {
    _exists = true;
    _isFile = ((S_IFREG & buf.st_mode ) !=0);
    _isDir = ((S_IFDIR & buf.st_mode ) !=0);
    _modTime = buf.st_mtime;
    _size = buf.st_size;
	_canRead = _S_IREAD & buf.st_mode;
	_canWrite = _S_IWRITE & buf.st_mode;
  }

#else
  struct stat buf ;

  if (stat(path.c_str(), &buf ) < 0) {
    _exists = false;
    _canRead = false;

      // check parent directory for write-ability
      std::string parentPath = dir();
      struct stat parentBuf;
      if (stat(parentPath.c_str(), &parentBuf) >= 0) {
          _canWrite = parentBuf.st_mode & S_IWUSR;
      } else {
          _canWrite = false;
      }
  } else {
    _exists = true;
    _isFile = ((S_ISREG(buf.st_mode )) != 0);
    _isDir = ((S_ISDIR(buf.st_mode )) != 0);
    _modTime = buf.st_mtime;
    _size = buf.st_size;
    _canRead = S_IRUSR & buf.st_mode;
    _canWrite = S_IWUSR & buf.st_mode;
  }

#endif
    // ensure permissions are no less restrictive than what the
    // permissions checker offers
    if ( _permission_checker ) {
        Permissions p = _permission_checker(*this);
        _canRead &= p.read;
        _canWrite &= p.write;
    }

  _cached = true;
}

//------------------------------------------------------------------------------
void SGPath::checkAccess() const
{
  if ( _rwCached && _cacheEnabled ) {
    return;
  }

  updateAttrsIfNull();
  _rwCached = true;
}

bool SGPath::exists() const
{
#if defined(SG_WINDOWS)
  // optimisation: _wstat is slow, eg for TerraSync
  if (!_cached && !_existsCached) {
      std::wstring w(wstr());
    if ((path.length() > 1) && (path.back() == '/')) {
	    w.pop_back();
    }

    _existsCached = true;
    _exists = PathFileExistsW(w.c_str());
    return _exists;
  }
#endif
  updateAttrsIfNull();
  return _exists;
}

//------------------------------------------------------------------------------
bool SGPath::canRead() const
{
  checkAccess();
  return _canRead;
}

//------------------------------------------------------------------------------
bool SGPath::canWrite() const
{
  checkAccess();
  return _canWrite;
}

bool SGPath::isDir() const
{
  updateAttrsIfNull();
  return _exists && _isDir;
}

bool SGPath::isFile() const
{
  updateAttrsIfNull();
  return _exists && _isFile;
}

//------------------------------------------------------------------------------

int SGPath::create_dir(mode_t mode)
{
  if ( !permissionsAllowsWrite() )
  {
    SG_LOG( SG_IO,
            SG_WARN, "Error creating directory for '" << *this << "'"
                                                    " reason: access denied" );
    return -3;
  }

    SGPath dirP = dirPath();
    if (dirP.isNull() )
        return -1;
    string_list path_elements = sgPathBranchSplit(dirP.utf8Str());
	bool absolute = dirP.isAbsolute();
    unsigned int i = 1;

#if defined(SG_WINDOWS)
    SGPath dir(path_elements.front(), _permission_checker);
	// exists() does not work for drive letter paths, eg 'C:\'.
	// Detect this case and skip to the next element immediately
	if (absolute && path_elements.size() >= 2) {
		dir.append(path_elements[i++]);
	}
#else
    SGPath dir((absolute ? "/" : "") + path_elements.front(), _permission_checker);
#endif
	while (dir.exists() && (i < path_elements.size())) {
		dir.append(path_elements[i++]);
	}

	// already exists
	if (dir.exists() && (i == path_elements.size())) {
		return 0;
	}

  for(;;)
  {
#if defined (SG_WINDOWS)
	  std::wstring ds = dir.wstr();
	  if (_wmkdir(ds.c_str()))
#else
	  std::string ds = dir.utf8Str();
    if( mkdir(ds.c_str(), mode) )
#endif
    {
      SG_LOG( SG_IO, SG_ALERT, "Error creating directory: (" << dir << "):"
             << simgear::strutils::error_string(errno) );
        return errno;
    }
    else
      SG_LOG(SG_IO, SG_DEBUG, "Directory created: " << dir);

	if (i >= path_elements.size()) {
		break;
	}

    dir.append(path_elements[i++]);
  }

  _cached = false; // re-stat on next query
  return 0;
}

string_list sgPathBranchSplit( const string &dirpath ) {
    string_list path_elements;
    string element, path = dirpath;
    while ( ! path.empty() ) {
        size_t p = path.find( sgDirPathSep );
        if ( p != string::npos ) {
            element = path.substr( 0, p );
            path.erase( 0, p + 1 );
        } else {
            element = path;
            path = "";
        }
        if ( ! element.empty() )
            path_elements.push_back( element );
    }
    return path_elements;
}


string_list sgPathSplit( const string &search_path ) {
    string tmp = search_path;
    string_list result;
    result.clear();

    bool done = false;

    while ( !done ) {
        int index = tmp.find(SGPath::pathListSep[0]);
        if (index >= 0) {
            result.push_back( tmp.substr(0, index) );
            tmp = tmp.substr( index + 1 );
        } else {
            if ( !tmp.empty() )
                result.push_back( tmp );
            done = true;
        }
    }

    return result;
}

bool SGPath::isAbsolute() const
{
  if (path.empty()) {
    return false;
  }

#ifdef _WIN32
  // detect '[A-Za-z]:/'
  if (path.size() > 2) {
    if (isalpha(path[0]) && (path[1] == ':') && (path[2] == sgDirPathSep)) {
      return true;
    }
  }
#endif

  return (path[0] == sgDirPathSep);
}

bool SGPath::isNull() const
{
  return path.empty();
}

#if defined(ENABLE_OLD_PATH_API)
std::string SGPath::str_native() const
{
#ifdef _WIN32
    std::string s = local8BitStr();
    std::string::size_type pos;
    std::string nativeSeparator;
    nativeSeparator = sgDirPathSepBad;

    while( (pos=s.find( sgDirPathSep )) != std::string::npos ) {
        s.replace( pos, 1, nativeSeparator );
    }
    return s;
#else
    return utf8Str();
#endif
}
#endif

//------------------------------------------------------------------------------
bool SGPath::remove()
{
  if( !permissionsAllowsWrite() )
  {
    SG_LOG( SG_IO, SG_WARN, "file remove failed: (" << *this << ")"
                                               " reason: access denied" );
    return false;
  }

#if defined(SG_WINDOWS)
  std::wstring ps = wstr();

  // windows forbids removing a read-only file, let's try to deal
  // with that case
  int err = _wchmod(ps.c_str(), _S_IWRITE | _S_IREAD);
  if (err != 0) {
	  SG_LOG(SG_IO, SG_WARN, "failed to make file writeable prior to remove:" << *this);
  } else {
	  err = _wunlink(ps.c_str());
  }
#else
  std::string ps = local8BitStr();
  int err = ::unlink(ps.c_str());
#endif
  if( err )
  {
    SG_LOG( SG_IO, SG_WARN, "file remove failed: (" << *this << ") "
                                               " reason: " << strerror(errno) );
    // TODO check if failed unlink can really change any of the cached values
  }

  _cached = false; // stat again if required
  _rwCached = false;
  return (err == 0);
}

time_t SGPath::modTime() const
{
    updateAttrsIfNull();
    return _modTime;
}

size_t SGPath::sizeInBytes() const
{
    updateAttrsIfNull();
    return _size;
}

bool SGPath::operator==(const SGPath& other) const
{
    return (path == other.path);
}

bool SGPath::operator!=(const SGPath& other) const
{
    return (path != other.path);
}

bool operator<(const SGPath& lhs, const SGPath& rhs)
{ return lhs.path < rhs.path; }

bool operator>(const SGPath& lhs, const SGPath& rhs)
{ return operator<(rhs, lhs); }

bool operator<=(const SGPath& lhs, const SGPath& rhs)
{ return !operator>(lhs, rhs); }

bool operator>=(const SGPath& lhs, const SGPath& rhs)
{ return !operator<(lhs, rhs); }

//------------------------------------------------------------------------------
bool SGPath::rename(const SGPath& newName)
{
  if( !canRead() || !canWrite() || !newName.canWrite() )
  {
    SG_LOG( SG_IO, SG_WARN, "rename failed: from " << *this <<
                                            " to " << newName <<
                                            " reason: access denied" );
    return false;
  }

#ifdef SG_WINDOWS
	if (newName.exists()) {
		SGPath r(newName);
		if (!r.remove()) {
			return false;
		}
	}
#endif

#if defined(SG_WINDOWS)
	std::wstring p = wstr();
	std::wstring np = newName.wstr();
	if (_wrename(p.c_str(), np.c_str()) != 0)
#else
    std::string p = utf8Str();
    std::string np = newName.utf8Str();
    if( ::rename(p.c_str(), np.c_str()) != 0 )
#endif

  {
    SG_LOG( SG_IO, SG_WARN, "rename failed: from " << *this <<
                                            " to " << newName <<
                                            " reason: " << strerror(errno) );
    return false;
  }

  path = newName.path;

  // Do not remove permission checker (could happen for example if just using
  // a std::string as new name)
  if( newName._permission_checker )
    _permission_checker = newName._permission_checker;

  _cached = false;
  _rwCached = false;

  return true;
}

//------------------------------------------------------------------------------
SGPath SGPath::standardLocation(StandardLocation type, const SGPath& def)
{
  switch(type)
  {
    case HOME:
      return home(def);

#ifdef _WIN32
    case DESKTOP:
        if (IsWindowsVistaOrGreater())
            return pathForKnownFolder(FOLDERID_Desktop, def);

        return pathForCSIDL(CSIDL_DESKTOPDIRECTORY, def);

    case DOWNLOADS:
        if (IsWindowsVistaOrGreater())
            return pathForKnownFolder(FOLDERID_Downloads, def);

        if (!def.isNull())
            return def;

        return pathForCSIDL(CSIDL_DESKTOPDIRECTORY, def);

    case DOCUMENTS:
        if (IsWindowsVistaOrGreater())
            return pathForKnownFolder(FOLDERID_Documents, def);

        return pathForCSIDL(CSIDL_MYDOCUMENTS, def);

    case PICTURES:
        if (IsWindowsVistaOrGreater())
            return pathForKnownFolder(FOLDERID_Pictures, def);

        return pathForCSIDL(CSIDL_MYPICTURES, def);

#elif __APPLE__
      // since this is C++, we can't include NSPathUtilities.h to access the enum
      // values, so hard-coding them here (they are stable, don't worry)
    case DOWNLOADS:
      return appleSpecialFolder(15, 1, def);
    case DESKTOP:
      return appleSpecialFolder(12, 1, def);
    case DOCUMENTS:
      return appleSpecialFolder(9, 1, def);
    case PICTURES:
      return appleSpecialFolder(19, 1, def);
#else
    case DESKTOP:
      return getXDGDir("DESKTOP", def, "Desktop");
    case DOWNLOADS:
      return getXDGDir("DOWNLOADS", def, "Downloads");
    case DOCUMENTS:
      return getXDGDir("DOCUMENTS", def, "Documents");
    case PICTURES:
      return getXDGDir("PICTURES", def, "Pictures");
#endif
    default:
      SG_LOG( SG_GENERAL,
              SG_WARN,
              "SGPath::standardLocation() unhandled type: " << type );
      return def;
  }
}

//------------------------------------------------------------------------------
SGPath SGPath::fromEnv(const char* name, const SGPath& def)
{
#if defined(SG_WINDOWS)
	std::wstring wname = simgear::strutils::convertUtf8ToWString(name);
	const wchar_t* val = _wgetenv(wname.c_str());
	if (val && val[0])
		return SGPath(val, def._permission_checker);
#else
  const char* val = getenv(name);
  if( val && val[0] )
    return SGPath(val, def._permission_checker);
#endif
  return def;
}

//------------------------------------------------------------------------------

std::vector<SGPath> SGPath::pathsFromEnv(const char *name)
{
    std::vector<SGPath> r;
#if defined(SG_WINDOWS)
	std::wstring wname = simgear::strutils::convertUtf8ToWString(name);
	const wchar_t* val = _wgetenv(wname.c_str());
#else
	const char* val = getenv(name);
#endif
	if (!val) {
		return r;
	}
   
#if defined(SG_WINDOWS)
	return pathsFromUtf8(simgear::strutils::convertWStringToUtf8(val));
#else
	return pathsFromUtf8(val);
#endif
}

//------------------------------------------------------------------------------

std::vector<SGPath> SGPath::pathsFromUtf8(const std::string& paths)
{
	std::vector<SGPath> r;
	string_list items = sgPathSplit(paths);
	string_list_iterator it;
	for (it = items.begin(); it != items.end(); ++it) {
		r.push_back(SGPath::fromUtf8(it->c_str()));
	}

	return r;
}

//------------------------------------------------------------------------------

std::vector<SGPath> SGPath::pathsFromLocal8Bit(const std::string& paths)
{
    std::vector<SGPath> r;
    string_list items =  sgPathSplit(paths);
    string_list_iterator it;
    for (it = items.begin(); it != items.end(); ++it) {
        r.push_back(SGPath::fromLocal8Bit(it->c_str()));
    }

    return r;
}

//------------------------------------------------------------------------------
SGPath SGPath::home(const SGPath& def)
{
#ifdef _WIN32
    return fromEnv("USERPROFILE", def);
#else
    return fromEnv("HOME", def);
#endif
}

//------------------------------------------------------------------------------
SGPath SGPath::desktop(const SGPath& def)
{
  return standardLocation(DESKTOP, def);
}

//------------------------------------------------------------------------------
SGPath SGPath::documents(const SGPath& def)
{
  return standardLocation(DOCUMENTS, def);
}

//------------------------------------------------------------------------------
SGPath SGPath::realpath() const
{
#if defined(SG_WINDOWS)
    // with absPath NULL, will allocate, and ignore length
  	std::wstring ws = wstr();
    wchar_t *buf = _wfullpath( NULL, ws.c_str(), _MAX_PATH );
#else
    // POSIX
    char* buf = ::realpath(path.c_str(), NULL);
#endif
    if (!buf) // File does not exist: return the realpath it would have if created now
              // (needed for SGPath::validate() security)
    {
        if (path.empty()) {
            return simgear::Dir::current().path();
        }

        std::string this_dir = dir();
        if (isAbsolute() && this_dir.empty()) { // top level
            this_dir = "/";
        }
        if (file() == "..") {
            this_dir = SGPath(this_dir).realpath().dir();
            if (this_dir.empty()) { // invalid path: .. above root
                return SGPath();
            }
            return SGPath(this_dir).realpath(); // use native path separator,
                        // and handle 'existing/nonexisting/../symlink' paths
        }
        return SGPath(this_dir).realpath() / file();
    }

#if defined(SG_WINDOWS)
	  SGPath p = SGPath(std::wstring(buf), NULL);
#else
		SGPath p(SGPath::fromLocal8Bit(buf));
#endif
    free(buf);
    return p;

}

//------------------------------------------------------------------------------

std::string SGPath::join(const std::vector<SGPath>& paths, const std::string& joinWith)
{
    std::string r;
    if (paths.empty()) {
        return r;
    }

    r = paths[0].utf8Str();
    for (size_t i=1; i<paths.size(); ++i) {
        r += joinWith + paths[i].utf8Str();
    }

    return r;
}

//------------------------------------------------------------------------------
std::wstring SGPath::wstr() const
{
	return simgear::strutils::convertUtf8ToWString(path);
}

//------------------------------------------------------------------------------
bool SGPath::permissionsAllowsWrite() const
{
    return _permission_checker ? _permission_checker(*this).write : true;
}

//------------------------------------------------------------------------------
std::string SGPath::fileUrl() const
{
    // we should really URL encode the names here?
    if (isAbsolute()) {
        // check for a windows drive letter
#if defined(SG_WINDOWS)
        if (isalpha(path.front())) {
            // file URLs on Windows must look like file:///C:/Foo/Bar
            return "file:///" + utf8Str();
        }
#endif
        // the leading directory seperator of the path becomes the required
        // third slash in this case.
        return "file://" + utf8Str();
    } else {
        SG_LOG(SG_GENERAL, SG_WARN, "Cannot convert relative path to a URL:" << path);
        return {};
    }
}

//------------------------------------------------------------------------------
bool SGPath::touch()
{
    if (!permissionsAllowsWrite())
    {
        SG_LOG(SG_IO, SG_WARN, "file touch failed: (" << *this << ")"
               " reason: access denied" );
        return false;
    }
    
    if (!exists()) {
        SG_LOG(SG_IO, SG_WARN, "file touch failed: (" << *this << ")"
               " reason: missing file");
        return false;
    }
#if defined(SG_WINDOWS)
    auto ws = wstr();
    // set this link for docs on behaviour here, about passing nullptr
    // https://msdn.microsoft.com/en-us/library/aa273399(v=vs.60).aspx
    if (_wutime(ws.c_str(), nullptr) != 0) {
        SG_LOG(SG_IO, SG_WARN, "file touch failed: (" << *this << ")"
               " reason: _wutime failed with error:" << simgear::strutils::error_string(errno));
        return false;
    }
#else
    if (::utime(path.c_str(), nullptr) != 0) {
        SG_LOG(SG_IO, SG_WARN, "file touch failed: (" << *this << ")"
               " reason: utime failed with error:" << simgear::strutils::error_string(errno));
        return false;
    }
#endif
    
    // reset the cache flag so we re-stat() on next request
    _cached = false;
    return true;
}
