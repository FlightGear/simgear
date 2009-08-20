
#ifdef HAVE_CONFIG_H
#  include <simgear/simgear_config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/compiler.h>

#include <osg/GL>

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/screen/extensions.hxx>
#include <simgear/screen/RenderTexture.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// DEBUG - add a lot of noise
//#ifndef _DEBUG
//#define _DEBUG
//#endif

#if defined (_DEBUG) 
const char * get_attr_name( int val, int * pdef );
#define dbg_printf printf
#else
#if defined (__GNUC__)
#define dbg_printf(format,args...) ((void)0)
#else // defined (__GNUC__)
#define dbg_printf(
#endif // defined (__GNUC__)
#endif // defined (_DEBUG)


void Reshape(int w, int h);

GLuint      iTextureProgram     = 0;
GLuint      iPassThroughProgram = 0;

RenderTexture *rt = NULL;

float       rectAngle         = 0;
float       torusAngle        = 0;
bool        bTorusMotion      = true;
bool        bRectMotion       = true;
bool        bShowDepthTexture = false;

static const char *g_modeTestStrings[] = 
{
    "rgb tex2D",
    "rgba tex2D depthTex2D",
    "rgba=8 depthTexRECT ctt",
    "rgba samples=4 tex2D ctt",
    "rgba=8 tex2D mipmap",
    "rgb=5,6,5 tex2D",
    "rgba=16f texRECT",
    "rgba=32f texRECT depthTexRECT",
    "rgba=16f texRECT depthTexRECT ctt",
    "r=32f texRECT depth ctt",
    "rgb double tex2D",
    "r=32f texRECT ctt aux=4"
};

static int g_numModeTestStrings = sizeof(g_modeTestStrings) / sizeof(char*);
static int g_currentString      = 0;

//---------------------------------------------------------------------------
// Function     	: PrintGLerror
// Description	    : 
//---------------------------------------------------------------------------
void PrintGLerror( const char *msg )
{
    GLenum errCode;
    const GLubyte *errStr;
    
    if ((errCode = glGetError()) != GL_NO_ERROR) 
    {
        errStr = gluErrorString(errCode);
        fprintf(stderr,"OpenGL ERROR: %s: %s\n", errStr, msg);
    }
}

