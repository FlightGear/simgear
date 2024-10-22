// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1999 Richard Kaszeta <bofh@me.umn.edu>

/**
 * @file
 * @brief Dump a copy of the OpenGL screen buffer to a file
 */

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
