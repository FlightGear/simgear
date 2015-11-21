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
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <fstream>

#ifdef _WIN32
#  include <direct.h>
#endif
#include "sg_path.hxx"

#include <boost/algorithm/string/case_conv.hpp>

using std::string;
using simgear::strutils::starts_with;

/**
 * define directory path separators
 */

static const char sgDirPathSep = '/';
static const char sgDirPathSepBad = '\\';

#ifdef _WIN32
static const char sgSearchPathSep = ';';
#else
static const char sgSearchPathSep = ':';
#endif

#ifdef _WIN32
#include <ShlObj.h>         // for CSIDL
// TODO: replace this include file with the official <versionhelpers.h> header
// included in the Windows 8.1 SDK
#include "sgversionhelpers.hxx"

static SGPath pathForCSIDL(int csidl, const SGPath& def)
{
	typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, LPSTR, int, BOOL);
	static GetSpecialFolderPath SHGetSpecialFolderPath = NULL;

	// lazy open+resolve of shell32
	if (!SHGetSpecialFolderPath) {
		HINSTANCE shellDll = ::LoadLibrary("shell32");
		SHGetSpecialFolderPath = (GetSpecialFolderPath) GetProcAddress(shellDll, "SHGetSpecialFolderPathA");
	}

	if (!SHGetSpecialFolderPath){
		return def;
	}

	char path[MAX_PATH];
	if (SHGetSpecialFolderPath(0, path, csidl, false)) {
		return SGPath(path, def.getPermissionChecker());
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

            if (pSHGetKnownFolderPath(folderId, KF_FLAG_DEFAULT_PATH, NULL, &localFolder) == S_OK) {
                // copy into local memory
                char path[MAX_PATH];
                size_t len;
                if (wcstombs_s(&len, path, localFolder, MAX_PATH) != S_OK) {
                    path[0] = '\0';
                    SG_LOG(SG_GENERAL, SG_WARN, "WCS to MBS failed");
                }

                SGPath folder_path = SGPath(path, def.getPermissionChecker());

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

  std::ifstream user_dirs_file( user_dirs.c_str() );
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
    _cacheEnabled(true)
{
}


// create a path based on "path"
SGPath::SGPath( const std::string& p, PermissionChecker validator )
    : path(p),
    _permission_checker(validator),
    _cached(false),
    _rwCached(false),
    _cacheEnabled(true)
{
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
    _cacheEnabled(p._cacheEnabled)
{
    append(r);
    fix();
}

SGPath::SGPath(const SGPath& p) :
  path(p.path),
  _permission_checker(p._permission_checker),
  _cached(p._cached),
  _rwCached(p._rwCached),
  _cacheEnabled(p._cacheEnabled),
  _canRead(p._canRead),
  _canWrite(p._canWrite),
  _exists(p._exists),
  _isDir(p._isDir),
  _isFile(p._isFile),
  _modTime(p._modTime)
{
}

SGPath& SGPath::operator=(const SGPath& p)
{
  path = p.path;
  _permission_checker = p._permission_checker,
  _cached = p._cached;
  _rwCached = p._rwCached;
  _cacheEnabled = p._cacheEnabled;
  _canRead = p._canRead;
  _canWrite = p._canWrite;
  _exists = p._exists;
  _isDir = p._isDir;
  _isFile = p._isFile;
  _modTime = p._modTime;
  return *this;
}

// destructor
SGPath::~SGPath() {
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

//add a new path component to the existing path string
void SGPath::add( const string& p ) {
    append( sgSearchPathSep+p );
}


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
    return boost::to_lower_copy(extension());
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
        return boost::to_lower_copy(path.substr(firstDot + 1));
    } else {
        return "";
    }
}

//------------------------------------------------------------------------------
void SGPath::validate() const
{
  if (_cached && _cacheEnabled) {
    return;
  }

  if (path.empty()) {
	  _exists = false;
	  return;
  }

#ifdef _WIN32
  struct _stat buf ;
  bool remove_trailing = false;
  string statPath(path);
  if ((path.length() > 1) && (path.back() == '/')) {
	  statPath.pop_back();
  }
      
  if (_stat(statPath.c_str(), &buf ) < 0) {
    _exists = false;
  } else {
    _exists = true;
    _isFile = ((S_IFREG & buf.st_mode ) !=0);
    _isDir = ((S_IFDIR & buf.st_mode ) !=0);
    _modTime = buf.st_mtime;
  }

#else
  struct stat buf ;

  if (stat(path.c_str(), &buf ) < 0) {
    _exists = false;
  } else {
    _exists = true;
    _isFile = ((S_ISREG(buf.st_mode )) != 0);
    _isDir = ((S_ISDIR(buf.st_mode )) != 0);
    _modTime = buf.st_mtime;
  }
  
#endif
  _cached = true;
}

//------------------------------------------------------------------------------
void SGPath::checkAccess() const
{
  if( _rwCached && _cacheEnabled )
    return;

  if( _permission_checker )
  {
    Permissions p = _permission_checker(*this);
    _canRead = p.read;
    _canWrite = p.write;
  }
  else
  {
    _canRead = true;
    _canWrite = true;
  }

  _rwCached = true;
}

bool SGPath::exists() const
{
  validate();
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
  validate();
  return _exists && _isDir;
}

bool SGPath::isFile() const
{
  validate();
  return _exists && _isFile;
}

//------------------------------------------------------------------------------
#ifdef _WIN32
#  define sgMkDir(d,m)       _mkdir(d)
#else
#  define sgMkDir(d,m)       mkdir(d,m)
#endif

int SGPath::create_dir(mode_t mode)
{
  if( !canWrite() )
  {
    SG_LOG( SG_IO,
            SG_WARN, "Error creating directory for '" << str() << "'"
                                                    " reason: access denied" );
    return -3;
  }

    string_list dirlist = sgPathSplit(dir());
    if ( dirlist.empty() )
        return -1;
    string path = dirlist[0];
    string_list path_elements = sgPathBranchSplit(path);
    bool absolute = !path.empty() && path[0] == sgDirPathSep;

    unsigned int i = 1;
    SGPath dir(absolute ? string( 1, sgDirPathSep ) : "", _permission_checker);
    dir.concat( path_elements[0] );
#ifdef _WIN32
    if ( dir.str().find(':') != string::npos && path_elements.size() >= 2 ) {
        dir.append( path_elements[1] );
        i = 2;
    }
#endif
  struct stat info;
  int r;
  for(; (r = stat(dir.c_str(), &info)) == 0 && i < path_elements.size(); ++i)
    dir.append(path_elements[i]);
  if( r == 0 )
      return 0; // Directory already exists

  for(;;)
  {
    if( sgMkDir(dir.c_str(), mode) )
    {
      SG_LOG( SG_IO,
              SG_ALERT, "Error creating directory: (" << dir.str() << ")" );
      return -2;
    }
    else
      SG_LOG(SG_IO, SG_DEBUG, "Directory created: " << dir.str());

    if( i >= path_elements.size() )
      return 0;

    dir.append(path_elements[i++]);
  }

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
        int index = tmp.find(sgSearchPathSep);
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

std::string SGPath::str_native() const
{
#ifdef _WIN32
    std::string s = str();
    std::string::size_type pos;
    std::string nativeSeparator;
    nativeSeparator = sgDirPathSepBad;

    while( (pos=s.find( sgDirPathSep )) != std::string::npos ) {
        s.replace( pos, 1, nativeSeparator );
    }
    return s;
#else
    return str();
#endif
}

//------------------------------------------------------------------------------
bool SGPath::remove()
{
  if( !canWrite() )
  {
    SG_LOG( SG_IO, SG_WARN, "file remove failed: (" << str() << ")"
                                               " reason: access denied" );
    return false;
  }

  int err = ::unlink(c_str());
  if( err )
  {
    SG_LOG( SG_IO, SG_WARN, "file remove failed: (" << str() << ") "
                                               " reason: " << strerror(errno) );
    // TODO check if failed unlink can really change any of the cached values
  }

  _cached = false; // stat again if required
  _rwCached = false;
  return (err == 0);
}

time_t SGPath::modTime() const
{
    validate();
    return _modTime;
}

bool SGPath::operator==(const SGPath& other) const
{
    return (path == other.path);
}

bool SGPath::operator!=(const SGPath& other) const
{
    return (path != other.path);
}

//------------------------------------------------------------------------------
bool SGPath::rename(const SGPath& newName)
{
  if( !canRead() || !canWrite() || !newName.canWrite() )
  {
    SG_LOG( SG_IO, SG_WARN, "rename failed: from " << str() <<
                                            " to " << newName.str() <<
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
  if( ::rename(c_str(), newName.c_str()) != 0 )
  {
    SG_LOG( SG_IO, SG_WARN, "rename failed: from " << str() <<
                                            " to " << newName.str() <<
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
  const char* val = getenv(name);
  if( val && val[0] )
    return SGPath(val, def._permission_checker);
  return def;
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
std::string SGPath::realpath() const
{
#if defined(_MSC_VER) /*for MS compilers */ || defined(_WIN32) /*needed for non MS windows compilers like MingW*/
    // with absPath NULL, will allocate, and ignore length
    char *buf = _fullpath( NULL, path.c_str(), _MAX_PATH );
#else
    // POSIX
    char* buf = ::realpath(path.c_str(), NULL);
#endif
    if (!buf) // File does not exist: return the realpath it would have if created now
              // (needed for fgValidatePath security)
    {
        if (path.empty()) {
            return SGPath(".").realpath(); // current directory
        }
        std::string this_dir = dir();
        if (isAbsolute() && this_dir.empty()) { // top level
            this_dir = "/";
        }
        if (file() == "..") {
            this_dir = SGPath(SGPath(this_dir).realpath()).dir();
            if (this_dir.empty()) { // invalid path: .. above root
                return "";
            }
            return SGPath(this_dir).realpath(); // use native path separator,
                        // and handle 'existing/nonexisting/../symlink' paths
        }
        return SGPath(this_dir).realpath() +
#if defined(_MSC_VER) || defined(_WIN32)
          "\\" + file();
#else
          "/" + file();
#endif
    }
    std::string p(buf);
    free(buf);
    return p;
}
