// ----------------------------------------------------------------------------
//  Description      : ixlib internationalization wrapper 
// ----------------------------------------------------------------------------
//  (c) Copyright 2001 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_I18N




#include <string>
#include <libintl.h>
#define _(String) gettext(String)
#define N_(String) (String)




#endif
