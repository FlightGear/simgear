///@file
//
// Copyright (C) 2018  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SG_NASAL_ME_HXX_
#define SG_NASAL_ME_HXX_

#include <simgear/nasal/nasal.h>

namespace nasal
{

  /**
   * Wrap a naRef to indicate it references the self/me object in Nasal method
   * calls.
   */
  struct Me
  {
    naRef _ref;

    explicit Me(naRef ref = naNil()):
      _ref(ref)
    {}

    operator naRef() { return _ref; }
  };

} // namespace nasal

#endif /* SG_NASAL_ME_HXX_ */
