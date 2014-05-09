// Adapter for using the canvas with different applications
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

#ifndef SG_CANVAS_SYSTEM_ADAPTER_HXX_
#define SG_CANVAS_SYSTEM_ADAPTER_HXX_

#include "canvas_fwd.hxx"

class SGSubsystem;

namespace simgear
{
namespace HTTP { class Client; }
namespace canvas
{

  class SystemAdapter
  {
    public:

      virtual ~SystemAdapter() {}
      virtual FontPtr getFont(const std::string& name) const = 0;
      virtual void addCamera(osg::Camera* camera) const = 0;
      virtual void removeCamera(osg::Camera* camera) const = 0;
      virtual osg::Image* getImage(const std::string& path) const = 0;
      virtual SGSubsystem* getSubsystem(const std::string& name) const = 0;
      virtual HTTP::Client* getHTTPClient() const = 0;
  };

} // namespace canvas
} // namespace simgear

#endif /* SG_CANVAS_SYSTEM_ADAPTER_HXX_ */
