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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


/**
 * Dump the screen buffer to a ppm file.
 * @param filename name of file
 * @param win_width width of our opengl window
 * @param win_height height of our opengl window
 */
void my_glDumpWindow( const char *filename, int win_width, int win_height );


/**
 * Given a GLubyte *buffer, write it out to a ppm file.
 * @param filename name of file
 * @param buffer pointer to opengl buffer
 * @param win_width width of buffer
 * @param win_height height of buffer
 * @param mode one of GL_RGBA, GL_RGB, etc.
 */
void my_glWritePPMFile( const char *filename, GLubyte *buffer, int win_width, 
		        int win_height, int mode);
