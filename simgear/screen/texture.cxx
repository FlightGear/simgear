/*
 * Texture manipulation routines
 *
 * Copyright (c) Mark J. Kilgard, 1997.
 * Code added in april 2003 by Erik Hofman
 *
 * This program is freely distributable without licensing fees 
 * and is provided without guarantee or warrantee expressed or 
 * implied. This program is -not- in the public domain.
 *
 * $Id$
 */

#include <simgear/compiler.h>

#ifdef WIN32
# include <windows.h>
#endif

#include SG_GLU_H

#include <zlib.h>

#include "texture.hxx"
#include "colours.h"


const char *FILE_OPEN_ERROR = "Unable to open file.";
const char *WRONG_COUNT = "Unsupported number of color channels.";
const char *NO_TEXTURE = "No texture data resident.";
const char *OUT_OF_MEMORY = "Out of memory.";


SGTexture::SGTexture()
   : texture_id(0),
     texture_data(0),
     num_colors(3)
{
}

SGTexture::SGTexture(unsigned int width, unsigned int height)
   : texture_id(0),
     errstr("")
{
    texture_data = new GLubyte[ width * height * 3 ];
}

SGTexture::~SGTexture()
{
    if ( texture_data ) {
        delete texture_data;
    }

    if ( texture_id != 0 ) {
        free_id();
    }
}

void
SGTexture::bind()
{
    bool gen = false;
    if (!texture_id) {
#ifdef GL_VERSION_1_1
        glGenTextures(1, &texture_id);

#elif GL_EXT_texture_object
        glGenTexturesEXT(1, &texture_id);
#endif
        gen = true;
    }

#ifdef GL_VERSION_1_1
    glBindTexture(GL_TEXTURE_2D, texture_id);

#elif GL_EXT_texture_object
    glBindTextureEXT(GL_TEXTURE_2D, texture_id);
#endif

    if (gen) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

/**
 * A function to resize the OpenGL window which will be used by
 * the dynamic texture generating routines.
 *
 * @param width The width of the new window
 * @param height The height of the new window
 */
void
SGTexture::resize(unsigned int width, unsigned int height)
{
    GLfloat aspect;

    // Make sure that we don't get a divide by zero exception
    if (height == 0)
        height = 1;

    // Set the viewport for the OpenGL window
    glViewport(0, 0, width, height);

    // Calculate the aspect ratio of the window
    aspect = width/height;

    // Go to the projection matrix, this gets modified by the perspective
    // calulations
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Do the perspective calculations
    gluPerspective(45.0, aspect, 1.0, 400.0);

    // Return to the modelview matrix
    glMatrixMode(GL_MODELVIEW);
}

/**
 * A function to prepare the OpenGL state machine for dynamic
 * texture generation.
 *
 * @param width The width of the texture
 * @param height The height of the texture
 */
void
SGTexture::prepare(unsigned int width, unsigned int height) {

    texture_width = width;
    texture_height = height;

    // Resize the OpenGL window to the size of our dynamic texture
    resize(texture_width, texture_height);

    glClearColor(0.0, 0.0, 0.0, 1.0);
}

/**
 * A function to generate the dynamic texture.
 *
 * The actual texture can be accessed by calling get_texture()
 *
 * @param width The width of the previous OpenGL window
 * @param height The height of the previous OpenGL window
 */
void
SGTexture::finish(unsigned int width, unsigned int height) {
    // If a texture hasn't been created then it gets created, and the contents
    // of the frame buffer gets copied into it. If the texture has already been
    // created then its contents just get updated.
    bind();
    if (!texture_data)
    {
      // Copies the contents of the frame buffer into the texture
      glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0,
                                      texture_width, texture_height, 0);

    } else {
      // Copies the contents of the frame buffer into the texture
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                                         texture_width, texture_height);
    }

    // Set the OpenGL window back to its previous size
    resize(width, height);

    // Clear the window back to black
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


