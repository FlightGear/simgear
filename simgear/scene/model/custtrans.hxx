/**
 * $Id$
 */

#ifndef _SG_CUSTOM_TRANSFORM_HXX
#define _SG_CUSTOM_TRANSFORM_HXX 1

#include "plib/ssg.h"

class SGCustomTransform : public ssgBranch
{
public:
    typedef void (*TransCallback)( sgMat4 r, sgFrustum *f, sgMat4 m, void *d );
    virtual ssgBase *clone( int clone_flags = 0 );
    SGCustomTransform();
    virtual ~SGCustomTransform(void);

    void setTransCallback( TransCallback c, void *d ) {
        _callback = c;
        _data = d;
    }

    virtual const char *getTypeName(void);
    virtual void cull( sgFrustum *f, sgMat4 m, int test_needed );

protected:
    virtual void copy_from( SGCustomTransform *src, int clone_flags );

private:
    TransCallback _callback;
    void *_data;
};

#endif // _SG_CUSTOM_TRANSFORM_HXX
