/*
 * SPDX-FileCopyrightText: Copyright (C) 2007  Tim Moore timoore@redhat.com
 * SPDX-FileCopyrightText: Copyright (C) 2006-2007 Mathias Froehlich
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <simgear/scene/util/OsgSingleton.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>

namespace simgear {

class GroundLightManager : public ReferencedSingleton<GroundLightManager> {
public:
    unsigned getLightNodeMask(const SGUpdateVisitor* updateVisitor);
};

} // namespace simgear
