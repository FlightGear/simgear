// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Mapping between strings and osg text alignment flags
 */

#ifndef ENUM_MAPPING
# error "Only include with ENUM_MAPPING defined!"
#endif

ENUM_MAPPING(LEFT_TOP,      "left-top")
ENUM_MAPPING(LEFT_CENTER,   "left-center")
ENUM_MAPPING(LEFT_BOTTOM,   "left-bottom")

ENUM_MAPPING(CENTER_TOP,    "center-top")
ENUM_MAPPING(CENTER_CENTER, "center-center")
ENUM_MAPPING(CENTER_BOTTOM, "center-bottom")

ENUM_MAPPING(RIGHT_TOP,     "right-top")
ENUM_MAPPING(RIGHT_CENTER,  "right-center")
ENUM_MAPPING(RIGHT_BOTTOM,  "right-bottom")

ENUM_MAPPING(LEFT_BASE_LINE,    "left-baseline")
ENUM_MAPPING(CENTER_BASE_LINE,  "center-baseline")
ENUM_MAPPING(RIGHT_BASE_LINE,   "right-baseline")

ENUM_MAPPING(LEFT_BOTTOM_BASE_LINE,     "left-bottom-baseline")
ENUM_MAPPING(CENTER_BOTTOM_BASE_LINE,   "center-bottom-baseline")
ENUM_MAPPING(RIGHT_BOTTOM_BASE_LINE,    "right-bottom-baseline")
