// screen-dump.cxx -- dump a copy of the opengl screen buffer to a file
//
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


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <GL/gl.h>

#include "screen-dump.hxx"


#define RGB3 3			// 3 bytes of color info per pixel
#define RGBA 4			// 4 bytes of color+alpha info

bool sg_glWritePPMFile(const char *filename, GLubyte *buffer, int win_width, int win_height, int mode)
{
    int i, j, k, q;
    unsigned char *ibuffer;
    FILE *fp;
    int pixelSize = mode==GL_RGBA?4:3;

    ibuffer = (unsigned char *) malloc(win_width*win_height*RGB3);

    if ( (fp = fopen(filename, "wb")) == NULL ) {
	printf("Warning: cannot open %s\n", filename);
	return false;
    }

    fprintf(fp, "P6\n# CREATOR: glReadPixel()\n%d %d\n%d\n",
	    win_width, win_height, UCHAR_MAX);
    q = 0;
    for (i = 0; i < win_height; i++)
	for (j = 0; j < win_width; j++)
	    for (k = 0; k < RGB3; k++)
		ibuffer[q++] = (unsigned char)
		    *(buffer + (pixelSize*((win_height-1-i)*win_width+j)+k));
    fwrite(ibuffer, sizeof(unsigned char), RGB3*win_width*win_height, fp);
    fclose(fp);
    free(ibuffer);

    printf("wrote file (%d x %d pixels, %d bytes)\n",
	   win_width, win_height, RGB3*win_width*win_height);
    return true;
}


// dump the screen buffer to a ppm file
bool sg_glDumpWindow(const char *filename, int win_width, int win_height) {
    GLubyte *buffer;
    bool result;

    buffer = (GLubyte *) malloc(win_width*win_height*RGBA);

    // read window contents from color buffer with glReadPixels
    glFinish();
    glReadPixels(0, 0, win_width, win_height, 
		 GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    result = sg_glWritePPMFile( filename, buffer, win_width, win_height,
				GL_RGBA );
    free(buffer);

    return result;
}

