// jpgfactory.cxx -- jpeg frontend for TR library
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


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include "jpgfactory.hxx"
#include <string.h>   

#ifdef __cplusplus
extern "C" {
#endif

static void init_destination (j_compress_ptr cinfo);
static void term_destination (j_compress_ptr cinfo);
static boolean empty_output_buffer (j_compress_ptr cinfo);

#ifdef __cplusplus
}
#endif

// OSGFIME: offscreenrendering on osg - completely new context ...

typedef struct {
    struct jpeg_destination_mgr pub; /* public fields */
    unsigned char * outfile; /* target stream */
    JOCTET * buffer;         /* start of buffer */
    int numbytes;            /* num bytes used */
    int maxsize;             /* size of outfile */
    int error;               /* error flag */
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;

void (*jpgRenderFrame)(void) = NULL;

trJpgFactory::trJpgFactory() {
    imageWidth = imageHeight = 0;
    tile       = NULL;
    buffer     = NULL;
    IMAGE      = NULL;
    tr         = NULL;
    cinfo.dest = NULL;
}

trJpgFactory::~trJpgFactory() {
    destroy();
}

/*
 * deallocate our dynamic parts
 */

void trJpgFactory::destroy( int error )
{
    if( error )
        printf( "!! Malloc Failure trJpgFactory ( %d )!!\n",
                error );

    if( cinfo.dest )  jpeg_destroy_compress(&cinfo);
    if( tr )          trDelete(tr);
    if( IMAGE )       delete [] IMAGE;
    if( buffer )      delete [] buffer;
    if( tile )        delete [] tile;
}

/*
 * allocate and initialize the jpeg compress struct
 * application needs to dealocate this
 */

int trJpgFactory::jpeg_init()
{
    j_compress_ptr cinfoPtr = &cinfo;

    cinfoPtr->err = jpeg_std_error(&jerr);
    jpeg_create_compress(cinfoPtr);

    /* following taken from jpeg library exaample code */
    cinfoPtr->dest = (struct jpeg_destination_mgr *)
                     (*cinfoPtr->mem->alloc_small)
                     ((j_common_ptr)cinfoPtr,
                      JPOOL_PERMANENT,
                      sizeof(my_destination_mgr));

    my_dest_ptr dest = (my_dest_ptr)cinfoPtr->dest;

    if( !dest ) {
        destroy(5);
        return 5;
    }

    dest->pub.init_destination    = init_destination;
    dest->pub.empty_output_buffer = empty_output_buffer;
    dest->pub.term_destination    = term_destination;
    dest->outfile  = NULL;
    dest->numbytes = 0;
    dest->maxsize  = 0;

    cinfoPtr->image_width      = imageWidth;
    cinfoPtr->image_height     = imageHeight;
    cinfoPtr->input_components = 3;
    cinfoPtr->in_color_space   = JCS_RGB;
    jpeg_set_defaults(cinfoPtr);
    jpeg_set_quality(cinfoPtr, (100 * 90) >> 8, TRUE);

    return 0;
}


/*
 * may also be used as reinit() to change image size
 */

int trJpgFactory::init(int width, int height )
{
    destroy();

    if( width <= 0 || height <= 0 ) {
        imageWidth  = DEFAULT_XS;
        imageHeight = DEFAULT_YS;
    } else {
        imageWidth  = width;
        imageHeight = height;
    }

    int bufsize = imageWidth * imageHeight * 3 * sizeof(GLubyte);

    /* allocate buffer large enough to store one tile */
    tile = new GLubyte[bufsize];
    if (!tile) {
        destroy(1);
        return 1;
    }

    /* allocate buffer to hold a row of tiles */
    buffer = new GLubyte[ bufsize ];
    if (!buffer) {
        destroy(2);
        return 2;
    }

    /* this should be big enough */
    IMAGESIZE = bufsize + 1024;
    IMAGE = new unsigned char[ IMAGESIZE ];
    if( !IMAGE ) {
        destroy(3);
        return 3;
    }

    tr = trNew();
    if( !tr ) {
        destroy(4);
        return 4;
    }
    
    trRowOrder(tr, TR_TOP_TO_BOTTOM);
    trTileSize(tr, imageWidth, imageHeight, 0);
    trImageSize(tr, imageWidth, imageHeight);
    trTileBuffer(tr, GL_RGB, GL_UNSIGNED_BYTE, tile);

    return jpeg_init();
}

/*
 *compress the image
 */

int trJpgFactory::compress()
{
    JSAMPROW  row_pointer[1];
    int       row_stride;

    /* to keep track of jpeg library routines */
    my_dest_ptr dest = (my_dest_ptr) cinfo.dest;

    //printf("\tjpeg_start_compress(&cinfo, TRUE)\n");
    jpeg_start_compress(&cinfo, TRUE);
    if( !dest->error ) {
        dest->outfile = IMAGE;
        dest->maxsize = IMAGESIZE;
        row_stride    = cinfo.image_width * 3;

        while( cinfo.next_scanline < cinfo.image_height &&
               !dest->error )
        {
            row_pointer[0] = buffer + (cinfo.next_scanline * row_stride);
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
    }
    if( !dest->error ) {
        // printf("\n\tBEFORE: jpeg_finish_compress(&cinfo)\n");
        jpeg_finish_compress(&cinfo);
        // printf("\tAFTER: jpeg_finish_compress(&cinfo)\n");
    } else {
        printf("INTERNAL JPEG_FACTORY ERROR\n");
        jpeg_abort_compress(&cinfo);
        /* OK - I am paranoid */
        dest->numbytes = 0;
    }
    return dest->numbytes;
}

/*
 * Makes the image then calls compress()
 */

int trJpgFactory::render()
{
    if( !tr || !jpgRenderFrame ) {
        printf("!! NO tr !!\n   trJpgFactory::render()\n");
        return 0;
    }

    // Make sure we have SSG projection primed for current view
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* just to be safe... */
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // printf("\ttrBeginTile(tr)\n");
    trBeginTile(tr);
    jpgRenderFrame();
    trEndTile(tr);

    /* just to be safe */
    int curTileHeight = trGet(tr, TR_CURRENT_TILE_HEIGHT);
    int curTileWidth  = trGet(tr, TR_CURRENT_TILE_WIDTH);

    /* reverse image top to bottom */
    int bytesPerImageRow = imageWidth * 3*sizeof(GLubyte);
    int bytesPerTileRow  = imageWidth * 3*sizeof(GLubyte);
    int bytesPerCurrentTileRow = (curTileWidth) * 3*sizeof(GLubyte);
    int i;
    for (i=0;i<imageHeight;i++) {
        memcpy(buffer + (curTileHeight-1-i) * bytesPerImageRow, /* Dest */
               tile + i*bytesPerTileRow,              /* Src */
               bytesPerCurrentTileRow);               /* Byte count*/
    }

    //  printf("exit trJpgFactory::render()\n");
    return  compress();
}


#define OUTPUT_BUF_SIZE  4096

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

static void init_destination (j_compress_ptr cinfo)
{
    // printf("enter init_destination()\n");
    my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

    /* following taken from jpeg library exaample code
     * Allocate the output buffer ---
     * it automaically will be released when done with image */
	
    dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small)
                   ((j_common_ptr)cinfo, JPOOL_IMAGE,
                    OUTPUT_BUF_SIZE * sizeof(JOCTET) );

    if( !dest->buffer ) {
        printf("MALLOC FAILED jpegFactory init_destination()\n");
        dest->error = TRUE;
    } else {
        dest->error = FALSE;
    }
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer   = OUTPUT_BUF_SIZE;
    dest->numbytes = 0;
}

/*
 * Empty the output buffer --- called whenever buffer fills up.
 */

static boolean empty_output_buffer (j_compress_ptr cinfo)
{
    // printf("enter empty_output_buffer(%d)\n", OUTPUT_BUF_SIZE);
    my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

    if( (!dest->error) &&
          ((dest->numbytes + OUTPUT_BUF_SIZE) < dest->maxsize) )
    {
        memcpy( dest->outfile+dest->numbytes, dest->buffer, (size_t)OUTPUT_BUF_SIZE);

        dest->pub.next_output_byte = dest->buffer;
        dest->pub.free_in_buffer   = OUTPUT_BUF_SIZE;

        dest->numbytes += OUTPUT_BUF_SIZE;
    } else {
        printf("BUFFER OVERFLOW jpegFactory empty_output_buffer()\n");
        dest->numbytes = 0;
        dest->error    = TRUE;
    }
    return TRUE;
}

/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

static void term_destination (j_compress_ptr cinfo)
{
    my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

    size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

    if( (!dest->error) &&
          ((dest->numbytes + datacount) < (unsigned int)dest->maxsize) )
    {
        memcpy( dest->outfile+dest->numbytes, dest->buffer, datacount);
        dest->numbytes += datacount;
    } else {
        printf("BUFFER OVERFLOW jpegFactory term_destination()\n");
        dest->numbytes = 0;
        dest->error = TRUE;
    }
    // printf(" term_destination(%d) total %d\n", datacount, dest->numbytes );
}

#ifdef __cplusplus
}
#endif

