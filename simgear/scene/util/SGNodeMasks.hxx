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

#ifndef SG_SCENE_NODEMASKS_HXX
#define SG_SCENE_NODEMASKS_HXX

#include "RenderConstants.hxx"

/// If set, do terrain elevation computations with that nodes
#define SG_NODEMASK_TERRAIN_BIT        simgear::TERRAIN_BIT
/// If set, this is the main model of this simulation
#define SG_NODEMASK_MAINMODEL_BIT      simgear::MAINMODEL_BIT
/// If set, cast shadows
#define SG_NODEMASK_CASTSHADOW_BIT     simgear::CASTSHADOW_BIT
/// If set, cast recieves shadows
#define SG_NODEMASK_RECEIVESHADOW_BIT  simgear::RECEIVESHADOW_BIT
#define SG_NODEMASK_GUI_BIT            simgear::GUI_BIT
#define SG_NODEMASK_2DPANEL_BIT        simgear::PANEL2D_BIT
/// If set, the node is pickable
#define SG_NODEMASK_PICK_BIT           simgear::PICK_BIT
#endif
