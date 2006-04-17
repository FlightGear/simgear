// jpgfactory.hxx --  jpeg frontend for TR library
//
// Written by Norman Vine, started August 2001.
//
// Copyright (C) 2001  Norman Vine - nhv@yahoo.com
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifndef _FG_JPGFACTORY_HXX
#define _FG_JPGFACTORY_HXX

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <jpeglib.h>
#include <jerror.h>

#ifdef __cplusplus
}
#endif

#include <simgear/screen/tr.h>


extern void (*jpgRenderFrame)(void);

/* should look at how VNC does this */
class trJpgFactory {
    private:
        int imageWidth;
        int imageHeight;
        GLubyte *tile;
        GLubyte *buffer;

        TRcontext *tr;
        unsigned char *IMAGE;
        int IMAGESIZE;

        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        int jpeg_init();
        int compress();

        typedef enum {
            DEFAULT_XS = 320,
            DEFAULT_YS = 240
        } JPG_FACTORY_ENUM;

    public:
        trJpgFactory();
        ~trJpgFactory();

        int init(int width = 0, int height = 0 );
        void destroy(int error = 0);

        int render();
        unsigned char *data() { return IMAGE ; }

        struct jpeg_compress_struct *JPGinfo() { return &cinfo ; }
};

#endif // #ifndef _FG_JPGFACTORY_HXX
