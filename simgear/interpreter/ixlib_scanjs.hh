// ----------------------------------------------------------------------------
//  Description      : Javascript scanner
// ----------------------------------------------------------------------------
//  (c) Copyright 2000 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_SCANJS
#define IXLIB_SCANJS




#undef yyFlexLexer
#define yyFlexLexer jsFlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer




#endif