void
SGTexture::read_alpha_texture(const char *name)
{
    GLubyte *lptr;
    SGTexture::ImageRec *image;
    int y;

    if (texture_data)
        delete texture_data;

    image = ImageOpen(name);
    if(!image) {
        errstr = FILE_OPEN_ERROR;
        return;
    }

    texture_width = image->xsize;
    texture_height = image->ysize;

    // printf("image->zsize = %d\n", image->zsize);

    if (image->zsize != 1) {
      ImageClose(image);
      errstr = WRONG_COUNT;
      return;
    }

    texture_data = new GLubyte[ image->xsize * image->ysize ];
    num_colors = 1;
    if (!texture_data) {
        errstr = NO_TEXTURE;
        return;
    }

    lptr = texture_data;
    for(y=0; y<image->ysize; y++) {
        ImageGetRow(image,lptr,y,0);
        lptr += image->xsize;
    }
    ImageClose(image);
}

void
SGTexture::read_rgb_texture(const char *name)
{
    GLubyte *ptr;
    GLubyte *rbuf, *gbuf, *bbuf, *abuf;
    SGTexture::ImageRec *image;
    int y;

    if (texture_data)
        delete texture_data;

    image = ImageOpen(name);
    if(!image) {
        errstr = FILE_OPEN_ERROR;
        return;
    }

    texture_width = image->xsize;
    texture_height = image->ysize;
    if (image->zsize != 3 && image->zsize != 4) {
      ImageClose(image);
      errstr = WRONG_COUNT;
      return;
    }

    texture_data = new GLubyte[ image->xsize * image->ysize * 3 ];
    num_colors = 3;
    rbuf = new GLubyte[ image->xsize ];
    gbuf = new GLubyte[ image->xsize ];
    bbuf = new GLubyte[ image->xsize ];
    abuf = new GLubyte[ image->xsize ];
    if(!texture_data || !rbuf || !gbuf || !bbuf || !abuf) {
      delete texture_data;
      delete rbuf;
      delete gbuf;
      delete bbuf;
      delete abuf;
      errstr = OUT_OF_MEMORY;
      return;
    }

    ptr = texture_data;
    for(y=0; y<image->ysize; y++) {
        if(image->zsize == 4) {
            ImageGetRow(image,rbuf,y,0);
            ImageGetRow(image,gbuf,y,1);
            ImageGetRow(image,bbuf,y,2);
            ImageGetRow(image,abuf,y,3); // discard
            rgbtorgb(rbuf,gbuf,bbuf,ptr,image->xsize);
            ptr += (image->xsize * 3);
        } else {
            ImageGetRow(image,rbuf,y,0);
            ImageGetRow(image,gbuf,y,1);
            ImageGetRow(image,bbuf,y,2);
            rgbtorgb(rbuf,gbuf,bbuf,ptr,image->xsize);
            ptr += (image->xsize * 3);
        }
    }

    ImageClose(image);
    delete rbuf;
    delete gbuf;
    delete bbuf;
    delete abuf;
}



void
SGTexture::read_rgba_texture(const char *name)
{
    GLubyte *ptr;
    GLubyte *rbuf, *gbuf, *bbuf, *abuf;
    SGTexture::ImageRec *image;
    int y;

    if (texture_data)
        delete texture_data;

    image = ImageOpen(name);
    if(!image) {
        errstr = FILE_OPEN_ERROR;
        return;
    }

    texture_width = image->xsize;
    texture_height = image->ysize;
    if (image->zsize != 3 && image->zsize != 4) {
      ImageClose(image);
      errstr = WRONG_COUNT;
      return;
    }

    texture_data = new GLubyte[ image->xsize * image->ysize * 4 ];
    num_colors = 4;
    rbuf = new GLubyte[ image->xsize ];
    gbuf = new GLubyte[ image->xsize ];
    bbuf = new GLubyte[ image->xsize ];
    abuf = new GLubyte[ image->xsize ];
    if(!texture_data || !rbuf || !gbuf || !bbuf || !abuf) {
      delete texture_data;
      delete rbuf;
      delete gbuf;
      delete bbuf;
      delete abuf;
      errstr = OUT_OF_MEMORY;
      return;
    }

    ptr = texture_data;
    memset(abuf, 255, image->xsize);
    for(y=0; y<image->ysize; y++) {
        if(image->zsize == 4) {
            ImageGetRow(image,rbuf,y,0);
            ImageGetRow(image,gbuf,y,1);
            ImageGetRow(image,bbuf,y,2);
            ImageGetRow(image,abuf,y,3);
            rgbatorgba(rbuf,gbuf,bbuf,abuf,ptr,image->xsize);
            ptr += (image->xsize * 4);
        } else {
            ImageGetRow(image,rbuf,y,0);
            ImageGetRow(image,gbuf,y,1);
            ImageGetRow(image,bbuf,y,2);
            rgbatorgba(rbuf,gbuf,bbuf,abuf,ptr,image->xsize);
            ptr += (image->xsize * 3);
        }
    }

    ImageClose(image);
    delete rbuf;
    delete gbuf;
    delete bbuf;
    delete abuf;
}

