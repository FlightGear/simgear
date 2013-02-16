// GroundLightManager.hxx - manage StateSets for point lights
//
// Copyright (C) 2007  Tim Moore timoore@redhat.com
// Copyright (C) 2006-2007 Mathias Froehlich 
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <osg/ref_ptr>
#include <osg/Vec4>
#include <osg/Referenced>
#include <osg/StateSet>


#include <simgear/scene/util/OsgSingleton.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>

namespace simgear
{
class GroundLightManager : public ReferencedSingleton<GroundLightManager> {
public:
    GroundLightManager();
    osg::StateSet* getRunwayLightStateSet() { return runwayLightSS.get(); }
    osg::StateSet* getTaxiLightStateSet() { return taxiLightSS.get(); }
    osg::StateSet* getGroundLightStateSet() { return groundLightSS.get(); }
    // The SGUpdateVisitor manages various environmental properties,
    // so use it.
    void update (const SGUpdateVisitor* updateVisitor);
    unsigned getLightNodeMask(const SGUpdateVisitor* updateVisitor);
protected:
    osg::ref_ptr<osg::StateSet> runwayLightSS;
    osg::ref_ptr<osg::StateSet> taxiLightSS;
    osg::ref_ptr<osg::StateSet> groundLightSS;
};
}
