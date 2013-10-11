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

#ifndef CANVAS_EVENT_LISTENER_HXX_
#define CANVAS_EVENT_LISTENER_HXX_

#include "canvas_fwd.hxx"
#include <simgear/nasal/naref.h>

namespace simgear
{
namespace canvas
{

  class NasalEventListener:
    public SGReferenced
  {
    public:
      NasalEventListener( naRef code,
                          const SystemAdapterPtr& sys_adapter );
      ~NasalEventListener();

      void operator()(const canvas::EventPtr& event) const;

    protected:
      naRef _code;
      int _gc_key;
      SystemAdapterWeakPtr _sys;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_EVENT_LISTENER_HXX_ */
