
class CGlPrinter
{
public:
	typedef enum { RENDER_TO_PRINTER, RENDER_TO_BITMAP, READ_BITMAP, PRINT_BITMAP } PRINTMODE;
public:
	inline CGlPrinter( PRINTMODE mode = READ_BITMAP );
	inline ~CGlPrinter();
	inline bool Begin( char *title, int w=0, int h=0 );
	inline void End( GLubyte *bm=NULL );
	inline double GetAspect();
	inline int GetHorzRes();
	inline int GetVertRes();

private:
	PRINTMODE m_printMode;
	PRINTDLG m_printDLG;
	BITMAPINFO m_bitmapInfo;
	void *m_bitmap;
	HBITMAP bitmapHandle;
	HDC m_bitmapDC;
	HGLRC m_printerHGLRC;
	HGLRC m_bitmapHGLRC;
	int m_pageWidth;				// Dimension of printer page (x)
	int m_pageHeight;				// Dimension of printer page (y)
};

inline double CGlPrinter::GetAspect()
{
	return (double)m_pageWidth/(double)m_pageHeight;
}

inline int CGlPrinter::GetHorzRes()
{
	return m_pageWidth;
}

inline int CGlPrinter::GetVertRes()
{
	return m_pageHeight;
}

inline CGlPrinter::CGlPrinter( PRINTMODE mode ) :
	m_printerHGLRC( 0 ), m_bitmapHGLRC( 0 ), m_printMode( mode ), m_bitmap( NULL )
{
}

inline CGlPrinter::~CGlPrinter()
{
}

