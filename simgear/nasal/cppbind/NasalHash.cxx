// Wrapper class for Nasal hashes
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include "NasalHash.hxx"
#include "to_nasal.hxx"

#include <cassert>

namespace nasal
{

  //----------------------------------------------------------------------------
  Hash::Hash(naContext c):
    _hash(naNewHash(c)),
    _context(c),
    _keys(naNil())
  {

  }

  //----------------------------------------------------------------------------
  Hash::Hash(naRef hash, naContext c):
    _hash(hash),
    _context(c),
    _keys(naNil())
  {
    assert( naIsHash(_hash) );
  }

  //----------------------------------------------------------------------------
  Hash::iterator Hash::begin()
  {
    return iterator(this, 0);
  }

  //----------------------------------------------------------------------------
  Hash::iterator Hash::end()
  {
    return iterator(this, size());
  }

  //----------------------------------------------------------------------------
  Hash::const_iterator Hash::begin() const
  {
    return const_iterator(this, 0);
  }

  //----------------------------------------------------------------------------
  Hash::const_iterator Hash::end() const
  {
    return const_iterator(this, size());
  }

  //----------------------------------------------------------------------------
  void Hash::set(const std::string& name, naRef ref)
  {
    naHash_set(_hash, to_nasal(_context, name), ref);
    _keys = naNil();
  }

  //----------------------------------------------------------------------------
  naRef Hash::get(naRef key) const
  {
    naRef result;
    return naHash_get(_hash, key, &result) ? result : naNil();
  }

  //----------------------------------------------------------------------------
  naRef Hash::get(const std::string& name) const
  {
    return get( to_nasal(_context, name) );
  }

  //----------------------------------------------------------------------------
  int Hash::size() const
  {
    return naVec_size(get_naRefKeys());
  }

  //----------------------------------------------------------------------------
  std::vector<std::string> Hash::keys() const
  {
    return from_nasal<std::vector<std::string> >(_context, get_naRefKeys());
  }

  //----------------------------------------------------------------------------
  Hash Hash::createHash(const std::string& name)
  {
    Hash hash(_context);
    set(name, hash);
    return hash;
  }

  //----------------------------------------------------------------------------
  void Hash::setContext(naContext context)
  {
    _context = context;
  }

  //----------------------------------------------------------------------------
  naRef Hash::get_naRef() const
  {
    return _hash;
  }

  //----------------------------------------------------------------------------
  naRef Hash::get_naRefKeys() const
  {
    if( naIsNil(_keys) && naIsHash(_hash) )
    {
      _keys = naNewVector(_context);
      naHash_keys(_keys, _hash);
    }

    return _keys;
  }

  //----------------------------------------------------------------------------
  bool Hash::isNil() const
  {
      return naIsNil(_hash);
  }

} // namespace nasal
