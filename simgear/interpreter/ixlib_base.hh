/* ----------------------------------------------------------------------------
    Description      : iXiONmedia library base declarations
   ----------------------------------------------------------------------------
    (c) Copyright 1996 by iXiONmedia, all rights reserved.
   ----------------------------------------------------------------------------
   This header must be C-safe for autoconf purposes.
   */




#ifndef IXLIB_BASE
#define IXLIB_BASE


#undef HAVE_CONFIG_H

#ifdef HAVE_CONFIG_H
#include <ixlib_config.hh>
#undef PACKAGE
#undef VERSION
#endif




#ifdef __cplusplus
namespace ixion {
  extern "C" {
#endif
/* Aliases --------------------------------------------------------------------
*/
  const double Pi = 3.141592653589793285;
  const double Euler = 2.718281828;
  const double Gravity = 9.8065; // m/s^2
  const double UniGravity = 6.673e-11; // m^3/kg s^2
  const double Epsilon0 = 8.8542e-12; // F/m
  const double Mu0 = 1.2566e-6; // H/m
  const double LightSpeed = 2.9972e8; // m/s
  const double Planck = 6.6261e-34; // Js




/* STL Helper macro -----------------------------------------------------------
*/
#define FOREACH(VAR,LIST,LISTTYPE) \
  for (LISTTYPE::iterator VAR = (LIST).begin(),last = (LIST).end();VAR != last;VAR++) 
#define FOREACH_CONST(VAR,LIST,LISTTYPE) \
  for (LISTTYPE::const_iterator VAR = (LIST).begin(),last = (LIST).end();VAR != last;VAR++) 




/* Nomenclature typedefs ------------------------------------------------------
*/
  typedef unsigned char   	TUnsigned8;
  typedef unsigned short  	TUnsigned16;
  typedef unsigned long   	TUnsigned32;
  typedef unsigned long long 	TUnsigned64;
  
  typedef signed char   	TSigned8;
  typedef signed short  	TSigned16;
  typedef signed long   	TSigned32;
  typedef signed long long 	TSigned64;

  typedef TSigned8      	TDelta8;
  typedef TSigned16     	TDelta16;
  typedef TSigned32     	TDelta32;
  typedef TSigned64		TDelta64;
  typedef signed        	TDelta;
  
  typedef TUnsigned8      	TSize8;
  typedef TUnsigned16     	TSize16;
  typedef TUnsigned32     	TSize32;
  typedef TUnsigned64		TSize64;
  typedef unsigned        	TSize;
  
  typedef TUnsigned8      	TIndex8;
  typedef TUnsigned16     	TIndex16;
  typedef TUnsigned32     	TIndex32;
  typedef TUnsigned64		TIndex64;
  typedef unsigned        	TIndex;
  
  typedef TUnsigned8		TByte;
  
  
  
  
  int ixlibGetMajorVersion();
  int ixlibGetMinorVersion();
  int ixlibGetMicroVersion();

  void ixlibInitI18n();
  
  
  
  
  #ifdef __cplusplus
    }
  }
  #endif




#endif
