/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Tim Moore
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

#ifndef SIMGEAR_RENDER_CONSTANTS_HXX
#define SIMGEAR_RENDER_CONSTANTS_HXX
// Constants used in the scene graph, both node masks and render bins.
namespace simgear {

enum NodeMask {
    TERRAIN_BIT = (1 << 0),
    MAINMODEL_BIT = (1 << 1),
    CASTSHADOW_BIT = (1 << 2),
    RECEIVESHADOW_BIT = (1 << 3),
    GUI_BIT = (1 << 4),
    PANEL2D_BIT = (1 << 5),
    PICK_BIT = (1 << 6),
    // Different classes of lights turned on by node masks
    GROUNDLIGHTS0_BIT = (1 << 7),
    GROUNDLIGHTS1_BIT = (1 << 8),
    GROUNDLIGHTS2_BIT = (1 << 9),
    RUNWAYLIGHTS_BIT = (1 << 10),
    LIGHTS_BITS = (GROUNDLIGHTS0_BIT | GROUNDLIGHTS1_BIT | GROUNDLIGHTS2_BIT
                   | RUNWAYLIGHTS_BIT),
    // Sky parts
    BACKGROUND_BIT = (1 << 11),
    // Everything else that isn't terrain. Initially for clouds;
    // eventually for other models?
    MODEL_BIT = (1 << 12),
    MODELLIGHT_BIT = (1 << 13),
    PERMANENTLIGHT_BIT = (1 << 14)
};

// Theory of bin numbering:
//
// Normal opaque objects are assigned bin 0.
//
// Random objects like trees may have transparency, but there are too
// many to depth sort individually. By drawing them after the terrain
// we can at least keep the sky under the ground from poking through.
//
// Point lights blend with the terrain to simulate attenuation but
// should completely obscure any transparent geometry behind
// them. Also, they should be visible through semi-transparent cloud
// layers, so they are rendered before the cloud layers.
//
// Clouds layers can't be depth sorted because they are too big, so
// they are rendered before other transparent objects. The layer
// partial ordering is handled in the clouds code.
//
// OSG and its file loaders throw all transparent objects into bin 10.

enum RenderBin {
    RANDOM_OBJECTS_BIN = 2,
    POINT_LIGHTS_BIN = 8,
    CLOUDS_BIN = 9,
    TRANSPARENT_BIN = 10        // assigned by OSG
};
}
#endif
