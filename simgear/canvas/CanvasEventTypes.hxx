// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Mapping between canvas gui Event types and their names
 */

#ifndef ENUM_MAPPING
# error "Don't include this file directly!"
#endif

ENUM_MAPPING(MOUSE_DOWN,  "mousedown",  MouseEvent)
ENUM_MAPPING(MOUSE_UP,    "mouseup",    MouseEvent)
ENUM_MAPPING(CLICK,       "click",      MouseEvent)
ENUM_MAPPING(DBL_CLICK,   "dblclick",   MouseEvent)
ENUM_MAPPING(DRAG,        "drag",       MouseEvent)
ENUM_MAPPING(DRAG_START,  "dragstart",  MouseEvent)
ENUM_MAPPING(DRAG_END,    "dragend",    MouseEvent)
ENUM_MAPPING(WHEEL,       "wheel",      MouseEvent)
ENUM_MAPPING(MOUSE_MOVE,  "mousemove",  MouseEvent)
ENUM_MAPPING(MOUSE_OVER,  "mouseover",  MouseEvent)
ENUM_MAPPING(MOUSE_OUT,   "mouseout",   MouseEvent)
ENUM_MAPPING(MOUSE_ENTER, "mouseenter", MouseEvent)
ENUM_MAPPING(MOUSE_LEAVE, "mouseleave", MouseEvent)
ENUM_MAPPING(KEY_DOWN,    "keydown",    KeyboardEvent)
ENUM_MAPPING(KEY_UP,      "keyup",      KeyboardEvent)
ENUM_MAPPING(KEY_PRESS,   "keypress",   KeyboardEvent)
