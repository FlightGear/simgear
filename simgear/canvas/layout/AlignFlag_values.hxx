// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Enumeration of layout alignment flags.
 */

#ifndef ALIGN_ENUM_MAPPING
# error "Only include with ALIGN_ENUM_MAPPING defined!"
#endif

ALIGN_ENUM_MAPPING(AlignFill, 0, Use all available space)

ALIGN_ENUM_MAPPING(AlignLeft, 0x01, Align with left edge)
ALIGN_ENUM_MAPPING(AlignRight, 0x02, Align with right edge)
ALIGN_ENUM_MAPPING(AlignHCenter, 0x04, Center horizontally in available space)

ALIGN_ENUM_MAPPING(AlignTop, 0x20, Align with top edge)
ALIGN_ENUM_MAPPING(AlignBottom, 0x40, Align with bottom edge)
ALIGN_ENUM_MAPPING(AlignVCenter, 0x80, Center vertically in available space)

ALIGN_ENUM_MAPPING( AlignCenter,
                    AlignVCenter | AlignHCenter,
                    Center both vertically and horizontally )

ALIGN_ENUM_MAPPING( AlignHorizontal_Mask,
                    AlignLeft | AlignRight | AlignHCenter,
                    Mask of all horizontal alignment flags )
ALIGN_ENUM_MAPPING( AlignVertical_Mask,
                    AlignTop | AlignBottom | AlignVCenter,
                    Mask of all vertical alignment flags )
