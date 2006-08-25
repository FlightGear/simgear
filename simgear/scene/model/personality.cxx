/**
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "personality.hxx"
#include "animation.hxx"

static int
personality_pretrav_callback(ssgEntity * entity, int mask)
{
    ((SGPersonalityBranch *)entity)->_old_current = SGAnimation::current_object;
    SGAnimation::current_object = (SGPersonalityBranch *)entity;
    return 1;
}

static int
personality_posttrav_callback(ssgEntity * entity, int mask)
{
    SGAnimation::current_object = ((SGPersonalityBranch *)entity)->_old_current;
    ((SGPersonalityBranch *)entity)->_old_current = 0;
    return 1;
}

SGPersonalityBranch::SGPersonalityBranch()
{
    setTravCallback(SSG_CALLBACK_PRETRAV, personality_pretrav_callback);
    setTravCallback(SSG_CALLBACK_POSTTRAV, personality_posttrav_callback);
}

void SGPersonalityBranch::setDoubleValue( double value, SGAnimation *anim, int var_id, int var_num )
{
    _doubleValues[ Key( anim, var_id, var_num ) ] = value;
}

void SGPersonalityBranch::setIntValue( int value, SGAnimation *anim, int var_id, int var_num )
{
    _intValues[ Key( anim, var_id, var_num ) ] = value;
}

double SGPersonalityBranch::getDoubleValue( SGAnimation *anim, int var_id, int var_num ) const
{
    map<Key,double>::const_iterator it = _doubleValues.find( Key( anim, var_id, var_num ) );
    if ( it != _doubleValues.end() ) {
        return it->second;
    } else {
        return 0;
    }
}

int SGPersonalityBranch::getIntValue( SGAnimation *anim, int var_id, int var_num ) const
{
    map<Key,int>::const_iterator it = _intValues.find( Key( anim, var_id, var_num ) );
    if ( it != _intValues.end() ) {
        return it->second;
    } else {
        return 0;
    }
}
