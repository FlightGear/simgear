// screen-dump.hxx -- dump a copy of the opengl screen buffer to a file
//
// Contributed by Richard Kaszeta <bofh@me.umn.edu>, started October 1999.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


// dump the screen buffer to a ppm file
void my_glDumpWindow( const char *filename, int win_width, int win_height );

void my_glWritePPMFile( const char *filename, GLubyte *buffer, int win_width, 
		        int win_height, int mode);
