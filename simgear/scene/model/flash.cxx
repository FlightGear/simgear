/*
     $Id$
*/

#include "plib/ssg.h"
#include "flash.hxx"
void _ssgPushMatrix ( sgMat4 m );
void _ssgPopMatrix  ();
void _ssgReadInt     ( FILE *fd,                int   *var );
void _ssgWriteInt    ( FILE *fd, const          int    var );
extern sgMat4 _ssgOpenGLAxisSwapMatrix;

void SGFlash::copy_from( SGFlash *src, int clone_flags )
{
  ssgBranch::copy_from( src, clone_flags );
  sgCopyVec3( _center, src->_center );
  sgCopyVec3( _axis, src->_axis );
  _power = src->_power;
  _factor = src->_factor;
  _offset = src->_offset;
  _min_v = src->_min_v;
  _max_v = src->_max_v;
  _two_sides = src->_two_sides;
}

ssgBase *SGFlash::clone( int clone_flags )
{
  SGFlash *b = new SGFlash;
  b -> copy_from( this, clone_flags );
  return b;
}


SGFlash::SGFlash()
{
  type = ssgTypeBranch();
}

SGFlash::~SGFlash()
{
}

void SGFlash::setAxis( sgVec3 axis )
{
  sgCopyVec3( _axis, axis );
  sgNormalizeVec3( _axis );
}

void SGFlash::setCenter( sgVec3 center )
{
  sgCopyVec3( _center, center );
}

void SGFlash::setParameters( float power, float factor, float offset, bool two_sides )
{
  _power = power;
  _factor = factor;
  _offset = offset;
  _two_sides = two_sides;
}

void SGFlash::setClampValues( float min_v, float max_v )
{
  _min_v = min_v;
  _max_v = max_v;
}

void SGFlash::cull( sgFrustum *f, sgMat4 m, int test_needed )
{
  if ( ! preTravTests( &test_needed, SSGTRAV_CULL ) )
    return;

  sgVec3 transformed_axis;
  sgXformVec3( transformed_axis, _axis, m );
  sgNormalizeVec3( transformed_axis );

  float cos_angle = transformed_axis[ 2 ]; // z component, through the screen
  float scale_factor = 0.f;
  if ( _two_sides && cos_angle < 0 )
    scale_factor = _factor * (float)pow( -cos_angle, _power ) + _offset;
  else if ( cos_angle > 0 )
    scale_factor = _factor * (float)pow( cos_angle, _power ) + _offset;

  if ( scale_factor < _min_v )
      scale_factor = _min_v;
  if ( scale_factor > _max_v )
      scale_factor = _max_v;

  sgMat4 tmp, transform;
  sgMakeIdentMat4( transform );
  transform[0][0] = scale_factor;
  transform[1][1] = scale_factor;
  transform[2][2] = scale_factor;
  transform[3][0] = _center[0] * ( 1 - scale_factor );
  transform[3][1] = _center[1] * ( 1 - scale_factor );
  transform[3][2] = _center[2] * ( 1 - scale_factor );

  sgCopyMat4( tmp, m );
  sgPreMultMat4( tmp, transform );

  _ssgPushMatrix( tmp );
  glPushMatrix();
  glLoadMatrixf( (float *) tmp );

  for ( ssgEntity *e = getKid ( 0 ); e != NULL; e = getNextKid() )
    e -> cull( f, tmp, test_needed );

  glPopMatrix();
  _ssgPopMatrix();

  postTravTests( SSGTRAV_CULL );
}


void SGFlash::hot( sgVec3 s, sgMat4 m, int test_needed )
{
  ssgBranch::hot( s, m, test_needed );
}

void SGFlash::los( sgVec3 s, sgMat4 m, int test_needed )
{
  ssgBranch::los( s, m, test_needed );
}


void SGFlash::isect( sgSphere *s, sgMat4 m, int test_needed )
{
  ssgBranch::isect( s, m, test_needed );
}



int SGFlash::load( FILE *fd )
{
//  _ssgReadInt( fd, & point_rotate );

  return ssgBranch::load(fd);
}

int SGFlash::save( FILE *fd )
{
//  _ssgWriteInt( fd, point_rotate );

  return ssgBranch::save(fd);
}

const char *SGFlash::getTypeName (void) { return "SGFlash"; }
