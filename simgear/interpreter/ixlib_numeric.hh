// ----------------------------------------------------------------------------
//  Description      : numeric / order processing
// ----------------------------------------------------------------------------
//  (c) Copyright 1996 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_NUMERIC
#define IXLIB_NUMERIC




#include <cstring>
#include <ixlib_base.hh>
#include <ixlib_exgen.hh>




// Macros ---------------------------------------------------------------------
#ifdef _GNUC_
  #define NUM_MIN(a,b) ( (a)<?(b) )
  #define NUM_MAX(a,b) ( (a)>?(b) )
  #define NUM_ABS(a)   ( (a)<0 ? (-(a)) : (a) )
#else
  #define NUM_MIN(a,b) ( (a)<(b) ? (a) : (b) )
  #define NUM_MAX(a,b) ( (a)>(b) ? (a) : (b) )
  #define NUM_ABS(a)   ( (a)<0 ? (-(a)) : (a) )
#endif

#define NUM_LIMIT(lower,value,upper) \
  ( NUM_MAX(lower,NUM_MIN(upper,vallue)) )
#define NUM_INBOUND(lower,value,upper) \
  (((lower) <= (value)) && ((value) <= (upper)))
#define NUM_OVERLAP(a1,a2,b1,b2) \
  ((((a1)<=(b1))&&((a2)>(b1)))||(((a1)<(b2))&&((a2)>(b2)))||(((a1)>=(b1))&&((a2)<=(b2))))
#define NUM_CIRCLEINC(index,size) \
  ( ((index)+1) >= (size) ? 0 : ((index)+1) )
#define NUM_CIRCLEDIST(head,tail,size) \
  ( (head)<(tail) ? ((head)+(size)-(tail)) : ((head)-(tail)) )




// Primitive inlines ---------------------------------------------------------
namespace ixion {
  inline signed short sgn(signed long value);
  inline bool getBit(unsigned long value,char bit);
  inline TUnsigned8 hiByte(TUnsigned16 value);
  inline TUnsigned16 hiWord(TUnsigned32 value);
  inline TUnsigned8 loByte(TUnsigned16 value);
  inline TUnsigned16 loWord(TUnsigned32 value);
  inline TUnsigned16 makeWord(TUnsigned8 hi,TUnsigned8 lo);
  inline TUnsigned32 makeDWord(TUnsigned16 hi,TUnsigned16 lo);




// BCD encoding ---------------------------------------------------------------
  unsigned long unsigned2BCD(unsigned long value);
  unsigned long BCD2unsigned(unsigned long value);




// Primitive inlines ---------------------------------------------------------
  inline signed short ixion::sgn(signed long value) {
    return (value<0) ? -1 : ( (value>0) ? 1 : 0);
    }
  
  
  
  
  inline bool ixion::getBit(unsigned long value,char bit) {
    return (value >> bit) & 1;
    }
  
  
  
  
  inline TUnsigned8 ixion::hiByte(TUnsigned16 value) {
    return value >> 8;
    }
  
  
  
  
  inline TUnsigned16 ixion::hiWord(TUnsigned32 value) {
    return value >> 16;
    }
  
  
  
  
  inline TUnsigned8 ixion::loByte(TUnsigned16 value) {
    return value & 0xff;
    }
  
  
  
  
  inline TUnsigned16 ixion::loWord(TUnsigned32 value) {
    return value & 0xffff;
    }
  
  
  
  
  inline TUnsigned16 ixion::makeWord(TUnsigned8 hi,TUnsigned8 lo) {
    return (TUnsigned16) hi << 8 | lo;
    }
  
  
  
  
  inline TUnsigned32 ixion::makeDWord(TUnsigned16 hi,TUnsigned16 lo) {
    return (TUnsigned32) hi << 16 | lo;
    }
  }




#endif
