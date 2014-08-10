// Mapping between canvas gui Event types and their names
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

#ifndef ENUM_MAPPING
# error "Don't include this file directly!"
#endif

ENUM_MAPPING(MOUSE_DOWN,  "mousedown",  MouseEvent)
ENUM_MAPPING(MOUSE_UP,    "mouseup",    MouseEvent)
ENUM_MAPPING(CLICK,       "click",      MouseEvent)
ENUM_MAPPING(DBL_CLICK,   "dblclick",   MouseEvent)
ENUM_MAPPING(DRAG,        "drag",       MouseEvent)
ENUM_MAPPING(WHEEL,       "wheel",      MouseEvent)
ENUM_MAPPING(MOUSE_MOVE,  "mousemove",  MouseEvent)
ENUM_MAPPING(MOUSE_OVER,  "mouseover",  MouseEvent)
ENUM_MAPPING(MOUSE_OUT,   "mouseout",   MouseEvent)
ENUM_MAPPING(MOUSE_ENTER, "mouseenter", MouseEvent)
ENUM_MAPPING(MOUSE_LEAVE, "mouseleave", MouseEvent)
ENUM_MAPPING(KEY_DOWN,    "keydown",    KeyboardEvent)
ENUM_MAPPING(KEY_UP,      "keyup",      KeyboardEvent)
ENUM_MAPPING(KEY_PRESS,   "keypress",   KeyboardEvent)