void
SGTexture::read_raw_texture(const char *name)
{
    GLubyte *ptr;
    SGTexture::ImageRec *image;
    int y;

    if (texture_data)
        delete texture_data;

    image = RawImageOpen(name);

    if(!image) {
        errstr = FILE_OPEN_ERROR;
        return;
    }

    texture_width = 256;
    texture_height = 256;

    texture_data = new GLubyte[ 256 * 256 * 3 ];
    if(!texture_data) {
      errstr = OUT_OF_MEMORY;
      return;
    }

    ptr = texture_data;
    for(y=0; y<256; y++) {
        gzread(image->gzfile, ptr, 256*3);
        ptr+=256*3;
    }
    ImageClose(image);
}

void
SGTexture::read_r8_texture(const char *name)
{
    unsigned char c[1];
    GLubyte *ptr;
    SGTexture::ImageRec *image;
    int xy;

    if (texture_data)
        delete texture_data;

    //it wouldn't make sense to write a new function ...
    image = RawImageOpen(name);

    if(!image) {
        errstr = FILE_OPEN_ERROR;
        return;
    }

    texture_width = 256;
    texture_height = 256;

    texture_data = new GLubyte [ 256 * 256 * 3 ];
    if(!texture_data) {
        errstr = OUT_OF_MEMORY;
        return;
    }

    ptr = texture_data;
    for(xy=0; xy<(256*256); xy++) {
        gzread(image->gzfile, c, 1);

        //look in the table for the right colours
        ptr[0]=msfs_colour[c[0]][0];
        ptr[1]=msfs_colour[c[0]][1];
        ptr[2]=msfs_colour[c[0]][2];

        ptr+=3;
    }
    ImageClose(image);
}


void
SGTexture::write_texture(const char *name) {
   SGTexture::ImageRec *image = ImageWriteOpen(name);

   for (int c=0; c<num_colors; c++) {
      GLubyte *ptr = texture_data + c;
      for (int y=0; y<texture_height; y++) {
         for (int x=0; x<texture_width; x++) {
	    image->tmp[x]=*ptr;
            ptr = ptr + num_colors;
         }
         fwrite(image->tmp, 1, texture_width, image->file);
      }
   }

   ImageClose(image);
}


void
SGTexture::set_pixel(GLuint x, GLuint y, GLubyte *c)
{
    if (!texture_data) {
        errstr = NO_TEXTURE;
        return;
    }

    unsigned int pos = (x + y*texture_width) * num_colors;
    memcpy(texture_data+pos, c, num_colors);
}


GLubyte *
SGTexture::get_pixel(GLuint x, GLuint y)
{
    static GLubyte c[4] = {0, 0, 0, 0};

    if (!texture_data) {
        errstr = NO_TEXTURE;
        return c;
    }

    unsigned int pos = (x + y*texture_width)*num_colors;
    memcpy(c, texture_data + pos, num_colors);

    return c;
}

