/*
     $Id$
*/

#include "plib/ssg.h"
#include "custtrans.hxx"
void _ssgPushMatrix ( sgMat4 m );
void _ssgPopMatrix  ();
void _ssgReadInt     ( FILE *fd,                int   *var );
void _ssgWriteInt    ( FILE *fd, const          int    var );
extern sgMat4 _ssgOpenGLAxisSwapMatrix;

void SGCustomTransform::copy_from( SGCustomTransform *src, int clone_flags )
{
  ssgBranch::copy_from( src, clone_flags );
  _callback = src->_callback;
  _data = (void *)src->_callback;
}

ssgBase *SGCustomTransform::clone( int clone_flags )
{
  SGCustomTransform *b = new SGCustomTransform;
  b -> copy_from( this, clone_flags );
  return b;
}


SGCustomTransform::SGCustomTransform()
 : _callback(0),_data(0)
{
  type = ssgTypeBranch();
}

SGCustomTransform::~SGCustomTransform()
{
}

void SGCustomTransform::cull( sgFrustum *f, sgMat4 m, int test_needed )
{
  if ( ! preTravTests( &test_needed, SSGTRAV_CULL ) )
    return;

  if ( _callback ) {
    sgMat4 tmp;
    _callback( tmp, f, m, _data );

    _ssgPushMatrix( tmp );
    glPushMatrix();
    glLoadMatrixf( (float *) tmp );

    for ( ssgEntity *e = getKid ( 0 ); e != NULL; e = getNextKid() )
      e -> cull( f, tmp, test_needed );

    glPopMatrix();
    _ssgPopMatrix();
  }
  postTravTests( SSGTRAV_CULL );
}


const char *SGCustomTransform::getTypeName (void) { return "SGCustomTransform"; }
