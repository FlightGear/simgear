/**
 * \file screen-dump.hxx
 * Dump a copy of the opengl screen buffer to a file.
 */

// Contributed by Richard Kaszeta <bofh@me.umn.edu>, started October 1999.
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifndef SG_SCREEN_DUMP_HXX
#define SG_SCREEN_DUMP_HXX

/**
 * Dump the screen buffer to a PNG file.
 * @param filename name of file
 * @param win_width width of our opengl window
 * @param win_height height of our opengl window
 */
bool sg_glDumpWindow( const char *filename, int win_width, int win_height );


#endif // of SG_SCREEN_DUMP_HXX
