/**
 * $Id$
 */

#include "personality.hxx"

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
