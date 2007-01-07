/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef SG_SCENE_PICKCALLBACK_HXX
#define SG_SCENE_PICKCALLBACK_HXX

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGMath.hxx>

// Used to implement scenery interaction.
// The interface is still under development
class SGPickCallback : public SGReferenced {
public:
  struct Info {
    SGVec3d wgs84;
    SGVec3d local;
  };

  virtual ~SGPickCallback() {}
  virtual bool buttonPressed(int button, const Info& info)
  { return false; }
  virtual void update(double dt)
  { }
  virtual void buttonReleased(void)
  { }
};

struct SGSceneryPick {
  SGPickCallback::Info info;
  SGSharedPtr<SGPickCallback> callback;
};

#endif
