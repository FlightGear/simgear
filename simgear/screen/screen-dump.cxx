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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#if defined(__CYGWIN__)  /* && !defined(USING_X) */
#define WIN32
#endif

#if defined(WIN32)  /* MINGW and MSC predefine WIN32 */
# include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <simgear/compiler.h>

#include <osg/Image>
#include <osgDB/WriteFile>

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
	free(ibuffer);
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

    printf("wrote file '%s' (%d x %d pixels, %d bytes)\n",
	   filename, win_width, win_height, RGB3*win_width*win_height);
    return true;
}


// dump the screen buffer to a png file
bool sg_glDumpWindow(const char *filename, int win_width, int win_height) {
  osg::ref_ptr<osg::Image> img(new osg::Image);
  img->readPixels(0,0, win_width, win_height, GL_RGB, GL_UNSIGNED_BYTE);
  osgDB::writeImageFile(*img, filename);
  return true;
}