//---------------------------------------------------------------------------
// Function     	: CreateRenderTexture
// Description	    : 
//---------------------------------------------------------------------------
RenderTexture* CreateRenderTexture(const char *initstr)
{
    printf("\nCreating with init string: \"%s\"\n", initstr);

    int texWidth = 256, texHeight = 256;

    // Test deprecated interface
    //RenderTexture *rt2 = new RenderTexture(texWidth, texHeight);
    //if (!rt2->Initialize(true,false,false,false,false,8,8,8,0))

    RenderTexture *rt2 = new RenderTexture(); 
    rt2->Reset(initstr);
    if (!rt2->Initialize(texWidth, texHeight))
    {
        fprintf(stderr, "RenderTexture Initialization failed!\n");
    }
    else
    {
        printf("RenderTexture Initialization done.\n");
    }

    // for shadow mapping we still have to bind it and set the correct 
    // texture parameters using the SGI_shadow or ARB_shadow extension
    // setup the rendering context for the RenderTexture
    if (rt2->BeginCapture())
    {
        Reshape(texWidth, texHeight);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0, 0, 3, 0, 0, 0, 0, 1, 0);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST); 
        glClearColor(0.2, 0.2, 0.2, 1);
        rt2->EndCapture();
    }

    // enable linear filtering if available
    if (rt2->IsTexture() || rt2->IsDepthTexture())
    {
        if (rt2->IsMipmapped())
        {
            // Enable trilinear filtering so we can see the mipmapping
            if (rt2->IsTexture())
            {
                rt2->Bind();
                glTexParameteri(rt2->GetTextureTarget(),
                                GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(rt2->GetTextureTarget(),
                                GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(rt2->GetTextureTarget(),
                                GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
            }
            
            if (rt2->IsDepthTexture())
            {
                rt2->BindDepth();
                glTexParameteri(rt2->GetTextureTarget(),
                                GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(rt2->GetTextureTarget(),
                                GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(rt2->GetTextureTarget(),
                                GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
            }
        }   
        else if (!(rt2->IsRectangleTexture() || rt2->IsFloatTexture()))
        {
            if (rt2->IsTexture())
            {
                rt2->Bind();
                glTexParameteri(rt2->GetTextureTarget(),
                                GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(rt2->GetTextureTarget(),
                                GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            
            if (rt2->IsDepthTexture())
            {
                rt2->BindDepth();
                glTexParameteri(rt2->GetTextureTarget(),
                                GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(rt2->GetTextureTarget(),
                                GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
        }
    }

    if (rt2->IsDepthTexture())
    {
        fprintf(stderr, 
            "\nPress the spacebar to toggle color / depth textures.\n");
        if (!rt2->IsTexture())
            bShowDepthTexture = true;
    }
    else 
    {
        if (rt2->IsTexture())
            bShowDepthTexture = false;
    }

    PrintGLerror("Create");
    return rt2;
}

//---------------------------------------------------------------------------
// Function     	: DestroyRenderTexture
// Description	    : 
//---------------------------------------------------------------------------
void DestroyRenderTexture(RenderTexture *rt2)
{
    delete rt2;
}

//---------------------------------------------------------------------------
// Function     	: Keyboard
// Description	    : 
//---------------------------------------------------------------------------
void Keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 27: 
    case 'q':
        exit(0);
        break;
    case ' ':
        bShowDepthTexture = !bShowDepthTexture;
        break;
    case 13:
        ++g_currentString %= g_numModeTestStrings;
        dbg_printf("Changed to #%d = [%s]\n", g_currentString, g_modeTestStrings[g_currentString]);
        DestroyRenderTexture(rt);
        rt = CreateRenderTexture(g_modeTestStrings[g_currentString]);
        break;
    case 't':
        bTorusMotion = !bTorusMotion;
        break;
    case 'r':
        bRectMotion = !bRectMotion;
        break;
    default:
        return;
    }
}

//---------------------------------------------------------------------------
// Function     	: Idle
// Description	    : 
//---------------------------------------------------------------------------
void Idle()
{
    // make sure we don't try to display nonexistent textures
    if (!rt->IsDepthTexture())
        bShowDepthTexture = false; 
    
    if (bRectMotion) rectAngle += 1;
    if (bTorusMotion) torusAngle += 1;
    glutPostRedisplay();
}

//---------------------------------------------------------------------------
// Function     	: Reshape
// Description	    : 
//---------------------------------------------------------------------------
void Reshape(int w, int h)
{
    if (h == 0) h = 1;
    
    glViewport(0, 0, w, h);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    gluPerspective(60.0, (GLfloat)w/(GLfloat)h, 1, 5.0);
}

//---------------------------------------------------------------------------
// Function     	: Display
// Description	    : 
//---------------------------------------------------------------------------
void _Display()
{
    if (rt->IsInitialized() && rt->BeginCapture())
    {
      if (rt->IsDoubleBuffered()) glDrawBuffer(GL_BACK);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        
        glRotatef(torusAngle, 1, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColor3f(1,1,0);
        
        glutSolidTorus(0.25, 1, 32, 64);
        
        glPopMatrix();
        PrintGLerror("RT Update");

    rt->EndCapture();
    }    

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(1, 1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glRotatef(rectAngle / 10, 0, 1, 0);
        
    if(bShowDepthTexture && rt->IsDepthTexture())
        rt->BindDepth();
    else if (rt->IsTexture()) {
        rt->Bind();
    }

    rt->EnableTextureTarget();

    int maxS = rt->GetMaxS();
    int maxT = rt->GetMaxT();  
    
    glBegin(GL_QUADS);
    glTexCoord2f(0,       0); glVertex2f(-1, -1);
    glTexCoord2f(maxS,    0); glVertex2f( 1, -1);
    glTexCoord2f(maxS, maxT); glVertex2f( 1,  1);
    glTexCoord2f(0,    maxT); glVertex2f(-1,  1);
    glEnd();
    
    rt->DisableTextureTarget();
          
    glPopMatrix();
    
    PrintGLerror("display");
    glutSwapBuffers();
}




//---------------------------------------------------------------------------
// Function     	: main
// Description	    : 
//---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int argn = argc;
    glutInit(&argn, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowPosition(50, 50);
    glutInitWindowSize(512, 512);
    glutCreateWindow("TestRenderTexture");  
    
    glutDisplayFunc(_Display);
    glutIdleFunc(Idle);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    
    Reshape(512, 512);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, 2, 0, 0, 0, 0, 1, 0);
    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_DEPTH_TEST); 
    glClearColor(0.4, 0.6, 0.8, 1);
    

    rt = CreateRenderTexture(g_modeTestStrings[g_currentString]);

    if (rt->IsInitialized() && rt->BeginCapture()) {
      rt->EndCapture();
      dbg_printf("Torus should also be shown.\n");
    } else {
      dbg_printf("No Torus init = %s\n", (rt->IsInitialized() ? "ok" : "NOT INITIALISED"));
    }
    
    printf("Press Enter to change RenderTexture parameters.\n"
           "Press 'r' to toggle the rectangle's motion.\n"
           "Press 't' to toggle the torus' motion.\n");

    
    glutMainLoop();
    return 0;
}
