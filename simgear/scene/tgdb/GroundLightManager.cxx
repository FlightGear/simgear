/*
 * SPDX-FileCopyrightText: Copyright (C) 2007  Tim Moore timoore@redhat.com
 * SPDX-FileCopyrightText: Copyright (C) 2006-2007 Mathias Froehlich
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "GroundLightManager.hxx"

#include <simgear/scene/util/RenderConstants.hxx>

namespace simgear {

unsigned GroundLightManager::getLightNodeMask(const SGUpdateVisitor* updateVisitor)
{
    unsigned mask = 0;
    // The current sun angle in degree
    float sun_angle = updateVisitor->getSunAngleDeg();
    if (sun_angle > 85 || updateVisitor->getVisibility() < 5000)
        mask |= RUNWAYLIGHTS_BIT;
    // ground lights
    if ( sun_angle > 95 )
        mask |= GROUNDLIGHTS2_BIT;
    if ( sun_angle > 92 )
        mask |= GROUNDLIGHTS1_BIT;
    if ( sun_angle > 89 )
        mask |= GROUNDLIGHTS0_BIT;
    return mask;
}

} // namespace simgear
