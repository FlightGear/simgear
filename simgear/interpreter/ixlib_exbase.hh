// ----------------------------------------------------------------------------
//  Description      : Exception handling
// ----------------------------------------------------------------------------
//  (c) Copyright 1996 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_EXBASE
#define IXLIB_EXBASE




#include <stdexcept>
#include <ixlib_base.hh>




// constants ------------------------------------------------------------------
#define EX_INFOMAX 255




// throw macro ----------------------------------------------------------------
#define EX_THROW(TYPE,CODE)\
  throw ::ixion::TYPE##_exception(CODE,NULL,__FILE__,__LINE__);
#define EX_THROWINFO(TYPE,CODE,INFO)\
  throw ::ixion::TYPE##_exception(CODE,(char const *) INFO,__FILE__,__LINE__);
#define EX_CATCHCODE(TYPE,CODE,HANDLER)\
  catch (TYPE##_exception &ex) { \
    if (ex.Error != CODE) throw; \
    HANDLER \
  }
#define EX_CONVERT(TYPE,CODE,DESTTYPE,DESTCODE)\
  catch (TYPE##_exception &ex) { \
    if (ex.Error != CODE) throw; \
    throw DESTTYPE##_exception(DESTCODE,ex.Info,__FILE__,__LINE__); \
  }
  
  
  
  
// xBaseException -------------------------------------------------------------
namespace ixion {
  typedef unsigned int TErrorCode;
  
  
  
  
  struct base_exception : public std::exception {
    TErrorCode          Error;
    char                *Module;
    TIndex             	Line;
    char                *Category;
    bool		HasInfo;
    char 		Info[EX_INFOMAX+1];
    static char		RenderBuffer[EX_INFOMAX+1+100];
  
    base_exception(TErrorCode error,char const *info = NULL,char *module = NULL,
      TIndex line = 0,char *category = NULL);
    char const *what() const throw ();
    virtual const char *getText() const = 0;
    };
  }



#endif
