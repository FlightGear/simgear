/**
 * \file texture.cxx
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

#include <GL/glu.h>

#include <stdlib.h>	// malloc()

#include "texture.hxx"
#include "colours.h"

SGTexture::SGTexture()
{
    texture_data = NULL;
}

SGTexture::~SGTexture()
{
   if (texture_data)
      free(texture_data);
}

void
SGTexture::bind()
{
    if (!texture_data) {
#ifdef GL_VERSION_1_1
        glGenTextures(1, &texture_id);

#elif GL_EXT_texture_object
        glGenTexturesEXT(1, &texture_id);
#endif
    }

#ifdef GL_VERSION_1_1
    glBindTexture(GL_TEXTURE_2D, texture_id);

#elif GL_EXT_texture_object
    glBindTextureEXT(GL_TEXTURE_2D, texture_id);
#endif

    if (!texture_data) {
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

    // Clear the contents of the screen buffer to blue
    glClearColor(0.0, 0.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Turn off texturing (don't want the torus to be texture);
    glDisable(GL_TEXTURE_2D);
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
        free(texture_data);

    image = ImageOpen(name);
    if(!image) {
        return;
    }

    texture_width = image->xsize;
    texture_height = image->ysize;

    // printf("image->zsize = %d\n", image->zsize);

    if (image->zsize != 1) {
      ImageClose(image);
      return;
    }

    texture_data = (GLubyte *)
           malloc(image->xsize*image->ysize*sizeof(GLubyte));

    if (!texture_data)
        return;

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
        free(texture_data);

    image = ImageOpen(name);
    if(!image)
        return;

    texture_width = image->xsize;
    texture_height = image->ysize;
    if (image->zsize != 3 && image->zsize != 4) {
      ImageClose(image);
      return;
    }

    texture_data = (GLubyte *)malloc(image->xsize*image->ysize*sizeof(GLubyte)*3);
    rbuf = (GLubyte *)malloc(image->xsize*sizeof(GLubyte));
    gbuf = (GLubyte *)malloc(image->xsize*sizeof(GLubyte));
    bbuf = (GLubyte *)malloc(image->xsize*sizeof(GLubyte));
    abuf = (GLubyte *)malloc(image->xsize*sizeof(GLubyte));
    if(!texture_data || !rbuf || !gbuf || !bbuf || !abuf) {
      if (texture_data) free(texture_data);
      if (rbuf) free(rbuf);
      if (gbuf) free(gbuf);
      if (bbuf) free(bbuf);
      if (abuf) free(abuf);
      return;
    }

    ptr = texture_data;
    for(y=0; y<image->ysize; y++) {
        if(image->zsize == 4) {
            ImageGetRow(image,rbuf,y,0);
            ImageGetRow(image,gbuf,y,1);
            ImageGetRow(image,bbuf,y,2);
            ImageGetRow(image,abuf,y,3);  /* Discard. */
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
    free(rbuf);
    free(gbuf);
    free(bbuf);
    free(abuf);
}

void
SGTexture::read_raw_texture(const char *name)
{
    GLubyte *ptr;
    SGTexture::ImageRec *image;
    int y;

    if (texture_data)
        free(texture_data);

    image = RawImageOpen(name);

    if(!image)
        return;

    texture_width = 256;
    texture_height = 256;

    texture_data = (GLubyte *)malloc(256*256*sizeof(GLubyte)*3);
    if(!texture_data)
      return;

    ptr = texture_data;
    for(y=0; y<256; y++) {
                sgread(image->file, ptr, 256*3);
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
        free(texture_data);

    //it wouldn't make sense to write a new function ...
    image = RawImageOpen(name);

    if(!image)
        return;

    texture_width = 256;
    texture_height = 256;

    texture_data = (GLubyte *)malloc(256*256*sizeof(GLubyte)*3);
    if(!texture_data)
        return;

    ptr = texture_data;
    for(xy=0; xy<(256*256); xy++) {
        sgread(image->file,c,1);

        //look in the table for the right colours
        ptr[0]=msfs_colour[c[0]][0];
        ptr[1]=msfs_colour[c[0]][1];
        ptr[2]=msfs_colour[c[0]][2];

        ptr+=3;
    }
    ImageClose(image);
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

    image = (SGTexture::ImageRec *)malloc(sizeof(SGTexture::ImageRec));
    if (image == NULL) {
        // fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    if ((image->file = sgopen(fileName, "rb")) == NULL) {
      return NULL;
    }

    // fread(image, 1, 12, image->file);
    sgread(image->file, image, 12);

    if (swapFlag) {
        ConvertShort(&image->imagic, 6);
    }

    image->tmp = (GLubyte *)malloc(image->xsize*256);
    if (image->tmp == NULL) {
        // fprintf(stderr, "\nOut of memory!\n");
        exit(1);
    }

    if ((image->type & 0xFF00) == 0x0100) {
        x = image->ysize * image->zsize * (int) sizeof(unsigned);
        image->rowStart = (unsigned *)malloc(x);
        image->rowSize = (int *)malloc(x);
        if (image->rowStart == NULL || image->rowSize == NULL) {
            // fprintf(stderr, "\nOut of memory!\n");
            exit(1);
        }
        image->rleEnd = 512 + (2 * x);
        sgseek(image->file, 512, SEEK_SET);
        // fread(image->rowStart, 1, x, image->file);
        sgread(image->file, image->rowStart, x);
        // fread(image->rowSize, 1, x, image->file);
        sgread(image->file, image->rowSize, x);
        if (swapFlag) {
            ConvertUint(image->rowStart, x/(int) sizeof(unsigned));
            ConvertUint((unsigned *)image->rowSize, x/(int) sizeof(int));
        }
    }
    return image;
}


void
SGTexture::ImageClose(SGTexture::ImageRec *image) {
    sgclose(image->file);
    free(image->tmp);
    free(image);
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

    image = (SGTexture::ImageRec *)malloc(sizeof(SGTexture::ImageRec));
    if (image == NULL) {
        // fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    if ((image->file = sgopen(fileName, "rb")) == NULL) {
      return NULL;
    }

    sgread(image->file, image, 12);

    if (swapFlag) {
        ConvertShort(&image->imagic, 6);
    }


    //just allocate a pseudo value as I'm too lazy to change ImageClose()...
    image->tmp = (GLubyte *)malloc(1);

    if (image->tmp == NULL) {
        // fprintf(stderr, "\nOut of memory!\n");
        exit(1);
    }

    return image;
}

void
SGTexture::ImageGetRow(SGTexture::ImageRec *image, GLubyte *buf, int y, int z) {
    GLubyte *iPtr, *oPtr, pixel;
    int count;

    if ((image->type & 0xFF00) == 0x0100) {
        sgseek(image->file, (long) image->rowStart[y+z*image->ysize], SEEK_SET);        // fread(image->tmp, 1, (unsigned int)image->rowSize[y+z*image->ysize],
        //      image->file);
        sgread(image->file, image->tmp,
               (unsigned int)image->rowSize[y+z*image->ysize]);

        iPtr = image->tmp;
        oPtr = buf;
        for (;;) {
            pixel = *iPtr++;
            count = (int)(pixel & 0x7F);
            if (!count) {
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
        sgseek(image->file, 512+(y*image->xsize)+(z*image->xsize*image->ysize),
              SEEK_SET);
        // fread(buf, 1, image->xsize, image->file);
        sgread(image->file, buf, image->xsize);
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

