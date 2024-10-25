/*
 * SPDX-FileName: sphere.hxx
 * SPDX-FileComment: build a sphere object. Pulled straight out of MesaGLU/quadratic.c
 * SPDX-FileContributor: Pulled straight out of MesaGLU/quadratic.c
 * SPDX-FileContributor: Original gluSphere code Copyright (C) 1999-2000  Brian Paul licensed under the GPL
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

namespace simgear {
class EffectGeode;
}

simgear::EffectGeode* SGMakeSphere(double radius, int slices, int stacks);
