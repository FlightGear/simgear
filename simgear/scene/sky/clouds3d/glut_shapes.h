#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif


#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/gl.h>

#define APIENTRY

#ifdef __cplusplus
extern "C" {
#endif

extern void APIENTRY glutWireSphere(GLdouble radius, GLint slices, GLint stacks)
;
extern void APIENTRY glutSolidSphere(GLdouble radius, GLint slices, GLint stacks
);
extern void APIENTRY glutWireCone(GLdouble base, GLdouble height, GLint slices, 
GLint stacks);
extern void APIENTRY glutSolidCone(GLdouble base, GLdouble height, GLint slices,
 GLint stacks);
extern void APIENTRY glutWireCube(GLdouble size);
extern void APIENTRY glutSolidCube(GLdouble size);
extern void APIENTRY glutWireTorus(GLdouble innerRadius, GLdouble outerRadius, GLint sides, GLint rings);
extern void APIENTRY glutSolidTorus(GLdouble innerRadius, GLdouble outerRadius, GLint sides, GLint rings);
extern void APIENTRY glutWireDodecahedron(void);
extern void APIENTRY glutSolidDodecahedron(void);
extern void APIENTRY glutWireTeapot(GLdouble size);
extern void APIENTRY glutSolidTeapot(GLdouble size);
extern void APIENTRY glutWireOctahedron(void);
extern void APIENTRY glutSolidOctahedron(void);
extern void APIENTRY glutWireTetrahedron(void);
extern void APIENTRY glutSolidTetrahedron(void);
extern void APIENTRY glutWireIcosahedron(void);
extern void APIENTRY glutSolidIcosahedron(void);

#ifdef __cplusplus
}
#endif
