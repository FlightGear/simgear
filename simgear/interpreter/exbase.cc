// ----------------------------------------------------------------------------
//  Description      : Exception handling
// ----------------------------------------------------------------------------
//  (c) Copyright 1996 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#include <cstdio>
#include <cstring>
#include <ixlib_exbase.hh>




using namespace ixion;




// Description forms ----------------------------------------------------------
#define T_DESCRIPTION1          "[%s%04X] %s"
#define T_DESCRIPTION2          "[%s%04X] %s <%s>"
#define T_DESCRIPTION3          "[%s%04X] %s <%s,%d>"
#define T_DESCRIPTION1I         "[%s%04X] %s (%s)"
#define T_DESCRIPTION2I         "[%s%04X] %s (%s) <%s>"
#define T_DESCRIPTION3I         "[%s%04X] %s (%s) <%s,%d>"




// base_exception -------------------------------------------------------------
char base_exception::RenderBuffer[EX_INFOMAX+1+100];




base_exception::base_exception(TErrorCode error,char const *info,char *module,
  TIndex line,char *category)
: Error(error),Module(module),Line(line),Category(category) {
  HasInfo = (info!=NULL);
  if (info) {
    if (strlen(info)>EX_INFOMAX) {
      strncpy(Info,info,EX_INFOMAX);
      Info[EX_INFOMAX] = '\0';
      }
    else strcpy(Info,info);
    }
  }




char const *base_exception::what() const throw () {
  if (HasInfo) {
    if (Module) {
      if (Line)
	sprintf(RenderBuffer,T_DESCRIPTION3I,Category,Error,getText(),Info,Module,Line);
      else
	sprintf(RenderBuffer,T_DESCRIPTION2I,Category,Error,getText(),Info,Module);
      }
    else
      sprintf(RenderBuffer,T_DESCRIPTION1I,Category,Error,getText(),Info);
    }
  else {
    if (Module) {
      if (Line)
	sprintf(RenderBuffer,T_DESCRIPTION3,Category,Error,getText(),Module,Line);
      else
	sprintf(RenderBuffer,T_DESCRIPTION2,Category,Error,getText(),Module);
    }
    else
      sprintf(RenderBuffer,T_DESCRIPTION1,Category,Error,getText());
    }
  return RenderBuffer;
  }
