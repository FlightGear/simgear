class GlBitmap
{
public:
	GlBitmap( GLenum mode=GL_RGB, GLint width=0, GLint height=0, GLubyte *bitmap=0 );
	~GlBitmap();
	GLubyte *getBitmap();
	void copyBitmap( GlBitmap *from, GLint at_x, GLint at_y );
private:
	GLint m_bytesPerPixel;
	GLint m_width;
	GLint m_height;
	GLint m_bitmapSize;
	GLubyte *m_bitmap;
};
