/**
 * $Id$
 */

#ifndef _SG_PERSPARAM_HXX
#define _SG_PERSPARAM_HXX 1

#include <simgear/math/sg_random.h>


template <class T>
class SGPersonalityParameter {
public:
  SGPersonalityParameter( SGPropertyNode *props, const char *name, T defval )
    : _var( defval ), _min( defval ), _max( defval ) {
    SGPropertyNode_ptr node = props->getNode( name );
    if ( node != 0 ) {
      SGPropertyNode_ptr rand_n = node->getNode( "random" );
      if ( rand_n != 0 ) {
        _min = rand_n->getDoubleValue( "min", 0.0 );
        _max = rand_n->getDoubleValue( "max", 1.0 );
        shuffle();
      } else {
        _var = _min = _max = getNodeValue( props, name, defval );
      }
    }
  }
  SGPersonalityParameter<T> &operator=( T v ) { _var = v; return *this; }
  SGPersonalityParameter<T> &operator+=( T v ) { _var += v; return *this; }
  SGPersonalityParameter<T> &operator-=( T v ) { _var -= v; return *this; }
  T shuffle() { return ( _var = _min + sg_random() * ( _max - _min ) ); }
  T value() const { return _var; }
  T getNodeValue( SGPropertyNode *props, const char *name, T defval ) const;
  operator T() const { return _var; }

private:
  T _var;
  T _min;
  T _max;
};

#endif // _SG_PERSPARAM_HXX