SGTexture::ImageRec *
SGTexture::ImageOpen(const char *fileName)
{
     union {
       int testWord;
       char testByte[4];
     } endianTest;

    SGTexture::ImageRec *image;
    int swapFlag;
    int x;

    endianTest.testWord = 1;
    if (endianTest.testByte[0] == 1) {
        swapFlag = 1;
    } else {
        swapFlag = 0;
    }

    image = new SGTexture::ImageRec;
    memset(image, 0, sizeof(SGTexture::ImageRec));
    if (image == 0) {
        errstr = OUT_OF_MEMORY;
        return 0;
    }
    if ((image->gzfile = gzopen(fileName, "rb")) == 0) {
      errstr = FILE_OPEN_ERROR;
      return 0;
    }

    gzread(image->gzfile, image, 12);

    if (swapFlag) {
        ConvertShort(&image->imagic, 6);
    }

    image->tmp = new GLubyte[ image->xsize * 256 ];
    if (image->tmp == 0) {
        errstr = OUT_OF_MEMORY;
        return 0;
    }

    if ((image->type & 0xFF00) == 0x0100) {
        x = image->ysize * image->zsize * (int) sizeof(unsigned);
        image->rowStart = new unsigned[x];
        image->rowSize = new int[x];
        if (image->rowStart == 0 || image->rowSize == 0) {
            errstr = OUT_OF_MEMORY;
            return 0;
        }
        image->rleEnd = 512 + (2 * x);
        gzseek(image->gzfile, 512, SEEK_SET);
        gzread(image->gzfile, image->rowStart, x);
        gzread(image->gzfile, image->rowSize, x);
        if (swapFlag) {
            ConvertUint(image->rowStart, x/(int) sizeof(unsigned));
            ConvertUint((unsigned *)image->rowSize, x/(int) sizeof(int));
        }
    }
    return image;
}


void
SGTexture::ImageClose(SGTexture::ImageRec *image) {
    if (image->gzfile)  gzclose(image->gzfile);
    if (image->file)    fclose(image->file);
    delete image->tmp;
    delete image;
}

SGTexture::ImageRec *
SGTexture::RawImageOpen(const char *fileName)
{
     union {
       int testWord;
       char testByte[4];
     } endianTest;

    SGTexture::ImageRec *image;
    int swapFlag;

    endianTest.testWord = 1;
    if (endianTest.testByte[0] == 1) {
        swapFlag = 1;
    } else {
        swapFlag = 0;
    }

    image = new SGTexture::ImageRec;
    memset(image, 0, sizeof(SGTexture::ImageRec));
    if (image == 0) {
        errstr = OUT_OF_MEMORY;
        return 0;
    }
    if ((image->gzfile = gzopen(fileName, "rb")) == 0) {
      errstr = FILE_OPEN_ERROR;
      return 0;
    }

    gzread(image->gzfile, image, 12);

    if (swapFlag) {
        ConvertShort(&image->imagic, 6);
    }


    //just allocate a pseudo value as I'm too lazy to change ImageClose()...
    image->tmp = new GLubyte;

    if (image->tmp == 0) {
        errstr = OUT_OF_MEMORY;
        return 0;
    }

    return image;
}

SGTexture::ImageRec *
SGTexture::ImageWriteOpen(const char *fileName)
{
    union {
        int testWord;
        char testByte[4];
    } endianTest;
    ImageRec* image;
    int swapFlag;
    int x;

    endianTest.testWord = 1;
    if (endianTest.testByte[0] == 1) {
        swapFlag = 1;
    } else {
        swapFlag = 0;
    }

    image = new SGTexture::ImageRec;
    memset(image, 0, sizeof(SGTexture::ImageRec));
    if (image == 0) {
        errstr = OUT_OF_MEMORY;
        return 0;
    }
    if ((image->file = fopen(fileName, "wb")) == 0) {
        errstr = FILE_OPEN_ERROR;
        return 0;
    }

    image->imagic = 474;
    image->type = 0x0001;
    image->dim = (num_colors > 1) ? 3 : 2;
    image->xsize = texture_width;
    image->ysize = texture_height;
    image->zsize = num_colors;

    fwrite(image, 1, 12, image->file);

    fseek(image->file, 512, SEEK_SET);

    if (swapFlag) {
        ConvertShort(&image->imagic, 6);
    }

    image->tmp = new GLubyte[ image->xsize * 256 ];
    if (image->tmp == 0) {
        errstr = OUT_OF_MEMORY;
        return 0;
    }

    if ((image->type & 0xFF00) == 0x0100) {
        x = image->ysize * image->zsize * (int) sizeof(unsigned);
        image->rowStart = new unsigned[x];
        image->rowSize = new int[x];
        if (image->rowStart == 0 || image->rowSize == 0) {
            errstr = OUT_OF_MEMORY;
            return 0;
        }
        image->rleEnd = 512 + (2 * x);
        fseek(image->file, 512, SEEK_SET);
        fwrite(image->rowStart, 1, x, image->file);
        fwrite(image->rowSize, 1, x, image->file);
        if (swapFlag) {
            ConvertUint(image->rowStart, x/(int) sizeof(unsigned));
            ConvertUint((unsigned *)image->rowSize, x/(int) sizeof(int));
        }
    }

    return image;

}