inline bool CGlPrinter::Begin( char *title, int w, int h  )
{
	// Pixel format for Printer Device context
	static PIXELFORMATDESCRIPTOR pPrintfd = {
		sizeof(PIXELFORMATDESCRIPTOR),			// Size of this structure
		1,                                      // Version of this structure    
		PFD_DRAW_TO_WINDOW |                    // Draw to Window (not to m_bitmap)
		PFD_SUPPORT_OPENGL |                    // Support OpenGL calls 
		PFD_SUPPORT_GDI |                       // Allow GDI drawing in window too
		PFD_DEPTH_DONTCARE,						// Don't care about depth buffering
		PFD_TYPE_RGBA,                          // RGBA Color mode
		24,                                     // Want 24bit color 
		0,0,0,0,0,0,                            // Not used to select mode
		0,0,                                    // Not used to select mode
		0,0,0,0,0,                              // Not used to select mode
		0,                                      // Size of depth buffer
		0,                                      // Not used to select mode
		0,                                      // Not used to select mode
		0,				                        // Not used to select mode
		0,                                      // Not used to select mode
		0,0,0 };                                // Not used to select mode

	DOCINFO docInfo;        // Document Info
	int nPixelFormat;		// Pixel format requested

	// Get printer information
	memset(&m_printDLG,0,sizeof(PRINTDLG));
	m_printDLG.lStructSize = sizeof(PRINTDLG);
	m_printDLG.hwndOwner = GetForegroundWindow();
	m_printDLG.hDevMode = NULL;
	m_printDLG.hDevNames = NULL;
	m_printDLG.Flags = PD_RETURNDC | PD_ALLPAGES;

	// Display printer dialog
	if(!PrintDlg(&m_printDLG))
	{
		printf( "PrintDlg() failed %lx\n", GetLastError() );
		return false;
	}

	// Get the dimensions of the page
	m_pageWidth = GetDeviceCaps(m_printDLG.hDC, HORZRES);
	m_pageHeight = GetDeviceCaps(m_printDLG.hDC, VERTRES);

	// Initialize DocInfo structure
	docInfo.cbSize = sizeof(DOCINFO);
	docInfo.lpszDocName = title;
	docInfo.lpszOutput = NULL;


	// Choose a pixel format that best matches that described in pfd
	nPixelFormat = ChoosePixelFormat(m_printDLG.hDC, &pPrintfd);
	// Watch for no pixel format available for this printer
	if(nPixelFormat == 0)
		{
		// Delete the printer context
		DeleteDC(m_printDLG.hDC);

		printf( "ChoosePixelFormat() failed %lx\n", GetLastError() );
		return false;
		}

	// Start the document and page
	StartDoc(m_printDLG.hDC, &docInfo);
	StartPage(m_printDLG.hDC);
	
	// Set the pixel format for the device context, but watch for failure
	if(!SetPixelFormat(m_printDLG.hDC, nPixelFormat, &pPrintfd))
		{
		// Delete the printer context
		DeleteDC(m_printDLG.hDC);

		printf( "SetPixelFormat() failed %lx\n", GetLastError() );
		return false;
		}

	// Create the Rendering context and make it current
	if ( m_printMode == RENDER_TO_PRINTER )
	{
		m_printerHGLRC = wglCreateContext(m_printDLG.hDC);
		wglMakeCurrent(m_printDLG.hDC, m_printerHGLRC);
	}
	else 
	{
		memset( &m_bitmapInfo, 0, sizeof(BITMAPINFO) );
		m_bitmapInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
		if ( m_printMode == READ_BITMAP )
		{
			GLint vp[4];
			glGetIntegerv( GL_VIEWPORT, vp );
			m_bitmapInfo.bmiHeader.biWidth = vp[2];
			m_bitmapInfo.bmiHeader.biHeight = (vp[3] + 3) & ~3;
		}
		else
		{
			if ( m_printMode == RENDER_TO_BITMAP )
			{
				m_bitmapInfo.bmiHeader.biWidth = m_pageWidth;
				m_bitmapInfo.bmiHeader.biHeight = m_pageHeight;
			}
			else
			{
				// PRINT_BITMAP
				m_bitmapInfo.bmiHeader.biWidth = w;
				m_bitmapInfo.bmiHeader.biHeight = h;
			}
		}
		m_bitmapInfo.bmiHeader.biPlanes = 1;
		m_bitmapInfo.bmiHeader.biBitCount = 24;
		m_bitmapInfo.bmiHeader.biCompression = BI_RGB;
		m_bitmapInfo.bmiHeader.biSizeImage = m_bitmapInfo.bmiHeader.biWidth*m_bitmapInfo.bmiHeader.biHeight*3;
		m_bitmapInfo.bmiHeader.biXPelsPerMeter = 2952; // 75dpi
		m_bitmapInfo.bmiHeader.biYPelsPerMeter = 2952; // 75dpi
		m_bitmapInfo.bmiHeader.biClrUsed = 0;
		m_bitmapInfo.bmiHeader.biClrImportant = 0;
		bitmapHandle = CreateDIBSection( NULL, &m_bitmapInfo, DIB_RGB_COLORS, &m_bitmap, NULL, 0);
		m_bitmapDC = CreateCompatibleDC( NULL );
		if ( m_bitmapDC == NULL )
		{
			DeleteDC(m_printDLG.hDC);
			printf( "CreateCompatibleDC() failed %lx\n", GetLastError() );
			return false;
		}
		if ( SelectObject( m_bitmapDC, bitmapHandle ) == NULL )
		{
			DeleteDC(m_printDLG.hDC);
			DeleteDC(m_bitmapDC);
			printf( "SelectObject() failed %lx\n", GetLastError() );
			return false;
		}
		PIXELFORMATDESCRIPTOR pfd;
		memset( &pfd, 0, sizeof(PIXELFORMATDESCRIPTOR) );
		pfd.nVersion = 1 ;
		pfd.dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL;
		pfd.iPixelType = PFD_TYPE_RGBA ; 
		pfd.cColorBits = 24;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int pixelFormat = ::ChoosePixelFormat(m_bitmapDC, &pfd);
		SetPixelFormat (m_bitmapDC, pixelFormat, &pfd);
		if ( m_printMode == RENDER_TO_BITMAP )
		{
			m_bitmapHGLRC = wglCreateContext( m_bitmapDC );
			wglMakeCurrent( m_bitmapDC, m_bitmapHGLRC );
		}
	}

	if ( m_printMode == RENDER_TO_PRINTER || m_printMode == RENDER_TO_BITMAP )
	{
		// Set viewing volume info
		//glViewport(0,0,m_pageWidth/2,m_pageHeight/2);	// Put this in to restrict area of page.

		GLfloat nRange = 100.0f;

		// Reset projection matrix stack
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		// Establish clipping volume (left, right, bottom, top, near, far)
		// This keeps the perspective square regardless of window or page size

		if (m_pageHeight <= m_pageWidth) 
		{
			glOrtho (-nRange, nRange, -nRange*m_pageHeight/m_pageWidth, nRange*m_pageHeight/m_pageWidth, -nRange, nRange);
		}
		else 
		{
			glOrtho (-nRange*m_pageWidth/m_pageHeight, nRange*m_pageWidth/m_pageHeight, -nRange, nRange, -nRange, nRange);
		}

		// Reset Model view matrix stack
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(270.0f, 1.0, 0.0, 0.0);
	}

	return true;
}

