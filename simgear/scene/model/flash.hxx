/**
 * $Id$
 */

#ifndef _SG_FLASH_HXX
#define _SG_FLASH_HXX 1

class SGFlash : public ssgBranch
{
public:
    virtual ssgBase *clone( int clone_flags = 0 );
    SGFlash();
    virtual ~SGFlash(void);

    void setAxis( sgVec3 axis );
    void setCenter( sgVec3 center );
    void setParameters( float power, float factor, float offset, bool two_sides );
    void setClampValues( float min_v, float max_v );

    virtual const char *getTypeName(void);
    virtual int load( FILE *fd );
    virtual int save( FILE *fd );
    virtual void cull( sgFrustum *f, sgMat4 m, int test_needed );
    virtual void isect( sgSphere *s, sgMat4 m, int test_needed );
    virtual void hot( sgVec3 s, sgMat4 m, int test_needed );
    virtual void los( sgVec3 s, sgMat4 m, int test_needed );

protected:
    virtual void copy_from( SGFlash *src, int clone_flags );

private:
    sgVec3 _axis, _center;
    float _power, _factor, _offset, _min_v, _max_v;
    bool _two_sides;
};

#endif // _SG_FLASH_HXX