void
SGTexture::ImageGetRow(SGTexture::ImageRec *image, GLubyte *buf, int y, int z) {
    GLubyte *iPtr, *oPtr, pixel;
    int count;

    if ((image->type & 0xFF00) == 0x0100) {
        gzseek(image->gzfile, (long) image->rowStart[y+z*image->ysize], SEEK_SET);
        gzread(image->gzfile, image->tmp,
               (unsigned int)image->rowSize[y+z*image->ysize]);

        iPtr = image->tmp;
        oPtr = buf;
        for (;;) {
            pixel = *iPtr++;
            count = (int)(pixel & 0x7F);
            if (!count) {
                errstr = WRONG_COUNT;
                return;
            }
            if (pixel & 0x80) {
                while (count--) {
                    *oPtr++ = *iPtr++;
                }
            } else {
                pixel = *iPtr++;
                while (count--) {
                    *oPtr++ = pixel;
                }
            }
        }
    } else {
        gzseek(image->gzfile, 512+(y*image->xsize)+(z*image->xsize*image->ysize),
              SEEK_SET);
        gzread(image->gzfile, buf, image->xsize);
    }
}

void
SGTexture::ImagePutRow(SGTexture::ImageRec *image, GLubyte *buf, int y, int z) {
    GLubyte *iPtr, *oPtr, pixel;
    int count;

    if ((image->type & 0xFF00) == 0x0100) {
        fseek(image->file, (long) image->rowStart[y+z*image->ysize], SEEK_SET);
        fread( image->tmp, 1, (unsigned int)image->rowSize[y+z*image->ysize],
               image->file);

        iPtr = image->tmp;
        oPtr = buf;
        for (;;) {
            pixel = *iPtr++;
            count = (int)(pixel & 0x7F);
            if (!count) {
                errstr = WRONG_COUNT;
                return;
            }
            if (pixel & 0x80) {
                while (count--) {
                    *oPtr++ = *iPtr++;
                }
            } else {
                pixel = *iPtr++;
                while (count--) {
                    *oPtr++ = pixel;
                }
            }
        }
    } else {
        fseek(image->file, 512+(y*image->xsize)+(z*image->xsize*image->ysize),
              SEEK_SET);
        fread(buf, 1, image->xsize, image->file);
    }
}


void
SGTexture::rgbtorgb(GLubyte *r, GLubyte *g, GLubyte *b, GLubyte *l, int n) {
    while(n--) {
        l[0] = r[0];
        l[1] = g[0];
        l[2] = b[0];
        l += 3; r++; g++; b++;
    }
}

void
SGTexture::rgbatorgba(GLubyte *r, GLubyte *g, GLubyte *b, GLubyte *a,
                      GLubyte *l, int n) {
    while(n--) {
        l[0] = r[0];
        l[1] = g[0];
        l[2] = b[0];
        l[3] = a[0];
        l += 4; r++; g++; b++; a++;
    }
}


void
SGTexture::ConvertShort(unsigned short *array, unsigned int length) {
    unsigned short b1, b2;
    unsigned char *ptr;

    ptr = (unsigned char *)array;
    while (length--) {
        b1 = *ptr++;
        b2 = *ptr++;
        *array++ = (b1 << 8) | (b2);
    }
}


void
SGTexture::ConvertUint(unsigned *array, unsigned int length) {
    unsigned int b1, b2, b3, b4;
    unsigned char *ptr;

    ptr = (unsigned char *)array;
    while (length--) {
        b1 = *ptr++;
        b2 = *ptr++;
        b3 = *ptr++;
        b4 = *ptr++;
        *array++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4);
    }
}


void
SGTexture::make_monochrome(GLubyte r, GLubyte g, GLubyte b) {

   if (num_colors != 3)
      return;

   GLubyte ap[3];
   for (int y=0; y<texture_height; y++)
      for (int x=0; x<texture_width; x++)
      {
         GLubyte *rgb = get_pixel(x,y);
         GLubyte avg = (rgb[0] + rgb[1] + rgb[2]) / 3;

         ap[0] = avg*r/255;
         ap[1] = avg*g/255;
         ap[2] = avg*b/255;

         set_pixel(x,y,ap);
      }
}
