// screen-dump.cxx -- dump a copy of the opengl screen buffer to a file
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <GL/glut.h>
#include <simgear/xgl.h>

#include "screen-dump.hxx"


#define RGB  3			// 3 bytes of color info per pixel
#define RGBA 4			// 4 bytes of color+alpha info

void my_glWritePPMFile(const char *filename, GLubyte *buffer, int win_width, int win_height, int mode)
{
    int i, j, k, q;
    unsigned char *ibuffer;
    FILE *fp;
    int pixelSize = mode==GL_RGBA?4:3;

    ibuffer = (unsigned char *) malloc(win_width*win_height*RGB);

    fp = fopen(filename, "wb");
    fprintf(fp, "P6\n# CREATOR: glReadPixel()\n%d %d\n%d\n",
	    win_width, win_height, UCHAR_MAX);
    q = 0;
    for (i = 0; i < win_height; i++)
	for (j = 0; j < win_width; j++)
	    for (k = 0; k < RGB; k++)
		ibuffer[q++] = (unsigned char)
		    *(buffer + (pixelSize*((win_height-1-i)*win_width+j)+k));
    fwrite(ibuffer, sizeof(unsigned char), RGB*win_width*win_height, fp);
    fclose(fp);
    free(ibuffer);

    printf("wrote file (%d x %d pixels, %d bytes)\n",
	   win_width, win_height, RGB*win_width*win_height);
}


// dump the screen buffer to a ppm file
void my_glDumpWindow(const char *filename, int win_width, int win_height) {
    GLubyte *buffer;

    buffer = (GLubyte *) malloc(win_width*win_height*RGBA);

    // read window contents from color buffer with glReadPixels
    glFinish();
    glReadPixels(0, 0, win_width, win_height, 
		 GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	my_glWritePPMFile( filename, buffer, win_width, win_height, GL_RGBA );
    free(buffer);
}

