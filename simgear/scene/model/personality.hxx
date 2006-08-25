/**
 * $Id$
 */

#ifndef _SG_PERSONALITY_HXX
#define _SG_PERSONALITY_HXX 1

#include <simgear/compiler.h>
#include <plib/ssg.h>

#include <map>

SG_USING_STD(map);

class SGAnimation;

class SGPersonalityBranch : public ssgBranch {
public:
    SGPersonalityBranch();
    void setDoubleValue( double value, SGAnimation *anim, int var_id, int var_num = 0 );
    void setIntValue( int value, SGAnimation *anim, int var_id, int var_num = 0 );
    double getDoubleValue( SGAnimation *anim, int var_id, int var_num = 0 ) const;
    int getIntValue( SGAnimation *anim, int var_id, int var_num = 0 ) const;

    SGPersonalityBranch *_old_current;

private:
    struct Key {
        Key( SGAnimation *a, int i, int n = 0 ) : anim(a), var_id(i), var_num(n) {}
        SGAnimation *anim;
        int var_id;
        int var_num;
        bool operator<( const Key &r ) const {
                return anim < r.anim || 
                    ( anim == r.anim && ( var_id < r.var_id ||
                                        ( var_id == r.var_id && var_num < r.var_num ) ) );
        }
    };
    map<Key,double> _doubleValues;
    map<Key,int> _intValues;
};

#endif // _SG_PERSONALITY_HXX
