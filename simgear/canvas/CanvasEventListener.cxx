// Listener for canvas (GUI) events being passed to a Nasal function/code
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

#include "CanvasEventListener.hxx"
#include "CanvasSystemAdapter.hxx"

#include <simgear/nasal/nasal.h>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  EventListener::EventListener(naRef code, const SystemAdapterPtr& sys_adapter):
    _code(code),
    _gc_key(-1),
    _sys(sys_adapter)
  {
    assert( sys_adapter );
    if(    !naIsCode(code)
        && !naIsCCode(code)
        && !naIsFunc(code) )
      throw std::runtime_error
      (
        "canvas::EventListener: invalid function argument"
      );

    _gc_key = sys_adapter->gcSave(_code);
  }

  //----------------------------------------------------------------------------
  EventListener::~EventListener()
  {
    assert( !_sys.expired() );
    _sys.lock()->gcRelease(_gc_key);
  }

  //------------------------------------------------------------------------------
  void EventListener::call()
  {
    const size_t num_args = 1;
    naRef args[num_args] = {
      naNil()
    };
    _sys.lock()->callMethod(_code, naNil(), num_args, args, naNil());
  }


} // namespace canvas
} // namespace simgear
