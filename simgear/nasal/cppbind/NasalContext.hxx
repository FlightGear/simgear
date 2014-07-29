///@file
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SG_NASAL_CONTEXT_HXX_
#define SG_NASAL_CONTEXT_HXX_

#include "NasalHash.hxx"

namespace nasal
{

  /**
   * Manage lifetime and encapsulate a Nasal context.
   */
  class Context
  {
    public:
      Context();
      ~Context();

      operator naContext();

      Hash newHash();

    protected:
      naContext _ctx;
  };

} // namespace nasal

#endif /* SG_NASAL_CONTEXT_HXX_ */
