// ----------------------------------------------------------------------------
//  Description      : Generic exceptions
// ----------------------------------------------------------------------------
//  (c) Copyright 1996 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_EXGEN
#define IXLIB_EXGEN




#include <ixlib_exbase.hh>




// Error codes ----------------------------------------------------------------
#define EC_CANNOTEVALUATE               0
#define EC_NOTYETIMPLEMENTED            1
#define EC_ERROR                        2
#define EC_NULLPOINTER                  3
#define EC_INVALIDPAR			4
#define EC_INDEX			5
#define EC_BUFFEROVERFLOW               6
#define EC_BUFFERUNDERFLOW              7
#define EC_ITEMNOTFOUND                 8
#define EC_INVALIDOP                    9
#define EC_DIMENSIONMISMATCH		10
#define EC_CANCELLED			11
#define EC_EMPTYSET			12
#define EC_CANNOTREMOVEFROMGC		13
#define EC_REMAININGREF			14

#define ECMEM_GENERAL                   0



// Throw macro ----------------------------------------------------------------
#define EXGEN_THROW(CODE)\
  EX_THROW(generic,CODE)
#define EXGEN_THROWINFO(CODE,INFO)\
  EX_THROWINFO(generic,CODE,INFO)
#define EXGEN_NYI\
  EXGEN_THROW(EC_NOTYETIMPLEMENTED)




namespace ixion {
// generic_exception ----------------------------------------------------------
  struct generic_exception : public base_exception {
    generic_exception(TErrorCode error,char const *info = NULL,char *module = NULL,
      TIndex line = 0)
      : base_exception(error,info,module,line,"GEN") { 
      }
    virtual char const *getText() const;
    };
  }




#endif
