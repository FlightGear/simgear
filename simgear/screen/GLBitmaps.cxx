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
#include <simgear/xgl/xgl.h>

#include "GLBitmaps.h"

GlBitmap::GlBitmap( GLenum mode, GLint width, GLint height, GLubyte *bitmap )
: m_bytesPerPixel(mode==GL_RGB?3:4), m_width(width), m_height(height), m_bitmap(NULL)
{
	m_bitmapSize = m_bytesPerPixel*m_width*m_height;
	if ( !m_bitmapSize )
	{
		GLint vp[4];
		glGetIntegerv( GL_VIEWPORT, vp );
		m_width = vp[2];
		m_height = vp[3];
		m_bitmapSize = m_bytesPerPixel*m_width*m_height;
	}
	m_bitmap = (GLubyte *)malloc( m_bitmapSize );
	if ( bitmap ) memcpy( m_bitmap, bitmap, m_bitmapSize );
	else glReadPixels( 0,0, m_width,m_height, mode, GL_UNSIGNED_BYTE, m_bitmap );
}

GlBitmap::~GlBitmap( )
{
	if ( m_bitmap ) free( m_bitmap );
}

GLubyte *GlBitmap::getBitmap()
{
	return m_bitmap;
}

void GlBitmap::copyBitmap( GlBitmap *from, GLint at_x, GLint at_y )
{
	GLint newWidth = at_x + from->m_width;
	GLint newHeight = at_y + from->m_height;
	if ( newWidth < m_width ) newWidth = m_width;
	if ( newHeight < m_height ) newHeight = m_height;
	m_bitmapSize = m_bytesPerPixel*newWidth*newHeight;
	GLubyte *newBitmap = (GLubyte *)malloc( m_bitmapSize );
	GLint x,y;
	for ( y=0; y<m_height; y++ )
	{
		GLubyte *s = m_bitmap + m_bytesPerPixel * (y * m_width);
		GLubyte *d = newBitmap + m_bytesPerPixel * (y * newWidth);
		memcpy( d, s, m_bytesPerPixel * m_width );
	}
	m_width = newWidth;
	m_height = newHeight;
	free( m_bitmap );
	m_bitmap = newBitmap;
	for ( y=0; y<from->m_height; y++ )
	{
		GLubyte *s = from->m_bitmap + from->m_bytesPerPixel * (y * from->m_width);
		GLubyte *d = m_bitmap + m_bytesPerPixel * ((at_y+y) * m_width + at_x);
		for ( x=0; x<from->m_width; x++ )
		{
			d[0] = s[0];
			d[1] = s[1];
			d[2] = s[2];
			if ( m_bytesPerPixel == 4 )
			{
				d[3] = (from->m_bytesPerPixel == 4) ? s[3] : 0;
			}
			s += from->m_bytesPerPixel;
			d += m_bytesPerPixel;
		}
	}
}

