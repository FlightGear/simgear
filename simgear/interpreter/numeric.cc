// ----------------------------------------------------------------------------
//  Description      : Numeric / order processing
// ----------------------------------------------------------------------------
//  (c) Copyright 1998 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#include <ixlib_numeric.hh>




// BCD encoding ---------------------------------------------------------------
unsigned long ixion::unsigned2BCD(unsigned long value)
{
  unsigned long bcdvalue = 0,bcdshift = 0;
  while (value) {
    bcdvalue += (value % 10) << bcdshift;
    bcdshift += 4;
    value /= 10;
  }
  return bcdvalue;
}




unsigned long ixion::BCD2unsigned(unsigned long value)
{
  unsigned long decvalue = 0;
  for (unsigned long i = 1;value;i *= 10) {
    decvalue += (value & 0xf) * i;
    value >>= 4;
  }
  return decvalue;
}