inline void CGlPrinter::End( GLubyte *bm )
{
	if ( m_printDLG.hDC )
	{
		glFinish();

		if ( m_printMode == RENDER_TO_PRINTER )
		{
			if ( m_printerHGLRC )
			{
				wglDeleteContext( m_printerHGLRC );
			}
		}
		else
		{
			if ( m_printMode == RENDER_TO_BITMAP )
			{
				BitBlt( m_printDLG.hDC, 0,0, m_pageWidth,m_pageHeight, m_bitmapDC, 0,0, SRCCOPY );
			}
			else
			{
				if ( m_printMode == READ_BITMAP )
				{
					glPixelStorei( GL_PACK_ALIGNMENT, 4 );
					glPixelStorei( GL_PACK_ROW_LENGTH, 0 );
					glPixelStorei( GL_PACK_SKIP_ROWS, 0 );
					glPixelStorei( GL_PACK_SKIP_PIXELS, 0 );
					//GLubyte *tempbitmap = (GLubyte *) malloc(m_bitmapInfo.bmiHeader.biWidth*m_bitmapInfo.bmiHeader.biHeight*4);
					glReadPixels( 0,0, m_bitmapInfo.bmiHeader.biWidth,m_bitmapInfo.bmiHeader.biHeight,
						GL_RGB, GL_UNSIGNED_BYTE, m_bitmap );
				}
				else
				{
					//PRINT_BITMAP
					memcpy( m_bitmap, bm, m_bitmapInfo.bmiHeader.biSizeImage );
				}
				int i,j;
				GLubyte *rgb, temp;
				for ( i = 0; i < m_bitmapInfo.bmiHeader.biHeight; i++ )
				{
					for ( j = 0, rgb = ((GLubyte *)m_bitmap) + i * m_bitmapInfo.bmiHeader.biWidth * 3;
					j < m_bitmapInfo.bmiHeader.biWidth;
					j++, rgb +=3 )
					{
						temp = rgb[0];
						rgb[0] = rgb[2];
						rgb[2] = temp;
					}
				}
				long width = m_pageWidth;
				long height = width * m_bitmapInfo.bmiHeader.biHeight / m_bitmapInfo.bmiHeader.biWidth;
				if ( height > m_pageHeight )
				{
					height = m_pageHeight;
					width = height * m_bitmapInfo.bmiHeader.biWidth / m_bitmapInfo.bmiHeader.biHeight;
				}
				long xoffset = (m_pageWidth - width) / 2;
				long yoffset = (m_pageHeight - height) / 2;
				StretchBlt( m_printDLG.hDC, xoffset, yoffset, width, height, m_bitmapDC, 0, 0,
					m_bitmapInfo.bmiHeader.biWidth, m_bitmapInfo.bmiHeader.biHeight, SRCCOPY );
			}
			if ( m_bitmapDC ) 
			{
				DeleteDC( m_bitmapDC );
			}
			if ( bitmapHandle )
			{
				DeleteObject( bitmapHandle );
			}
			if ( m_bitmapHGLRC )
			{
				wglDeleteContext( m_bitmapHGLRC );
			}
		}

		/* EndPage... */
		if ( EndPage( m_printDLG.hDC ) <=0 )
		{
			printf( "EndPage() failed\n" );
		}

		/* EndDoc... */
		if ( EndDoc( m_printDLG.hDC ) <=0 )
		{
			printf( "EndDoc() failed\n" );
		}

		// Delete the printer context when done with it
		DeleteDC(m_printDLG.hDC);
		
		// Restore window rendering context
		//wglMakeCurrent(hDC, hRC);
		wglMakeCurrent( NULL, NULL );

	}
}
