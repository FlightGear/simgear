
#ifndef __SG_TEXTURE_HXX
#define __SG_TEXTURE_HXX 1

#include <GL/gl.h>
#include <zlib.h>

class SGTexture {

private:

    GLuint texture_id;
    GLubyte *texture_data;

    GLsizei texture_width;
    GLsizei texture_height;

    void resize(unsigned int width = 256, unsigned int height = 256);

protected:

    typedef struct _ImageRec {
        unsigned short imagic;
        unsigned short type;
        unsigned short dim;
        unsigned short xsize, ysize, zsize;
        unsigned int min, max;
        unsigned int wasteBytes;
        char name[80];
        unsigned long colorMap;
        gzFile file;
        GLubyte *tmp;
        unsigned long rleEnd;
        unsigned int *rowStart;
        int *rowSize;
    } ImageRec;

    void ConvertUint(unsigned *array, unsigned int length);
    void ConvertShort(unsigned short *array, unsigned int length);
    void rgbtorgb(GLubyte *r, GLubyte *g, GLubyte *b, GLubyte *l, int n);

    ImageRec *ImageOpen(const char *fileName);
    ImageRec *RawImageOpen(const char *fileName);
    void ImageClose(ImageRec *image);
    void ImageGetRow(ImageRec *image, GLubyte *buf, int y, int z);

public:

    SGTexture();
    ~SGTexture();

    /* Copyright (c) Mark J. Kilgard, 1997.  */
    void read_alpha_texture(const char *name);
    void read_rgb_texture(const char *name);
    void read_raw_texture(const char *name);
    void read_r8_texture(const char *name);

    inline bool usable() { return texture_data ? true : false; }

    inline GLuint id() { return texture_id; }
    inline GLubyte *texture() { return texture_data; }

    inline int width() { return texture_width; }
    inline int height() { return texture_height; }

    void prepare(unsigned int width = 256, unsigned int height = 256);
    void finish(unsigned int width, unsigned int height);

    void bind();
    inline void select() {
        // if (texture_data)
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB,
                          texture_width, texture_height, 0,
                          GL_RGB, GL_UNSIGNED_BYTE, texture_data );
    }
};

#endif

