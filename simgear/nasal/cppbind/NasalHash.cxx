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
    _hash( naNewHash(c) ),
    _context(c)
  {

  }

  //----------------------------------------------------------------------------
  Hash::Hash(naRef hash, naContext c):
    _hash(hash),
    _context(c)
  {
    assert( naIsHash(_hash) );
  }

  //----------------------------------------------------------------------------
  void Hash::set(const std::string& name, naRef ref)
  {
    naHash_set(_hash, to_nasal(_context, name), ref);
  }

  //----------------------------------------------------------------------------
  naRef Hash::get(const std::string& name)
  {
    naRef result;
    return naHash_get(_hash, to_nasal(_context, name), &result) ? result
                                                                : naNil();
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

} // namespace nasal
