///@file
/// Enumeration of layout alignment flags.
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
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
